/*
 * Copyright 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "iot_logging_task.h"

/* ACS includes */
#include "app_init.h"

/* Driver includes */
#include "fsl_trng.h"
#include "fsl_pit.h"

/* MbedTLS includes */
#include "ksdk_mbedtls.h"

/* App includes */
#include "sln_flash.h"

/* Board includes */
#include "pin_mux.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#if SERIAL_PORT_TYPE_USBCDC
#include "usb_device_config.h"
#include "usb_phy.h"
#include "serial_port_usb.h"
#endif
/*******************************************************************************
 * Definitions
 ******************************************************************************/

#if SERIAL_PORT_TYPE_USBCDC
#define CONTROLLER_ID kSerialManager_UsbControllerEhci0
#endif

/* Logging Task Defines. */
#define mainLOGGING_MESSAGE_QUEUE_LENGTH (32)
#define mainLOGGING_TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE * 6)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/* Global timer IRQ handle */
extern void timer_IRQHandler(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/

extern uintptr_t _g_global_flash_offset;

/*******************************************************************************
 * Code
 ******************************************************************************/

#if SERIAL_PORT_TYPE_USBCDC
static void USB_DeviceClockInit(void)
{
#if defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U)
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };
#endif
#if defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U)
    if (CONTROLLER_ID == kSerialManager_UsbControllerEhci0)
    {
        CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 480000000U);
    }
    else
    {
        CLOCK_EnableUsbhs1PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
        CLOCK_EnableUsbhs1Clock(kCLOCK_Usb480M, 480000000U);
    }
    USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
#endif
}
#endif


void SysTick_DelayTicks(uint32_t n)
{
    vTaskDelay(n);
}

/* Note: The PIT_ClearStatusFlags is called in timer_IRQHandler */
void PIT_IRQHandler(void)
{
    /* kPIT_Chnl_1 IRQ is used by the iot timer */
    if (PIT_GetStatusFlags(PIT, kPIT_Chnl_1) == kPIT_TimerFlag)
    {
        timer_IRQHandler();
    }
}

void HardFault_Handler(void)
{
    __asm("BKPT #0");
}

void vApplicationDaemonTaskStartupHook(void)
{
    /* Nothing to do here */
}

char *strdupFreeRTOSPort(const char *s)
{
    char *buff = NULL;

    if (s)
    {
        size_t bufsize = strlen(s) + 1;
        buff           = pvPortMalloc(bufsize);
        if (buff)
        {
            memcpy(buff, s, bufsize);
        }
    }

    return buff;
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
}

/**
 * @brief Application runtime entry point.
 */
static void demo_init(void)
{
    xLoggingTaskInitialize(mainLOGGING_TASK_STACK_SIZE, tskIDLE_PRIORITY + 5, mainLOGGING_MESSAGE_QUEUE_LENGTH);

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

#if SERIAL_PORT_TYPE_USBCDC
    USB_DeviceClockInit();
    DbgConsole_Init(kSerialManager_UsbControllerEhci0, 0, kSerialPort_UsbCdc, 0);
#elif SERIAL_PORT_TYPE_UART
    uint32_t uartClkSrcFreq = BOARD_DebugConsoleSrcFreq();
    DbgConsole_Init(BOARD_DEBUG_UART_INSTANCE, BOARD_DEBUG_UART_BAUDRATE, BOARD_DEBUG_UART_TYPE, uartClkSrcFreq);
#endif

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
