/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/* Board includes */
#include "board.h"
#include "pin_mux.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "iot_logging_task.h"

/* ACS includes */
#include "app_init.h"

/* AIS includes */
#include "aisv2.h"
#include "ais_buttons.h"
#include "ais_alerts.h"

/* AWS includes */
#include "aws_clientcredential.h"

/* Wake word includes */
#include "amazon_wake_word.h"

/* Driver includes */
#include "fsl_dmamux.h"
#include "fsl_edma.h"
#include "fsl_trng.h"
#include "sln_RT10xx_RGB_LED_driver.h"

/* Button includes */
#include "switch.h"

/* Shell includes */
#include "sln_shell.h"

/* WiFi includes */
#include "wifi_credentials.h"

/* Decimation includes */
#include "pdm_to_pcm_task.h"

/* MbedTLS includes */
#include "ksdk_mbedtls.h"

/* Required last to have defined all members of g_fileTable */
#include "sln_cfg_file.h"
#include "sln_file_table.h"

#include "clock_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief LOGGING Task settings */
#define LOGGING_TASK_STACK_SIZE   512
#define LOGGING_TASK_PRIORITY     tskIDLE_PRIORITY + 2
#define LOGGING_TASK_QUEUE_LENGTH 64

/*! @brief SLN SHELL Task settings */
#define SLN_SHELL_TASK_NAME       "Shell_Task"
#define SLN_SHELL_TASK_STACK_SIZE 512
#define SLN_SHELL_TASK_PRIORITY   tskIDLE_PRIORITY + 1

/*! @brief BUTTON Task settings */
#define BUTTON_TASK_NAME       "Button_Task"
#define BUTTON_TASK_STACK_SIZE 1024
#define BUTTON_TASK_PRIORITY   configMAX_PRIORITIES - 1

/*! @brief OTA Task settings */
#define OTA_TASK_NAME       "OTA_Task"
#define OTA_TASK_STACK_SIZE 384
#define OTA_TASK_PRIORITY   tskIDLE_PRIORITY + 1

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

__attribute__((section(".ocram_non_cacheable_bss"))) StackType_t shell_task_stack_buffer[SLN_SHELL_TASK_STACK_SIZE];
__attribute__((section(".ocram_non_cacheable_bss"))) StaticTask_t sln_shell_task_buffer;

__attribute__((section(".ocram_non_cacheable_bss"))) StackType_t button_task_stack_buffer[BUTTON_TASK_STACK_SIZE];
__attribute__((section(".ocram_non_cacheable_bss"))) StaticTask_t button_task_buffer;

__attribute__((section(".ocram_non_cacheable_bss"))) StackType_t ota_task_stack_buffer[OTA_TASK_STACK_SIZE];
__attribute__((section(".ocram_non_cacheable_bss"))) StaticTask_t ota_task_buffer;

extern uintptr_t _g_global_flash_offset;

/*******************************************************************************
 * Code
 ******************************************************************************/

void SysTick_DelayTicks(uint32_t n)
{
    vTaskDelay(n);
}

