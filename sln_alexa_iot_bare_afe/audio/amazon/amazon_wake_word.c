// ############################################################################
// #             Copyright (C), NXP Semiconductors
// #                       (C), NXP B.V. of Eindhoven
// #
// # All rights are reserved. Reproduction in whole or in part is prohibited
// # without the written consent of the copyright owner.
// # NXP reserves the right to make changes without notice at any time.
// # NXP makes no warranty, expressed, implied or statutory, including but
// # not limited to any implied warranty of merchantability or fitness for
// # any particular purpose, or that the use will not infringe any third
// # party patent, copyright or trademark. NXP must not be liable for any
// # loss or damage arising from its use.
// ############################################################################

/* FreeRTOS kernel includes. */
#include "board.h"
/* Freescale includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "fsl_debug_console.h"
#include "pdm_pcm_definitions.h"
#include "pryon_lite.h"
#include "amazon_wake_word.h"
#include "FreeRTOSConfig.h"
#include "sln_flash_mgmt.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PRL_MODEL_ALIGN __attribute__((aligned(16)))
#define AMZ_WW_SELFWAKE_DELAY_CYCLES (AMZ_WW_SELFWAKE_DELAY_MS / 10)

/*******************************************************************************
 * Global Vars
 ******************************************************************************/

PryonLiteDecoderConfig config = PryonLiteDecoderConfig_Default;
PryonLiteModelAttributes modelAttributes;
static PryonLiteDecoderHandle sDecoder = NULL;

PryonLiteDecoderConfig configSelfWake = PryonLiteDecoderConfig_Default;
PryonLiteModelAttributes modelAttributesSelfWake;
static PryonLiteDecoderHandle sDecoderSelfWake = NULL;

uint8_t *pu8WakeupDetected = NULL;
uint16_t *pu8WakeupSize    = NULL;

uint8_t u8WakeupDetectedSelfWake = 0;
uint16_t u8WakeupSizeSelfWake    = 0;
uint8_t u8SelfWakeCount          = 0;

char *decoderBuffer         = NULL; // should be an array large enough to hold the largest decoder
char *decoderBufferSelfWake = NULL; // should be an array large enough to hold the largest decoder

// initialize decoder
PryonLiteSessionInfo sessionInfo;
PryonLiteSessionInfo sessionInfoSelfWake;

/* Structure containing Wake Word Information */
typedef struct
{
    uint32_t ww_model_len;
    uint8_t ww_model[AMZ_WW_MODEL_LENGTH];
} tsWakeWordAttributes;

typedef enum
{
    E_AMAZON_WAKE_WORD_MODEL_50KB,
    E_AMAZON_WAKE_WORD_MODEL_150KB,
    E_AMAZON_WAKE_WORD_MODEL_250KB,
} teAmazonWakeWordModelType;

/* Wake word model information */
tsWakeWordAttributes sWakeWordAttr = {0};
SemaphoreHandle_t wwStateLock;

/* Wake Word Model Map Table */
static amzn_ww_model_map ww_model_map[AMZ_WW_NUMBER_OF_WW_MODELS] =    {
                                                                       { "en-US", WR_250k_en_US_alexa },
                                                                       { "en-CA", WR_250k_en_US_alexa },
                                                                       { "es-US", WR_250k_en_US_alexa },
                                                                       { "es-ES", U_250k_es_ES_alexa  },
                                                                       { "es-MX", U_250k_es_ES_alexa  },
                                                                       { "fr-CA", WR_250k_fr_CA_alexa },
                                                                       };

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

uint32_t SLN_AMAZON_WAKE_GetModelLocaleSize()
{
    return sWakeWordAttr.ww_model_len;
}

char * SLN_AMAZON_WAKE_GetLocaleString()
{
    return (char *)sWakeWordAttr.ww_model;
}

