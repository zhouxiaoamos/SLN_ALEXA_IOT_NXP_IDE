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

#ifndef _AMAZON_WAKE_WORD_H_
#define _AMAZON_WAKE_WORD_H_

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define AMZ_WW_MODEL_FILE_NAME "amz_ww_model.dat"
#define AMZ_WW_MODEL_LENGTH 6

#define AMZ_WW_NUMBER_OF_WW_MODELS 6

#define AMZN_MODEL_SUPPORTED_LOCALES \
    "\t\t\t\ten-US\r\n" \
    "\t\t\t\ten-CA\r\n" \
    "\t\t\t\tes-US\r\n" \
    "\t\t\t\tes-ES\r\n" \
    "\t\t\t\tes-MX\r\n" \
    "\t\t\t\tfr-CA\r\n" \

/* Wake Word Model Support Definitions */
#define AMZN_MODEL_WR_250k_en_US_alexa
#define AMZN_MODEL_U_250k_es_ES_alexa
#define AMZN_MODEL_WR_250k_fr_CA_alexa

#define MAX_SIZE_CMD 5

#define AMZ_WW_SELFWAKE_DELAY_MS (500)

typedef struct _amzn_ww_model_map
{
    char *model;
    const char *locale;
} amzn_ww_model_map;

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Wake Word Model Externs */
#ifdef AMZN_MODEL_U_1S_50k_de_DE_alexa
extern const char U_1S_50k_de_DE_alexa[];
#endif
#ifdef AMZN_MODEL_WS_250k_de_DE_alexa
extern const char WS_250k_de_DE_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_AU_alexa
extern const char U_1S_50k_en_AU_alexa[];
#endif
#ifdef AMZN_MODEL_WR_250k_en_AU_alexa
extern const char WR_250k_en_AU_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_CA_alexa
extern const char U_1S_50k_en_CA_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_GB_alexa
extern const char U_1S_50k_en_GB_alexa[];
#endif
#ifdef AMZN_MODEL_WR_250k_en_GB_alexa
extern const char WR_250k_en_GB_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_IN_alexa
extern const char U_1S_50k_en_IN_alexa[];
#endif
#ifdef AMZN_MODEL_WS_250k_en_IN_alexa
extern const char WS_250k_en_IN_alexa[];
#endif
#ifdef AMZN_MODEL_D_en_US_alexa
extern const char D_en_US_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_US_alexa
extern const char U_1S_50k_en_US_alexa[];
#endif
#ifdef AMZN_MODEL_WR_250k_en_US_alexa
extern const char WR_250k_en_US_alexa[];
#endif
#ifdef AMZN_MODEL_D_es_ES_alexa
extern const char D_es_ES_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_es_ES_alexa
extern const char U_1S_50k_es_ES_alexa[];
#endif
#ifdef AMZN_MODEL_U_250k_es_ES_alexa
extern const char U_250k_es_ES_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_es_MX_alexa
extern const char U_1S_50k_es_MX_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_fr_CA_alexa
extern const char U_1S_50k_fr_CA_alexa[];
#endif
#ifdef AMZN_MODEL_WR_250k_fr_CA_alexa
extern const char WR_250k_fr_CA_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_fr_FR_alexa
extern const char U_1S_50k_fr_FR_alexa[];
#endif
#ifdef AMZN_MODEL_WR_250k_fr_FR_alexa
extern const char WR_250k_fr_FR_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_it_IT_alexa
extern const char U_1S_50k_it_IT_alexa[];
#endif
#ifdef AMZN_MODEL_WS_250k_it_IT_alexa
extern const char WS_250k_it_IT_alexa[];
#endif
#ifdef AMZN_MODEL_U_1S_50k_ja_JP_alexa
extern const char U_1S_50k_ja_JP_alexa[];
#endif
#ifdef AMZN_MODEL_WS_250k_ja_JP_alexa
extern const char WS_250k_ja_JP_alexa[];
#endif
#ifdef AMZN_MODEL_D_pt_BR_alexa
extern const char D_pt_BR_alexa[];
#endif
#ifdef AMZN_MODEL_WR_250k_pt_BR_alexa
extern const char WR_250k_pt_BR_alexa[];
#endif

