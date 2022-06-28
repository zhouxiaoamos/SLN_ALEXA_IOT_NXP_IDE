/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "FreeRTOS.h"
#include "stddef.h"
#include "avs_sound_library.h"
#include "avs_sound_library_prompts.h"
#include "avs_support_locale_config.h"

#if AVS_LOCALE_DE_DE_SUPPORT == 1
#include "avs_sound_library_locale_de_DE.h"
#endif
#if AVS_LOCALE_EN_AU_SUPPORT == 1
#include "avs_sound_library_locale_en_AU.h"
#endif
#if AVS_LOCALE_EN_CA_SUPPORT == 1
#include "avs_sound_library_locale_en_CA.h"
#endif
#if AVS_LOCALE_EN_GB_SUPPORT == 1
#include "avs_sound_library_locale_en_GB.h"
#endif
#if AVS_LOCALE_EN_IN_SUPPORT == 1
#include "avs_sound_library_locale_en_IN.h"
#endif
#if AVS_LOCALE_EN_US_SUPPORT == 1
#include "avs_sound_library_locale_en_US.h"
#endif
#if AVS_LOCALE_ES_ES_SUPPORT == 1
#include "avs_sound_library_locale_es_ES.h"
#endif
#if AVS_LOCALE_ES_MX_SUPPORT == 1
#include "avs_sound_library_locale_es_MX.h"
#endif
#if AVS_LOCALE_ES_US_SUPPORT == 1
#include "avs_sound_library_locale_es_US.h"
#endif
#if AVS_LOCALE_FR_CA_SUPPORT == 1
#include "avs_sound_library_locale_fr_CA.h"
#endif
#if AVS_LOCALE_FR_FR_SUPPORT == 1
#include "avs_sound_library_locale_fr_FR.h"
#endif
#if AVS_LOCALE_HI_IN_SUPPORT == 1
#include "avs_sound_library_locale_hi_IN.h"
#endif
#if AVS_LOCALE_IT_IT_SUPPORT == 1
#include "avs_sound_library_locale_it_IT.h"
#endif
#if AVS_LOCALE_JA_JP_SUPPORT == 1
#include "avs_sound_library_locale_ja_JP.h"
#endif
#if AVS_LOCALE_PT_BR_SUPPORT == 1
#include "avs_sound_library_locale_pt_BR.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Global Vars
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

void AVS_SOUNDS_Init(avs_sound_library_t *sounds)
{
    if (sounds->sound_files == NULL)
    {
        sounds->sound_files = pvPortMalloc(sizeof(avs_sound_files_t));
    }
}

void AVS_SOUNDS_Deinit(avs_sound_library_t *sounds)
{
    if (sounds->sound_files != NULL)
    {
        pvPortFree(sounds->sound_files);
        sounds->sound_files = NULL;
    }

    sounds->sound_sizes = NULL;
}

void AVS_SOUNDS_LoadSounds(char *locale, avs_sound_library_t *sounds)
{
    if (sounds->sound_files == NULL)
    {
        sounds->sound_files = pvPortMalloc(sizeof(avs_sound_files_t));
    }

    if (sounds->core_sound_files == NULL)
    {
        sounds->core_sound_files = pvPortMalloc(sizeof(avs_sound_core_files_t));
    }

    /* Load System Sounds */
    AVS_SOUNDS_Load_System_Sounds(sounds);

#if AVS_LOCALE_DE_DE_SUPPORT == 1
    if (0 == strncmp(locale, "de_DE", 6))
    {
         AVS_SOUNDS_Load_de_DE_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_EN_AU_SUPPORT == 1
    if (0 == strncmp(locale, "en_AU", 6))
    {
         AVS_SOUNDS_Load_en_AU_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_EN_CA_SUPPORT == 1
    if (0 == strncmp(locale, "en_CA", 6))
    {
         AVS_SOUNDS_Load_en_CA_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_EN_GB_SUPPORT == 1
    if (0 == strncmp(locale, "en_GB", 6))
    {
         AVS_SOUNDS_Load_en_GB_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_EN_IN_SUPPORT == 1
    if (0 == strncmp(locale, "en_IN", 6))
    {
         AVS_SOUNDS_Load_en_IN_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_EN_US_SUPPORT == 1
    if (0 == strncmp(locale, "en_US", 6))
    {
         AVS_SOUNDS_Load_en_US_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_ES_ES_SUPPORT == 1
    if (0 == strncmp(locale, "es_ES", 6))
    {
         AVS_SOUNDS_Load_es_ES_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_ES_MX_SUPPORT == 1
    if (0 == strncmp(locale, "es_MX", 6))
    {
         AVS_SOUNDS_Load_es_MX_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_ES_US_SUPPORT == 1
    if (0 == strncmp(locale, "es_US", 6))
    {
         AVS_SOUNDS_Load_es_US_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_FR_CA_SUPPORT == 1
    if (0 == strncmp(locale, "fr_CA", 6))
    {
         AVS_SOUNDS_Load_fr_CA_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_FR_FR_SUPPORT == 1
    if (0 == strncmp(locale, "fr_FR", 6))
    {
         AVS_SOUNDS_Load_fr_FR_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_HI_IN_SUPPORT == 1
    if (0 == strncmp(locale, "hi_IN", 6))
    {
         AVS_SOUNDS_Load_hi_IN_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_IT_IT_SUPPORT == 1
    if (0 == strncmp(locale, "it_IT", 6))
    {
         AVS_SOUNDS_Load_it_IT_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_JA_JP_SUPPORT == 1
    if (0 == strncmp(locale, "ja_JP", 6))
    {
         AVS_SOUNDS_Load_ja_JP_Sounds(locale, sounds);
    }
#endif

#if AVS_LOCALE_PT_BR_SUPPORT == 1
    if (0 == strncmp(locale, "pt_BR", 6))
    {
         AVS_SOUNDS_Load_pt_BR_Sounds(locale, sounds);
    }
#endif

}