uint32_t SLN_AMAZON_WAKE_SetModelLocale(char *modelLocale)
{
    int32_t status      = WW_MODEL_SUCCESS;
    int32_t flashStatus = SLN_FLASH_MGMT_OK;

    sWakeWordAttr.ww_model_len = AMZ_WW_MODEL_LENGTH;
    strncpy((char *)sWakeWordAttr.ww_model, modelLocale, AMZ_WW_MODEL_LENGTH);

    flashStatus = SLN_FLASH_MGMT_Save(AMZ_WW_MODEL_FILE_NAME, sWakeWordAttr.ww_model, sWakeWordAttr.ww_model_len);

    if (flashStatus != SLN_FLASH_MGMT_OK)
    {
        if (flashStatus == SLN_FLASH_MGMT_EOVERFLOW || flashStatus == SLN_FLASH_MGMT_EOVERFLOW2)
        {
            flashStatus = SLN_FLASH_MGMT_Erase(AMZ_WW_MODEL_FILE_NAME);

            if (SLN_FLASH_MGMT_OK != flashStatus)
            {
                configPRINTF(("SLN_FLASH_MGMT_Erase AMZ_WW_MODEL_FILE_NAME Failed due to Overflow!\r\n"));
            }

            flashStatus =
                SLN_FLASH_MGMT_Save(AMZ_WW_MODEL_FILE_NAME, sWakeWordAttr.ww_model, sWakeWordAttr.ww_model_len);

            if (SLN_FLASH_MGMT_OK != flashStatus)
            {
                configPRINTF(("SLN_FLASH_MGMT_Save AMZ_WW_MODEL_FILE_NAME Failed!\r\n"));
            }
        }
        else
        {
            configPRINTF(("SLN_FLASH_MGMT_Save failed and returned: %X\r\n", status));
        }
    }

    if (flashStatus != SLN_FLASH_MGMT_OK)
    {
        status = WW_MODEL_FAILED;
    }

    return status;
}

amzn_ww_model_map *SLN_AMAZON_GetSupportedLocales(void)
{
    return &ww_model_map[0];
}

void SLN_AMAZON_WAKE_GetModelLocale(uint8_t *modelLocal)
{
    uint32_t status = SLN_FLASH_MGMT_OK;

    if (sWakeWordAttr.ww_model_len == 0)
    {
        memset(&sWakeWordAttr, 0, sizeof(sWakeWordAttr));

        /* Check if the ww model is set */
        status = SLN_FLASH_MGMT_Read(AMZ_WW_MODEL_FILE_NAME, sWakeWordAttr.ww_model, &sWakeWordAttr.ww_model_len);

        /* If no model is set, write default value in flash */
        if (status == SLN_FLASH_MGMT_ENOENTRY || status == SLN_FLASH_MGMT_ENOENTRY2)
        {
            strcpy((char *)sWakeWordAttr.ww_model, "en-US");
            sWakeWordAttr.ww_model_len = AMZ_WW_MODEL_LENGTH;

            status = SLN_FLASH_MGMT_Save(AMZ_WW_MODEL_FILE_NAME, sWakeWordAttr.ww_model, sWakeWordAttr.ww_model_len);
            if (status != SLN_FLASH_MGMT_OK)
            {
                if (status == SLN_FLASH_MGMT_EOVERFLOW || status == SLN_FLASH_MGMT_EOVERFLOW2)
                {
                    status = SLN_FLASH_MGMT_Erase(AMZ_WW_MODEL_FILE_NAME);
                    if (SLN_FLASH_MGMT_OK != status)
                    {
                        configPRINTF(
                            ("SLN_FLASH_MGMT_Erase AMZ_WW_MODEL_FILE_NAME Failed and returned : %X!\r\n", status));
                    }
                    status =
                        SLN_FLASH_MGMT_Save(AMZ_WW_MODEL_FILE_NAME, sWakeWordAttr.ww_model, sWakeWordAttr.ww_model_len);
                    if (SLN_FLASH_MGMT_OK != status)
                    {
                        configPRINTF(
                            ("SLN_FLASH_MGMT_Save AMZ_WW_MODEL_FILE_NAME Failed and returned : %X!\r\n", status));
                    }
                }
                else
                {
                    configPRINTF(("SLN_FLASH_MGMT_Save failed and returned: %X\r\n", status));
                }
            }
        }
        else if (status != SLN_FLASH_MGMT_OK)
        {
            configPRINTF(("SLN_FLASH_MGMT_Read failed and returned: %X\r\n", status));
        }
    }

    if (modelLocal != NULL)
    {
        strcpy((char *)modelLocal, (char *)sWakeWordAttr.ww_model);
    }
}

void SLN_AMAZON_WAKE_SetWakeupDetectedParams(uint8_t *pu8Wake, uint16_t *pu16WWLen)
{
    pu8WakeupDetected = pu8Wake;
    pu8WakeupSize     = pu16WWLen;
}