/* Wake Word Model Externs Length */
#ifdef AMZN_MODEL_U_1S_50k_de_DE_alexa
extern const int U_1S_50k_de_DE_alexaLen;
#endif
#ifdef AMZN_MODEL_WS_250k_de_DE_alexa
extern const int WS_250k_de_DE_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_AU_alexa
extern const int U_1S_50k_en_AU_alexaLen;
#endif
#ifdef AMZN_MODEL_WR_250k_en_AU_alexa
extern const int WR_250k_en_AU_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_CA_alexa
extern const int U_1S_50k_en_CA_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_GB_alexa
extern const int U_1S_50k_en_GB_alexaLen;
#endif
#ifdef AMZN_MODEL_WR_250k_en_GB_alexa
extern const int WR_250k_en_GB_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_IN_alexa
extern const int U_1S_50k_en_IN_alexaLen;
#endif
#ifdef AMZN_MODEL_WS_250k_en_IN_alexa
extern const int WS_250k_en_IN_alexaLen;
#endif
#ifdef AMZN_MODEL_D_en_US_alexa
extern const int D_en_US_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_en_US_alexa
extern const int U_1S_50k_en_US_alexaLen;
#endif
#ifdef AMZN_MODEL_WR_250k_en_US_alexa
extern const int WR_250k_en_US_alexaLen;
#endif
#ifdef AMZN_MODEL_D_es_ES_alexa
extern const int D_es_ES_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_es_ES_alexa
extern const int U_1S_50k_es_ES_alexaLen;
#endif
#ifdef AMZN_MODEL_U_250k_es_ES_alexa
extern const int U_250k_es_ES_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_es_MX_alexa
extern const int U_1S_50k_es_MX_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_fr_CA_alexa
extern const int U_1S_50k_fr_CA_alexaLen;
#endif
#ifdef AMZN_MODEL_WR_250k_fr_CA_alexa
extern const int WR_250k_fr_CA_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_fr_FR_alexa
extern const int U_1S_50k_fr_FR_alexaLen;
#endif
#ifdef AMZN_MODEL_WR_250k_fr_FR_alexa
extern const int WR_250k_fr_FR_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_it_IT_alexa
extern const int U_1S_50k_it_IT_alexaLen;
#endif
#ifdef AMZN_MODEL_WS_250k_it_IT_alexa
extern const int WS_250k_it_IT_alexaLen;
#endif
#ifdef AMZN_MODEL_U_1S_50k_ja_JP_alexa
extern const int U_1S_50k_ja_JP_alexaLen;
#endif
#ifdef AMZN_MODEL_WS_250k_ja_JP_alexa
extern const int WS_250k_ja_JP_alexaLen;
#endif
#ifdef AMZN_MODEL_D_pt_BR_alexa
extern const int D_pt_BR_alexaLen;
#endif
#ifdef AMZN_MODEL_WR_250k_pt_BR_alexa
extern const int WR_250k_pt_BR_alexaLen;
#endif

struct ww_model_cmd
{
    char cmd[MAX_SIZE_CMD];
    char lang_code[AMZ_WW_MODEL_LENGTH];
};

enum
{
    WW_MODEL_SUCCESS       = 0,
    WW_MODEL_NULL          = 1,
    WW_MODEL_NOT_SUPPORTED = 2,
    WW_MODEL_FAILED        = 3,
};
typedef struct ww_model_cmd ww_model_cmd_t;
/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Passes audio data to the wake word to be processed
 *
 * @param *pi16AudioBuff Audio sample to be fed to the wake word
 * @param *u16BufferSize The size of the sample of audio
 *
 * @returns Status of whether the model string is supported
 *
 */
uint32_t SLN_AMAZON_WAKE_ProcessWakeWord(int16_t *pi16AudioBuff, uint16_t u16BufferSize);

/*!
 * @brief Passes audio data to the self wake word to be processed
 *
 * @param *pi16AudioBuff Internal audio sample to be fed to the self wake word
 * @param *u16BufferSize The size of the sample of audio
 *
 * @returns Status of whether the model string is supported
 *
 */
uint32_t SLN_AMAZON_WAKE_ProcessWakeWordSelfWake(int16_t *pi16AudioBuff, uint16_t u16BufferSize);

/*!
 * @brief Initializes the Amazon Wake word
 *
 * @returns Status of whether the wake word was initialized
 *
 */
uint32_t SLN_AMAZON_WAKE_Initialize();

/*!
 * @brief Destroys the Amazon Wake word context
 *
 * @returns Status of whether the wake word was destroyed
 *
 */
uint32_t SLN_AMAZON_WAKE_Destroy();

/*!
 * @brief Configures the pointers to be modified upon wake and length update
 *
 * @param *pu8Wake Pointer to the wake word trigger variable
 * @param *pu16WWLen Pointer to the wake word offset start
 *
 * @returns Status of whether the model string is supported
 *
 */
void SLN_AMAZON_WAKE_SetWakeupDetectedParams(uint8_t *pu8Wake, uint16_t *pu16WWLen);

/*!
 * @brief Checks to see if the string local is supported
 *
 * @param *lang_code Checks to see if the locale string is supported
 *
 * @returns Status of whether the model string is supported
 *
 */
uint32_t SLN_AMAZON_WAKE_IsWakeWordSupported(char *lang_code);

/*!
 * @brief Get's the address Supported Locales array
 *
 * @returns The address of the locale support array
 *
 */
amzn_ww_model_map *SLN_AMAZON_GetSupportedLocales(void);

/*!
 * @brief Sets the Amazon Model Locale string to be used upon reset
 *
 * @param *modelLocale String of the wake word locale to be saved
 *
 * @returns Status of model locale string flash save
 *
 */
uint32_t SLN_AMAZON_WAKE_SetModelLocale(char *modelLocale);

/*!
 * @brief Gets the Amazon Model Locale string from Flash
 *
 * @param *modelLocale String of the wake word locale to be returned
 *
 */
void SLN_AMAZON_WAKE_GetModelLocale(uint8_t *modelLocal);

/*!
 * @brief Retrieves the Amazon Model Locale size
 *
 * @returns Size of the Amazon Wake Word Locale String
 *
 */
uint32_t SLN_AMAZON_WAKE_GetModelLocaleSize();

/*!
 * @brief Retrieves the Amazon Locale String
 *
 * @returns Amazon Wake Word Locale String
 *
 */
char * SLN_AMAZON_WAKE_GetLocaleString();

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _AMAZON_WAKE_WORD_H_ */

/*******************************************************************************
 * API
 ******************************************************************************/