static void vendor_startup_init(void)
{
    char random_data[32];

    /* Cryptographic hardware modules initialization */
    CRYPTO_InitHardware();

    /* Dummy random generation performed here; No idea why, but first random generation
     * fails and, if a TRNG operation is not tried earlier, kvs initialization fails */
    TRNG_GetRandomData(((TRNG_Type *)TRNG_BASE), random_data, sizeof(random_data));

    /* Initialize Flash to allow writing */
    SLN_Flash_Init();

    /* Initialize global flash base address variable utilized by KVS macros */
    _g_global_flash_offset = FlexSPI_AMBA_BASE;

    /*
     * AUDIO PLL setting: Frequency = Fref * (DIV_SELECT + NUM / DENOM)
     *                              = 24 * (32 + 77/100)
     *                              = 786.48 MHz
     */
    const clock_audio_pll_config_t audioPllConfig = {
        .loopDivider = 32, /* PLL loop divider. Valid range for DIV_SELECT divider value: 27~54. */
        .postDivider = 1,  /* Divider after the PLL, should only be 1, 2, 4, 8, 16. */
#if USE_TFA
        .numerator   = 77,  /* 30 bit numerator of fractional loop divider. */
        .denominator = 100, /* 30 bit denominator of fractional loop divider */
#elif USE_MQS
        .numerator   = 768,  /* 30 bit numerator of fractional loop divider. */
        .denominator = 1000, /* 30 bit denominator of fractional loop divider */
#endif /* USE_TFA */
    };

    CLOCK_InitAudioPll(&audioPllConfig);

    CLOCK_SetMux(kCLOCK_Sai1Mux, BOARD_PDM_SAI_CLOCK_SOURCE_SELECT);
    CLOCK_SetDiv(kCLOCK_Sai1PreDiv, BOARD_PDM_SAI_CLOCK_SOURCE_PRE_DIVIDER);
    CLOCK_SetDiv(kCLOCK_Sai1Div, BOARD_PDM_SAI_CLOCK_SOURCE_DIVIDER);
    CLOCK_EnableClock(kCLOCK_Sai1);

    CLOCK_SetMux(kCLOCK_Sai2Mux, BOARD_PDM_SAI_CLOCK_SOURCE_SELECT);
    CLOCK_SetDiv(kCLOCK_Sai2PreDiv, BOARD_PDM_SAI_CLOCK_SOURCE_PRE_DIVIDER);
    CLOCK_SetDiv(kCLOCK_Sai2Div, BOARD_PDM_SAI_CLOCK_SOURCE_DIVIDER);
    CLOCK_EnableClock(kCLOCK_Sai2);

    edma_config_t dmaConfig = {0};

    /* Create EDMA handle */
    /*
     * dmaConfig.enableRoundRobinArbitration = false;
     * dmaConfig.enableHaltOnError = true;
     * dmaConfig.enableContinuousLinkMode = false;
     * dmaConfig.enableDebugMode = false;
     */
    EDMA_GetDefaultConfig(&dmaConfig);
    EDMA_Init(DMA0, &dmaConfig);

    DMAMUX_Init(DMAMUX);

    /* LED, switch modules initialization */
    RGB_LED_Init();
    switchInit();
}

/**
 * @brief Application runtime entry point.
 */
static void demo_init(void)
{
    /* Set flash management callbacks */
    sln_flash_mgmt_cbs_t flash_mgmt_cbs = {pdm_to_pcm_mics_off, pdm_to_pcm_mics_on};
    SLN_FLASH_MGMT_SetCbs(&flash_mgmt_cbs);

    /* Initialize flash management */
    SLN_FLASH_MGMT_Init((sln_flash_entry_t *)g_fileTable, false);

    /* USB CDC console based shell initialization */
    sln_shell_init();

    /* Tasks creation */
    xLoggingTaskInitialize(LOGGING_TASK_STACK_SIZE, LOGGING_TASK_PRIORITY, LOGGING_TASK_QUEUE_LENGTH);

    xTaskCreateStatic(sln_shell_task, SLN_SHELL_TASK_NAME, SLN_SHELL_TASK_STACK_SIZE, NULL, SLN_SHELL_TASK_PRIORITY,
                      shell_task_stack_buffer, &sln_shell_task_buffer);

    xTaskCreateStatic(button_task, BUTTON_TASK_NAME, BUTTON_TASK_STACK_SIZE, NULL, BUTTON_TASK_PRIORITY,
                      button_task_stack_buffer, &button_task_buffer);

    xTaskCreateStatic(ota_task, OTA_TASK_NAME, OTA_TASK_STACK_SIZE, NULL, OTA_TASK_PRIORITY, ota_task_stack_buffer,
                      &ota_task_buffer);

    if (create_main_task() != 0)
    {
        configPRINTF(("create main task failed\n"));
    }
}

/*!
 * @brief Main function
 */
void main(void)
{
    /* Enable additional fault handlers */
    SCB->SHCSR |= (SCB_SHCSR_BUSFAULTENA_Msk | /*SCB_SHCSR_USGFAULTENA_Msk |*/ SCB_SHCSR_MEMFAULTENA_Msk);

    /* Relocate Vector Table */
#if RELOCATE_VECTOR_TABLE
    BOARD_RelocateVectorTableToRam();
#endif

    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_BootClockRUN();

    /* Other hardware modules initialization */
    vendor_startup_init();

    /* ACS early initialization */
    system_early_init();

    /* Demo initialization */
    demo_init();

    /* Run RTOS */
    vTaskStartScheduler();

    /* Should not reach this statement */
    while (1)
    {
    }
}