/* Function to check if wake word locale is supported */
uint32_t SLN_AMAZON_WAKE_IsWakeWordSupported(char *lang_code)
{
	if (lang_code == NULL)
	{
		return WW_MODEL_NULL;
	}
#ifdef AMZN_MODEL_U_1S_50k_de_DE_alexa
	if (0 == strcmp(lang_code, "de-DE"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_WS_250k_de_DE_alexa
	if (0 == strcmp(lang_code, "de-DE"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_AU_alexa
	if (0 == strcmp(lang_code, "en-AU"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_WR_250k_en_AU_alexa
	if (0 == strcmp(lang_code, "en-AU"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_CA_alexa
	if (0 == strcmp(lang_code, "en-CA"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_GB_alexa
	if (0 == strcmp(lang_code, "en-GB"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_WR_250k_en_GB_alexa
	if (0 == strcmp(lang_code, "en-GB"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_IN_alexa
	if (0 == strcmp(lang_code, "en-IN"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_WS_250k_en_IN_alexa
	if (0 == strcmp(lang_code, "en-IN"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_D_en_US_alexa
	if (0 == strcmp(lang_code, "en-US"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_US_alexa
	if (0 == strcmp(lang_code, "en-US"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_WR_250k_en_US_alexa
	if (0 == strcmp(lang_code, "en-US"))
	{
		return WW_MODEL_SUCCESS;
	}
	else if (0 == strcmp(lang_code, "en-CA"))
	{
		return WW_MODEL_SUCCESS;
	}
	else if (0 == strcmp(lang_code, "es-US"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_D_es_ES_alexa
	if (0 == strcmp(lang_code, "es-ES"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_es_ES_alexa
	if (0 == strcmp(lang_code, "es-ES"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_250k_es_ES_alexa
	if (0 == strcmp(lang_code, "es-ES"))
	{
		return WW_MODEL_SUCCESS;
	}
	else if (0 == strcmp(lang_code, "es-MX"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_es_MX_alexa
	if (0 == strcmp(lang_code, "es-MX"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_fr_CA_alexa
	if (0 == strcmp(lang_code, "fr-CA"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_WR_250k_fr_CA_alexa
	if (0 == strcmp(lang_code, "fr-CA"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_fr_FR_alexa
	if (0 == strcmp(lang_code, "fr-FR"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_WR_250k_fr_FR_alexa
	if (0 == strcmp(lang_code, "fr-FR"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_it_IT_alexa
	if (0 == strcmp(lang_code, "it-IT"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_WS_250k_it_IT_alexa
	if (0 == strcmp(lang_code, "it-IT"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_U_1S_50k_ja_JP_alexa
	if (0 == strcmp(lang_code, "ja-JP"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_WS_250k_ja_JP_alexa
	if (0 == strcmp(lang_code, "ja-JP"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_D_pt_BR_alexa
	if (0 == strcmp(lang_code, "pt-BR"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
#ifdef AMZN_MODEL_WR_250k_pt_BR_alexa
	if (0 == strcmp(lang_code, "pt-BR"))
	{
		return WW_MODEL_SUCCESS;
	}
#endif
	/* Reaching here means model us not supported */
	return WW_MODEL_NOT_SUPPORTED;
}

/* Function to assign the model that is requested */
static void getWakeWordModelFromLocale(teAmazonWakeWordModelType modelType)
{
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "de-DE"))
	{
#ifdef AMZN_MODEL_U_1S_50k_de_DE_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_de_DE_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_de_DE_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_WS_250k_de_DE_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WS_250k_de_DE_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WS_250k_de_DE_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "en-AU"))
	{
#ifdef AMZN_MODEL_U_1S_50k_en_AU_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_en_AU_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_en_AU_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_WR_250k_en_AU_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WR_250k_en_AU_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WR_250k_en_AU_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "en-CA"))
	{
#ifdef AMZN_MODEL_U_1S_50k_en_CA_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_en_CA_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_en_CA_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "en-GB"))
	{
#ifdef AMZN_MODEL_U_1S_50k_en_GB_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_en_GB_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_en_GB_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_WR_250k_en_GB_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WR_250k_en_GB_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WR_250k_en_GB_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "en-IN"))
	{
#ifdef AMZN_MODEL_U_1S_50k_en_IN_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_en_IN_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_en_IN_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_WS_250k_en_IN_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WS_250k_en_IN_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WS_250k_en_IN_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "en-US"))
	{
#ifdef AMZN_MODEL_D_en_US_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				D_en_US_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &D_en_US_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_US_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_en_US_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_en_US_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_WR_250k_en_US_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WR_250k_en_US_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WR_250k_en_US_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "es-ES"))
	{
#ifdef AMZN_MODEL_D_es_ES_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				D_es_ES_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &D_es_ES_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_U_1S_50k_es_ES_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_es_ES_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_es_ES_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_U_250k_es_ES_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				U_250k_es_ES_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_250k_es_ES_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "es-MX"))
	{
#ifdef AMZN_MODEL_U_1S_50k_es_MX_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_es_MX_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_es_MX_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "fr-CA"))
	{
#ifdef AMZN_MODEL_U_1S_50k_fr_CA_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_fr_CA_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_fr_CA_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_WR_250k_fr_CA_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WR_250k_fr_CA_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WR_250k_fr_CA_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "fr-FR"))
	{
#ifdef AMZN_MODEL_U_1S_50k_fr_FR_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_fr_FR_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_fr_FR_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_WR_250k_fr_FR_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WR_250k_fr_FR_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WR_250k_fr_FR_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "it-IT"))
	{
#ifdef AMZN_MODEL_U_1S_50k_it_IT_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_it_IT_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_it_IT_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_WS_250k_it_IT_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WS_250k_it_IT_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WS_250k_it_IT_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "ja-JP"))
	{
#ifdef AMZN_MODEL_U_1S_50k_ja_JP_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_50KB)
		{
			config.sizeofModel =
				U_1S_50k_ja_JP_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_1S_50k_ja_JP_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_WS_250k_ja_JP_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WS_250k_ja_JP_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WS_250k_ja_JP_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "pt-BR"))
	{
#ifdef AMZN_MODEL_D_pt_BR_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				D_pt_BR_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &D_pt_BR_alexa; // pointer to model in memory
		}
#endif
#ifdef AMZN_MODEL_WR_250k_pt_BR_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WR_250k_pt_BR_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WR_250k_pt_BR_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "en-CA"))
	{
#ifdef AMZN_MODEL_WR_250k_en_US_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WR_250k_en_US_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WR_250k_en_US_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "es-US"))
	{
#ifdef AMZN_MODEL_WR_250k_en_US_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				WR_250k_en_US_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &WR_250k_en_US_alexa; // pointer to model in memory
		}
#endif
	}
	if (0 == strcmp((char *)sWakeWordAttr.ww_model, "es-MX"))
	{
#ifdef AMZN_MODEL_U_250k_es_ES_alexa
		if (modelType == E_AMAZON_WAKE_WORD_MODEL_250KB)
		{
			config.sizeofModel =
				U_250k_es_ES_alexaLen; // example value, will be the size of the binary model byte array
			config.model = &U_250k_es_ES_alexa; // pointer to model in memory
		}
#endif
	}
}

static void loadModel(PryonLiteDecoderConfig *config, teAmazonWakeWordModelType modelType)
{
    // In order to detect keywords, the decoder uses a model which defines the parameters,
    // neural network weights, classifiers, etc that are used at runtime to process the audio
    // and give detection results.

    // Each model is packaged in two formats:
    // 1. A .bin file that can be loaded from disk (via fopen, fread, etc)
    // 2. A .c file that can be hard-coded at compile time

    /* This is the first time so Get the current Locale */
    if (sWakeWordAttr.ww_model_len == 0)
    {
        SLN_AMAZON_WAKE_GetModelLocale(NULL);
    }

    getWakeWordModelFromLocale(modelType);
}

// keyword detection callback
static void detectionCallback(PryonLiteDecoderHandle handle, const PryonLiteResult *result)
{
    configPRINTF(("Woke Up\r\n"));
    if (!u8WakeupDetectedSelfWake)
    {
        *pu8WakeupDetected = (0 == strcmp(result->keyword, "ALEXA") ? 1 : 0);
        *pu8WakeupSize     = ((result->endSampleIndex - result->beginSampleIndex) / 160);
    }
}

// VAD event callback
static void vadCallback(PryonLiteDecoderHandle handle, const PryonLiteVadEvent *vadEvent)
{
    // printf("VAD state %d\n", (int) vadEvent->vadState);
}

// keyword detection callback
static void detectionSelfWakeCallback(PryonLiteDecoderHandle handle, const PryonLiteResult *result)
{
    configPRINTF(("Self Wake Woke Up\r\n"));
    u8WakeupDetectedSelfWake = (0 == strcmp(result->keyword, "ALEXA") ? 1 : 0);
}

uint32_t SLN_AMAZON_WAKE_SelfWakeInitialize()
{
    loadModel(&configSelfWake, E_AMAZON_WAKE_WORD_MODEL_50KB);

    // Query for the size of instance memory required by the decoder
    volatile PryonLiteError err =
        PryonLite_GetModelAttributes(configSelfWake.model, configSelfWake.sizeofModel, &modelAttributesSelfWake);

    configSelfWake.decoderMem = (char *)pvPortMalloc(modelAttributesSelfWake.requiredDecoderMem);

    if (configSelfWake.decoderMem == NULL)
    {
        while (1)
            ;
    }

    configSelfWake.sizeofDecoderMem = modelAttributesSelfWake.requiredDecoderMem;

    configSelfWake.detectThreshold = 100;                       // default threshold
    configSelfWake.resultCallback  = detectionSelfWakeCallback; // register detection handler
    // configSelfWake.vadCallback = vadSelfWakeCallback; // register VAD handler
    // configSelfWake.useVad = 1;  // enable voice activity detector
    configSelfWake.lowLatency = 1; // Make it quicker

    sessionInfoSelfWake.samplesPerFrame = 160; // enable voice activity detector

    err = PryonLiteDecoder_Initialize(&configSelfWake, &sessionInfoSelfWake, &sDecoderSelfWake);

    if (err != PRYON_LITE_ERROR_OK)
    {
        // Set detection threshold for all keywords (this function can be called any time after decoder initialization)
        int detectionThreshold = 100;
        err                    = PryonLiteDecoder_SetDetectionThreshold(sDecoderSelfWake, NULL, detectionThreshold);
    }

    return err;
}

uint32_t SLN_AMAZON_WAKE_Initialize()
{

    if (wwStateLock == NULL)
    {
        wwStateLock = xSemaphoreCreateRecursiveMutex();
    }

    xSemaphoreTakeRecursive(wwStateLock, portMAX_DELAY);

    loadModel(&config, E_AMAZON_WAKE_WORD_MODEL_250KB);

    // Query for the size of instance memory required by the decoder
    PryonLiteError err = PryonLite_GetModelAttributes(config.model, config.sizeofModel, &modelAttributes);

    config.decoderMem = (char *)pvPortMalloc(modelAttributes.requiredDecoderMem);

    if (config.decoderMem == NULL)
    {
        while (1)
            ;
    }

    config.sizeofDecoderMem = modelAttributes.requiredDecoderMem;

    config.detectThreshold = 500;               // default threshold
    config.resultCallback  = detectionCallback; // register detection handler
    config.vadCallback     = vadCallback;       // register VAD handler
    config.useVad          = 1;                 // enable voice activity detector

    sessionInfo.samplesPerFrame = 160; // enable voice activity detector

    err = PryonLiteDecoder_Initialize(&config, &sessionInfo, &sDecoder);

    if (err != PRYON_LITE_ERROR_OK)
    {
        // Set detection threshold for all keywords (this function can be called any time after decoder initialization)
        int detectionThreshold = 500;
        err                    = PryonLiteDecoder_SetDetectionThreshold(sDecoder, NULL, detectionThreshold);
    }

    xSemaphoreGiveRecursive(wwStateLock);

    return err;
}

uint32_t SLN_AMAZON_WAKE_Destroy()
{
    PryonLiteError status;

    xSemaphoreTakeRecursive(wwStateLock, portMAX_DELAY);

    status = PryonLiteDecoder_Destroy(&sDecoder);

    if (status == PRYON_LITE_ERROR_OK)
    {
        vPortFree(config.decoderMem);
    }

    xSemaphoreGiveRecursive(wwStateLock);

    return status;
}

uint32_t SLN_AMAZON_WAKE_ProcessWakeWordSelfWake(int16_t *pi16AudioBuff, uint16_t u16BufferSize)
{
    PryonLiteError err;

    /* Push the audio data to detect if a self wake is happening */
    err = PryonLiteDecoder_PushAudioSamples(sDecoderSelfWake, pi16AudioBuff, sessionInfoSelfWake.samplesPerFrame);

    /* if the self wake has already been detected, then increment the delay */
    if (u8WakeupDetectedSelfWake)
    {
        u8SelfWakeCount++;
        if (u8SelfWakeCount > AMZ_WW_SELFWAKE_DELAY_CYCLES)
        {
            u8SelfWakeCount          = 0;
            u8WakeupDetectedSelfWake = 0;
        }
    }

    return err;
}

uint32_t SLN_AMAZON_WAKE_ProcessWakeWord(int16_t *pi16AudioBuff, uint16_t u16BufferSize)
{
    PryonLiteError err = PRYON_LITE_ERROR_OK;

    xSemaphoreTakeRecursive(wwStateLock, portMAX_DELAY);

    if (sDecoder != NULL)
    {
        err = PryonLiteDecoder_PushAudioSamples(sDecoder, pi16AudioBuff, sessionInfo.samplesPerFrame);
    }

    xSemaphoreGiveRecursive(wwStateLock);

    return err;
}
