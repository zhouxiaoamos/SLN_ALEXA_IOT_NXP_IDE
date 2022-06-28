/*
 * Copyright 2018-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "sln_shell.h"
#include "wifi_credentials.h"

/* Device specific includes */
#include "device_utils.h"

#include "sln_flash_mgmt.h"
#include "sln_cfg_file.h"
#include "perf.h"

#ifdef FFS_ENABLED
#include "ffs_provision_cli.h"
#endif /* FFS_ENABLED */

#include "sln_reset.h"
#include "amazon_wake_word.h"

#include "app_events.h"
#include "reconnection_task.h"
#include "aisv2.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#if (defined(SERIAL_MANAGER_NON_BLOCKING_MODE) && (SERIAL_MANAGER_NON_BLOCKING_MODE > 0U))
#define SLN_SERIAL_MANAGER_RECEIVE_BUFFER_LEN 2048U
#endif

#define USB_LOG_OFF 0
#define USB_LOG_ON  1

#define LOG_ERROR_PRINT "Error: failed to print a message\r\n"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
extern void *pvPortCalloc(size_t nmemb, size_t xSize);
// extern shell_command_t g_shellCommandexit;

#if USE_WIFI_CONNECTION
static shell_status_t sln_print_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_erase_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_setup_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
#endif /* USE_WIFI_CONNECTION */
static shell_status_t sln_reset_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_factory_reset_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_mem_view_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_enable_heap_trace_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_disable_heap_trace_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_enable_usb_logging_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_disable_usb_logging_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_logs_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_serial_number_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_faultlog_statusget_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_faultlog_statuserase_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_version_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_ww_model_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_heap_view_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t sln_tasks_stack_view_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
#ifdef SLN_TRACE_CPU_USAGE
static shell_status_t sln_trace_cpu_usage_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
#endif /* SLN_TRACE_CPU_USAGE */
#ifdef FFS_ENABLED
static shell_status_t sln_ffs_provision_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
#endif /* FFS_ENABLED */

static uint8_t usb_logging_flag_flash_set(uint8_t value);
static uint8_t usb_logging_flag_flash_get(void);
static void log_history_print(void);
static void sln_shell_locales_changed_event(char *locale);

/*******************************************************************************
 * Variables
 ******************************************************************************/
#if USE_WIFI_CONNECTION
SHELL_COMMAND_DEFINE(print,
                     "\r\n\"print\": Print the WiFi Network Credentials currently stored in flash\r\n",
                     sln_print_handler,
                     0);

SHELL_COMMAND_DEFINE(erase,
                     "\r\n\"erase\": Erase the current WiFi Network credentials from flash\r\n",
                     sln_erase_handler,
                     0);

SHELL_COMMAND_DEFINE(setup,
                     "\r\n\"setup\": Setup the WiFi Network Credentials\r\n"
                     "         Usage:\r\n"
                     "             setup SSID [PASSWORD] \r\n"
                     "         Parameters:\r\n"
                     "             SSID:       The wireless network name\r\n"
                     "             PASSWORD:   The password for the wireless network\r\n"
                     "                         For open networks it is not needed\r\n",
                     /* if more than two parameters, it'll take just the first two of them */
                     sln_setup_handler,
                     SHELL_IGNORE_PARAMETER_COUNT);
#endif
SHELL_COMMAND_DEFINE(reset, "\r\n\"reset\": Resets the MCU\r\n", sln_reset_handler, 0);

SHELL_COMMAND_DEFINE(factory_reset,
                     "\r\n\"factory_reset\": Performs a Factory Reset of the device\r\n",
                     sln_factory_reset_handler,
                     0);

SHELL_COMMAND_DEFINE(mem_view, "\r\n\"mem_view\": View available FreeRTOS heap.\r\n", sln_mem_view_handler, 0);

SHELL_COMMAND_DEFINE(enable_heap_trace,
                     "\r\n\"enable_heap_trace\": Enable FreeRTOS heap trace.\r\n"
                     "         Usage:\r\n"
                     "             enable_heap_trace THRESHOLD\r\n"
                     "         Parameters:\r\n"
                     "             THRESHOLD:   Minimum allocation size to trace.\r\n",
                     sln_enable_heap_trace_handler,
                     1);

SHELL_COMMAND_DEFINE(disable_heap_trace,
                     "\r\n\"disable_heap_trace\": Disable FreeRTOS heap trace.\r\n",
                     sln_disable_heap_trace_handler,
                     0);

SHELL_COMMAND_DEFINE(enable_usb_log,
                     "\r\n\"enable_usb_log\": Enable logging via USB.\r\n",
                     sln_enable_usb_logging_handler,
                     0);

SHELL_COMMAND_DEFINE(disable_usb_log,
                     "\r\n\"disable_usb_log\": Disable logging via USB.\r\n",
                     sln_disable_usb_logging_handler,
                     0);

SHELL_COMMAND_DEFINE(logs, "\r\n\"logs\": Print logs history.\r\n", sln_logs_handler, 0);

SHELL_COMMAND_DEFINE(serial_number,
                     "\r\n\"serial_number\": Print the device serial number\r\n",
                     sln_serial_number_handler,
                     0);

SHELL_COMMAND_DEFINE(faultlog_statusget,
                     "\r\n\"faultlog_statusget\": Print the fault log status registers from flash\r\n",
                     sln_faultlog_statusget_handler,
                     0);

SHELL_COMMAND_DEFINE(faultlog_statuserase,
                     "\r\n\"faultlog_statuserase\": Erase the fault log status registers from flash\r\n",
                     sln_faultlog_statuserase_handler,
                     0);

SHELL_COMMAND_DEFINE(version, "\r\n\"version\": Print firmware version\r\n", sln_version_handler, 0);

SHELL_COMMAND_DEFINE(ww_model,
                     "\r\n\"ww_model\": Setup the wake word model language\r\n"
                     "         Usage:\r\n"
                     "             ww_model set en-US      set the language model setting\r\n"
                     "             ww_model show           show current language model setting\r\n"
                     "\r\n"
                     "         Language list:\r\n" AMZN_MODEL_SUPPORTED_LOCALES,
                     sln_ww_model_handler,
                     SHELL_IGNORE_PARAMETER_COUNT);
SHELL_COMMAND_DEFINE(heap_view, "\r\n\"heap_view\": Print FreeRTOS heap consumptions\r\n", sln_heap_view_handler, 0);
SHELL_COMMAND_DEFINE(stacks_view,
                     "\r\n\"stacks_view\": Print FreeRTOS stacks consumptions\r\n",
                     sln_tasks_stack_view_handler,
                     0);

#ifdef SLN_TRACE_CPU_USAGE
SHELL_COMMAND_DEFINE(cpu_view, "\r\n\"cpu_view\": Print the CPU usage info\r\n", sln_trace_cpu_usage_handler, 0);
#endif /* SLN_TRACE_CPU_USAGE */

#ifdef FFS_ENABLED
SHELL_COMMAND_DEFINE(
    ffs_provision,
    "\r\n\"ffs_provision\": Configure the FFS provisioning process\r\n"
    "         Usage:\r\n"
    "             ffs_provision ready\r\n\r\n"
    "             ffs_provision start\r\n"
    "             ffs_provision stop\r\n\r\n"
    "             ffs_provision param_begin [param size]\r\n"
    "             ffs_provision param_chunk [param chunk]\r\n"
    "             ffs_provision param_newline\r\n\r\n"
    "             ffs_provision device_info_get [dsn/type/ffs_pid/ffs_pin/bt_name/dss_pub_key/client_id]\r\n"
    "             ffs_provision device_info_set [dsn/type/ffs_pid/ffs_pin/bt_name/dss_pub_key/client_id]\r\n\r\n"
    "             ffs_provision dha_get_field   [dsn/soc_id/csr/crt_chain/leaf_crt]\r\n"
    "             ffs_provision dha_keygen\r\n"
    "             ffs_provision dha_cert_set\r\n\r\n",
    sln_ffs_provision_handler,
    SHELL_IGNORE_PARAMETER_COUNT);
#endif /* FFS_ENABLED */

static uint8_t s_shellHandleBuffer[SHELL_HANDLE_SIZE];
static shell_handle_t s_shellHandle;

static uint8_t s_serialHandleBuffer[SERIAL_MANAGER_HANDLE_SIZE];
static serial_handle_t s_serialHandle = &s_serialHandleBuffer[0];

#if (defined(SERIAL_MANAGER_NON_BLOCKING_MODE) && (SERIAL_MANAGER_NON_BLOCKING_MODE > 0U))
__attribute__((section(".ocram_non_cacheable_bss"))) __attribute__((aligned(8)))
uint8_t readRingBuffer[SLN_SERIAL_MANAGER_RECEIVE_BUFFER_LEN];
#endif

static EventGroupHandle_t s_ShellEventGroup;
#if USE_WIFI_CONNECTION
static wifi_cred_t s_wifi_cred;
#endif

static ww_model_cmd_t ww_model_cmd;

static TaskHandle_t s_appInitTask;
static shell_heap_trace_t s_heap_trace;
static volatile uint8_t s_usb_logging     = USB_LOG_OFF;
static volatile uint8_t s_log_history_pos = -1;
__attribute__((
    section(".ocram_non_cacheable_bss"))) static char log_history[MAX_LOG_HISTORY][configLOGGING_MAX_MESSAGE_LENGTH];

/* Through these queue we communicate with the app task */
static QueueHandle_t s_appEventQueue = NULL;

/*******************************************************************************
 * Code
 ******************************************************************************/

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

static shell_status_t sln_ww_model_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    char *commandName = NULL;
    char *regionCode  = NULL;

    if (argc == 1 || argc > 3)
    {
        SHELL_Printf(
            s_shellHandle,
            "\r\nIncorrect command parameter(s).  Enter \"help\" to view a list of available commands.\r\n\r\n");
        return kStatus_SHELL_Error;
    }

    commandName = argv[1];

    if (argc > 2)
    {
        regionCode = argv[2];
    }

    uint32_t name_len = strlen(commandName);
    uint32_t code_len = regionCode ? strlen(regionCode) : 0;

    if (name_len + 1 <= sizeof("show"))
    {
        strncpy(ww_model_cmd.cmd, commandName, name_len + 1);
    }

    else
    {
        return kStatus_SHELL_Error;
    }
    if (code_len + 1 <= AMZ_WW_MODEL_LENGTH)
    {
        if (code_len != 0)
        {
            strncpy(ww_model_cmd.lang_code, regionCode, AMZ_WW_MODEL_LENGTH);
        }
        else
        {
            ww_model_cmd.lang_code[0] = '\0';
        }
    }
    else
    {
        return kStatus_SHELL_Error;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, WW_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

#if USE_WIFI_CONNECTION
static shell_status_t sln_print_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, PRINT_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_erase_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, ERASE_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_setup_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    if (argc == 1 || argc > 3)
    {
        SHELL_Printf(
            s_shellHandle,
            "\r\nIncorrect command parameter(s).  Enter \"help\" to view a list of available commands.\r\n\r\n");
        return kStatus_SHELL_Error;
    }

    char *kWiFiName     = NULL;
    char *kWiFiPassword = NULL;

    kWiFiName = argv[1];
    if (argc > 2)
    {
        kWiFiPassword = argv[2];
    }

    uint32_t name_len = strlen(kWiFiName);
    uint32_t pass_len = kWiFiPassword ? strlen(kWiFiPassword) : 0;

    if (name_len == 0)
    {
        SHELL_Printf(
            s_shellHandle,
            "\r\nIncorrect command parameter(s).  Enter \"help\" to view a list of available commands.\r\n\r\n");
        return kStatus_SHELL_Error;
    }

    if (name_len <= sizeof(s_wifi_cred.ssid.value))
    {
        memcpy(s_wifi_cred.ssid.value, kWiFiName, name_len);
        s_wifi_cred.ssid.length = name_len;
    }
    else
    {
        return kStatus_SHELL_Error;
    }

    if (pass_len + 1 <= sizeof(s_wifi_cred.password.value))
    {
        if (pass_len != 0)
        {
            memcpy(s_wifi_cred.password.value, kWiFiPassword, pass_len + 1);
        }
        else
        {
            s_wifi_cred.password.value[0] = '\0';
        }
        s_wifi_cred.password.length = pass_len;
    }
    else
    {
        return kStatus_SHELL_Error;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, SETUP_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}
#endif /* USE_WIFI_CONNECTION */

static shell_status_t sln_reset_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, RESET_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_factory_reset_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, FACTORY_RESET_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_mem_view_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, MEM_VIEW_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_enable_heap_trace_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (argc == 1 || argc > 2)
    {
        SHELL_Printf(
            s_shellHandle,
            "\r\nIncorrect command parameter(s).  Enter \"help\" to view a list of available commands.\r\n\r\n");
        return kStatus_SHELL_Error;
    }

    char *threshold = NULL;
    threshold       = argv[1];

    if (6 == safe_strlen(threshold, 6))
    {
        SHELL_Printf(
            s_shellHandle,
            "\r\nIncorrect command parameter(s).  Enter \"help\" to view a list of available commands.\r\n\r\n");
        return kStatus_SHELL_Error;
    }

    s_heap_trace.threshold = atoi(threshold);

    xEventGroupSetBitsFromISR(s_ShellEventGroup, ENA_HEAP_TRACE_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_disable_heap_trace_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, DIS_HEAP_TRACE_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_enable_usb_logging_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, ENA_USB_LOG_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_disable_usb_logging_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, DIS_USB_LOG_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_logs_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, LOGS_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_serial_number_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, SERIAL_NUMBER_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_faultlog_statusget_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, FAULTLOG_STATUSGET_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_faultlog_statuserase_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, FAULTLOG_STATUSERASE_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_version_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, VERSION_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_heap_view_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, HEAP_VIEW_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}

static shell_status_t sln_tasks_stack_view_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, STACKS_VIEW_EVT, &xHigherPriorityTaskWoken);
    return kStatus_SHELL_Success;
}

#ifdef SLN_TRACE_CPU_USAGE
static shell_status_t sln_trace_cpu_usage_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(s_ShellEventGroup, TRACE_CPU_USAGE_EVT, &xHigherPriorityTaskWoken);

    return kStatus_SHELL_Success;
}
#endif /* SLN_TRACE_CPU_USAGE */

#ifdef FFS_ENABLED
static shell_status_t sln_ffs_provision_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    ffs_provision_status_t status;

    status = FFSPROVISION_register_command(argc, argv);
    if (status == kStatus_FFS_PROVISION_Success)
    {
        xEventGroupSetBitsFromISR(s_ShellEventGroup, FFS_PROVISION_EVT, &xHigherPriorityTaskWoken);
    }
    else
    {
        SHELL_Printf(s_shellHandle, "ffs_provision command fail registration: %d\r\n", status);
    }

    return kStatus_SHELL_Success;
}
#endif /* FFS_ENABLED */

static char *getAppType(void)
{
    /* Find the current running bank by checking the ResetISR Address in the vector table (which is loaded into
     * DTC) */
    uint32_t runningFromBankA =
        (((*(uint32_t *)(APPLICATION_RESET_ISR_ADDRESS)) & APP_VECTOR_TABLE_APP_A) == APP_VECTOR_TABLE_APP_A);
    uint32_t runningFromBankb =
        (((*(uint32_t *)(APPLICATION_RESET_ISR_ADDRESS)) & APP_VECTOR_TABLE_APP_B) == APP_VECTOR_TABLE_APP_B);

    if (runningFromBankA)
        return JSON_FILEPATH_APPA;
    if (runningFromBankb)
        return JSON_FILEPATH_APPB;
    return JSON_FILEPATH_APPUNK;
}

int sln_shell_init(void)
{
    status_t status = 0;
    serial_manager_config_t serialConfig;

    serial_port_usb_cdc_config_t usbCdcConfig = {
        .controllerIndex = (serial_port_usb_cdc_controller_index_t)CONTROLLER_ID,
    };

    s_ShellEventGroup = xEventGroupCreate();

    static volatile uint8_t usb_clock_initialized = 0;
    if (!usb_clock_initialized)
    {
        usb_clock_initialized = 1;
        USB_DeviceClockInit();
    }

    /* Init Serial Manager for USB CDC */
    serialConfig.type = kSerialPort_UsbCdc;
#if (defined(SERIAL_MANAGER_NON_BLOCKING_MODE) && (SERIAL_MANAGER_NON_BLOCKING_MODE > 0U))
    serialConfig.ringBuffer     = &readRingBuffer[0];
    serialConfig.ringBufferSize = SLN_SERIAL_MANAGER_RECEIVE_BUFFER_LEN;
#endif
    serialConfig.portConfig = &usbCdcConfig;

    status = SerialManager_Init(s_serialHandle, &serialConfig);
    if (status != kStatus_SerialManager_Success)
    {
        return (int32_t)status;
    }

    /* Init SHELL */
    s_shellHandle = &s_shellHandleBuffer[0];
    SHELL_Init(s_shellHandle, s_serialHandle, "SHELL>> ");

    /* Add the SLN commands to the commands list */
#if USE_WIFI_CONNECTION
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(print));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(setup));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(erase));
#endif /* USE_WIFI_CONNECTION */
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(reset));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(factory_reset));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(mem_view));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(enable_heap_trace));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(disable_heap_trace));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(enable_usb_log));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(disable_usb_log));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(logs));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(serial_number));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(faultlog_statusget));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(faultlog_statuserase));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(version));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(ww_model));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(heap_view));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(stacks_view));
#ifdef SLN_TRACE_CPU_USAGE
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(cpu_view));
#endif /* SLN_TRACE_CPU_USAGE */
#ifdef FFS_ENABLED
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(ffs_provision));
#endif /* FFS_ENABLED */

    /* unregister default exit to use our own */
    // extern shell_command_t g_shellCommandexit;
    // SHELL_UnregisterCommand(SHELL_COMMAND(exit));

    /* TODO do not register any exit command - it'll break the logging */
    //    SHELL_RegisterCommand(s_shellHandle, &g_shellCommandOverrideExit);

    /* read USB logging flag from flash */
    s_usb_logging = usb_logging_flag_flash_get();

    return status;
}

void sln_shell_task(void *arg)
{
    volatile EventBits_t shellEvents = 0U;
    status_t status                  = 0;
    // extern shell_command_t g_shellCommandexit;

    SHELL_Printf(s_shellHandle, "Howdy! Type \"help\" to see what this shell can do!\r\n");
    SHELL_Printf(s_shellHandle, "SHELL>> ");

    while (1)
    {
        shellEvents = xEventGroupWaitBits(s_ShellEventGroup, 0x00FFFFFF, pdTRUE, pdFALSE, portMAX_DELAY);

#if USE_WIFI_CONNECTION
        if (shellEvents & PRINT_EVT)
        {
            wifi_cred_t wifi_cred = {0};

            /* Stores the WiFi SSID as a printable string. Added one extra byte for the NULL terminator. */
            char ssid_str[sizeof(s_wifi_cred.ssid.value) + 1] = {0};

            status = wifi_credentials_flash_get(&wifi_cred);
            if (!status)
            {
                if (check_valid_credentials(&wifi_cred))
                {
                    SHELL_Printf(s_shellHandle, "Found no credentials in flash\r\n");
                    SHELL_Printf(s_shellHandle, "SHELL>> ");
                }
                else
                {
                    SHELL_Printf(s_shellHandle, "These are the credentials:\r\n");

                    strncpy(ssid_str, (char *)wifi_cred.ssid.value, sizeof(s_wifi_cred.ssid.value));
                    SHELL_Printf(s_shellHandle, "Wifi name: %s\r\n", ssid_str);

                    SHELL_Printf(s_shellHandle, "Wifi password: %s\r\n", wifi_cred.password.value);
                    SHELL_Printf(s_shellHandle, "SHELL>> ");
                }
            }
            else
            {
                SHELL_Printf(s_shellHandle, "Failed to read wifi credentials from flash, error code %d\r\n", status);
                SHELL_Printf(s_shellHandle, "SHELL>> ");
            }
        }

        if (shellEvents & SETUP_EVT)
        {
            status = wifi_credentials_flash_set(&s_wifi_cred);
            if (!status)
            {
                SHELL_Printf(s_shellHandle, "Credentials saved\r\n");
#if PROVISIONING_METHOD == PROVISIONING_USE_USB_CDC
                /* if appInitTask is suspended, wake it */
                if (s_appInitTask && eSuspended == eTaskGetState(s_appInitTask))
                {
                    SHELL_Printf(s_shellHandle, "SHELL>> ");
                    vTaskResume(s_appInitTask);
                }
                else
                {
                    sln_reset("SETUP_EVT");
                }
#else
                sln_reset("SETUP_EVT");
#endif
            }
            else
            {
                SHELL_Printf(s_shellHandle, "Failed to write wifi credentials in flash, error code %d\r\n", status);
                SHELL_Printf(s_shellHandle, "SHELL>> ");
            }
        }

        if (shellEvents & ERASE_EVT)
        {
            status = wifi_credentials_flash_reset();
            if (!status)
            {
                SHELL_Printf(s_shellHandle, "Credentials erased\r\n");
                sln_reset("ERASE_EVT");
            }
            else
            {
                SHELL_Printf(s_shellHandle, "Failed to wipe wifi credentials from flash, error code %d\r\n", status);
                SHELL_Printf(s_shellHandle, "SHELL>> ");
            }
        }
#endif

        if (shellEvents & RESET_EVT)
        {
            sln_reset("RESET_EVT");
        }

        if (shellEvents & FACTORY_RESET_EVT)
        {
            AIS_Deregister();
        }

        if (shellEvents & MEM_VIEW_EVT)
        {
            /* Print out available bytes in the FreeRTOS heap */
            SHELL_Printf(s_shellHandle, "Available Heap: %d\r\n", xPortGetFreeHeapSize());
            SHELL_Printf(s_shellHandle, "SHELL>> ");
        }

        if (shellEvents & ENA_HEAP_TRACE_EVT)
        {
            /* Print out available bytes in the FreeRTOS heap */
            s_heap_trace.enable = 1;
        }

        if (shellEvents & DIS_HEAP_TRACE_EVT)
        {
            /* Print out available bytes in the FreeRTOS heap */
            s_heap_trace.enable    = 0;
            s_heap_trace.threshold = 0;
        }

        if (shellEvents & ENA_USB_LOG_EVT)
        {
            s_usb_logging = USB_LOG_ON;
            status        = usb_logging_flag_flash_set(s_usb_logging);

            if (!status)
            {
                SHELL_Printf(s_shellHandle, "USB logging enabled\r\n");
                SHELL_Printf(s_shellHandle, "SHELL>> ");
            }
            else
            {
                SHELL_Printf(s_shellHandle, "Failed to write usb logging flag in flash, error code %d\r\n", status);
                SHELL_Printf(s_shellHandle, "SHELL>> ");
            }
        }

        if (shellEvents & DIS_USB_LOG_EVT)
        {
            s_usb_logging = USB_LOG_OFF;
            status        = usb_logging_flag_flash_set(s_usb_logging);

            if (!status)
            {
                SHELL_Printf(s_shellHandle, "USB logging disabled\r\n");
                SHELL_Printf(s_shellHandle, "SHELL>> ");
            }
            else
            {
                SHELL_Printf(s_shellHandle, "Failed to write usb logging flag in flash, error code %d\r\n", status);
                SHELL_Printf(s_shellHandle, "SHELL>> ");
            }
        }

        if (shellEvents & LOGS_EVT)
        {
            log_history_print();
        }

        if (shellEvents & SERIAL_NUMBER_EVT)
        {
            char *serialNumber = NULL;

#if USE_BASE64_UNIQUE_ID
            /* Show the "=" character as this is what is labeled on the box */
            APP_GetUniqueID(&serialNumber, false);
#else
            APP_GetHexUniqueID(&serialNumber);
#endif /* USE_BASE64_UNIQUE_ID */

            SHELL_Printf(s_shellHandle, "%s\r\n", serialNumber);
            SHELL_Printf(s_shellHandle, "SHELL>> ");

            vPortFree(serialNumber);
        }

        if (shellEvents & FAULTLOG_STATUSGET_EVT)
        {
            fault_ret_t fault_ret = fault_context_print();

            if (kFaultRet_NotFound == fault_ret)
            {
                SHELL_Printf(s_shellHandle, "No fault log detected in flash\r\n");
            }
            else if (kFaultRet_Error == fault_ret)
            {
                SHELL_Printf(s_shellHandle, "[ERR] Failed to get fault log from flash\r\n");
            }

            SHELL_Printf(s_shellHandle, "SHELL>> ");
        }

        if (shellEvents & FAULTLOG_STATUSERASE_EVT)
        {
            if (0 == fault_context_erase())
            {
                SHELL_Printf(s_shellHandle, "Fault log successfully erased from flash\r\n");
            }
            else
            {
                SHELL_Printf(s_shellHandle, "Failed to erase the fault log from flash\r\n");
            }
            SHELL_Printf(s_shellHandle, "SHELL>> ");
        }

        if (shellEvents & VERSION_EVT)
        {
            SHELL_Printf(s_shellHandle, "Firmware version: %d.%d.%d, App type: %s\r\n", APP_MAJ_VER, APP_MIN_VER,
                         APP_BLD_VER, getAppType());
            SHELL_Printf(s_shellHandle, "SHELL>> ");
        }

        if (shellEvents & WW_EVT)
        {
            char *wakeWordString  = NULL;
            uint32_t ww_model_len = 0;

            if (0 == strcmp(ww_model_cmd.cmd, "show"))
            {
                ww_model_len = SLN_AMAZON_WAKE_GetModelLocaleSize();

                /* There is no valid wake word length */
                if (ww_model_len > 0)
                {
                    wakeWordString = pvPortMalloc(ww_model_len);
                }

                /* Check if the string was malloc'd */
                if (wakeWordString != NULL)
                {
                    SLN_AMAZON_WAKE_GetModelLocale((uint8_t *)wakeWordString);
                }

                /* Ensure there is a valid string length and there is valid memory address */
                if (ww_model_len > 0 && wakeWordString != NULL)
                {
                    SHELL_Printf(s_shellHandle, "\r\nLanguage found in flash: %s\r\n", (char *)wakeWordString);
                }
                else
                {
                    SHELL_Printf(s_shellHandle, "\r\nNo Wake Word locale found, may still be initializing... \r\n");
                }

                /* Need to free the memory */
                if (wakeWordString != NULL)
                {
                    vPortFree(wakeWordString);
                }

                /* Re-prompt the shell for next command */
                SHELL_Printf(s_shellHandle, "SHELL>> ");
            }
            else if (0 == strcmp(ww_model_cmd.cmd, "set"))
            {
                /* Check if model entered is supported */
                status = SLN_AMAZON_WAKE_IsWakeWordSupported(ww_model_cmd.lang_code);
                switch (status)
                {
                    case WW_MODEL_SUCCESS:
                        SHELL_Printf(s_shellHandle, "\r\nSending a language change request !\r\n", status);
                        sln_shell_locales_changed_event(ww_model_cmd.lang_code);
                        break;

                    case WW_MODEL_NULL:
                        SHELL_Printf(s_shellHandle, "\r\nSetting model failed! model must not be NULL!\r\n", status);
                        break;

                    case WW_MODEL_NOT_SUPPORTED:
                        SHELL_Printf(
                            s_shellHandle,
                            "\r\nThis model is not supported, please enter 'help' to see list of supported models\r\n");
                        break;

                    case WW_MODEL_FAILED:
                        SHELL_Printf(s_shellHandle, "\r\nFailed to save model to flash\r\n");
                        break;

                    default:
                        SHELL_Printf(s_shellHandle, "\r\nUnknown Error\r\n");
                        break;
                }
            }
            else
            {
                SHELL_Printf(s_shellHandle,
                             "\r\nIncorrect command parameter(s).  Enter \"help\" to view a list of available "
                             "commands.\r\n\r\n");
            }
        }

        if (shellEvents & HEAP_VIEW_EVT)
        {
            PERF_PrintHeap();
        }

        if (shellEvents & STACKS_VIEW_EVT)
        {
            PERF_PrintStacks();
        }

#ifdef SLN_TRACE_CPU_USAGE
        if (shellEvents & TRACE_CPU_USAGE_EVT)
        {
            char *cpuLoad = NULL;

            cpuLoad = PERF_GetCPULoad();
            if (cpuLoad != NULL)
            {
                SHELL_Printf(s_shellHandle, "\r\n\r\nCPU usage:\r\n%s\r\n", cpuLoad);
            }
            else
            {
                SHELL_Printf(s_shellHandle, "Could NOT get the CPU usage\r\n");
            }
        }
#endif /* SLN_TRACE_CPU_USAGE */

#ifdef FFS_ENABLED
        if (shellEvents & FFS_PROVISION_EVT)
        {
            ffs_provision_status_t ffs_status;
            ffs_status = FFSPROVISION_execute_command(s_shellHandle);
            if (ffs_status == kStatus_FFS_PROVISION_Success)
            {
                SHELL_Printf(s_shellHandle, "ffs_provision command success\r\n");
            }
            else
            {
                SHELL_Printf(s_shellHandle, "ffs_provision command fail execution: %d\r\n", ffs_status);
            }
        }
#endif /* FFS_ENABLED */
    }

    while (SerialManager_Deinit(s_serialHandle) == kStatus_SerialManager_Busy)
    {
        /* should happen pretty quickly after the call of g_shellCommandexit.pFuncCallBack,
         * just need to wait till the read and write handles are closed */
    }

    /* clean event group */
    vEventGroupDelete(s_ShellEventGroup);

    vTaskDelete(NULL);
}

void sln_shell_set_app_init_task_handle(TaskHandle_t *handle)
{
    if (NULL != handle)
    {
        s_appInitTask = *handle;
    }
}

void sln_shell_trace_malloc(void *ptr, size_t size)
{
    if (s_shellHandle)
    {
        if (s_heap_trace.enable)
        {
            if (size >= s_heap_trace.threshold)
            {
                SHELL_Printf(s_shellHandle, "[TRACE] Allocated %d bytes to 0x%X\r\n", size, (int)ptr);
                SHELL_Printf(s_shellHandle, "SHELL>> ");
            }
        }
    }
}

void sln_shell_trace_free(void *ptr, size_t size)
{
    if (s_shellHandle)
    {
        if (s_heap_trace.enable)
        {
            if (size >= s_heap_trace.threshold)
            {
                SHELL_Printf(s_shellHandle, "[TRACE] De-allocated %d bytes from 0x%X\r\n", size, (int)ptr);
                SHELL_Printf(s_shellHandle, "SHELL>> ");
            }
        }
    }
}

static uint8_t usb_logging_flag_flash_set(uint8_t value)
{
    status_t status   = 0;
    uint32_t len      = 0;
    sln_dev_cfg_t cfg = DEFAULT_CFG_VALUES;

    status = SLN_FLASH_MGMT_Read(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, &len);

    if ((SLN_FLASH_MGMT_OK == status) || (SLN_FLASH_MGMT_ENOENTRY2 == status))
    {
        cfg.usb_logging_flag = value;
        status               = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));

        if ((SLN_FLASH_MGMT_EOVERFLOW == status) || (SLN_FLASH_MGMT_EOVERFLOW2 == status))
        {
            SLN_FLASH_MGMT_Erase(DEVICE_CONFIG_FILE_NAME);

            status = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));
        }
    }

    return status == 0 ? 0 : 1;
}

static uint8_t usb_logging_flag_flash_get(void)
{
    status_t status   = 0;
    uint32_t len      = 0;
    sln_dev_cfg_t cfg = DEFAULT_CFG_VALUES;
    uint8_t usb_log_status;

    status = SLN_FLASH_MGMT_Read(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, &len);
    if (status == SLN_FLASH_MGMT_OK)
    {
        usb_log_status = cfg.usb_logging_flag;
    }
    else
    {
        usb_log_status = DEFAULT_USB_LOGGING_VAL;
    }

    return usb_log_status;
}

int log_shell_printf(const char *formatString, ...)
{
    va_list ap;
    char logbuf[configLOGGING_MAX_MESSAGE_LENGTH] = {0};
    int status;

    if (s_usb_logging == USB_LOG_ON)
    {
        va_start(ap, formatString);
        status = vsnprintf(logbuf, configLOGGING_MAX_MESSAGE_LENGTH, formatString, ap);
        va_end(ap);

        if (status >= 0)
        {
            SHELL_Write(s_shellHandle, logbuf, strlen(logbuf));
        }
        else
        {
            SHELL_Write(s_shellHandle, LOG_ERROR_PRINT, strlen(LOG_ERROR_PRINT));
        }
    }

    return 0;
}

void log_history_log_add(const char *formatString, ...)
{
    va_list ap;

    va_start(ap, formatString);
    s_log_history_pos = (s_log_history_pos + 1) % MAX_LOG_HISTORY;
    memset(log_history[s_log_history_pos], 0, configLOGGING_MAX_MESSAGE_LENGTH);

    vsnprintf(log_history[s_log_history_pos], configLOGGING_MAX_MESSAGE_LENGTH, formatString, ap);

    va_end(ap);
}

static void log_history_print(void)
{
    uint8_t position = 0;
    /* The maximum len is 256 if we increase it we need to modify the type of len */
    uint8_t len = 0;

    for (int idx = 1; idx <= MAX_LOG_HISTORY; idx++)
    {
        position = (s_log_history_pos + idx) % MAX_LOG_HISTORY;
        len      = safe_strlen(log_history[position], configLOGGING_MAX_MESSAGE_LENGTH);

        if (len > 0)
            SHELL_Write(s_shellHandle, log_history[position], len);
    }
}

uint32_t log_history_get_size()
{
    uint8_t position          = s_log_history_pos;
    uint32_t total_log_size   = 0;
    uint32_t current_log_size = 0;

    /* Get total logs size */
    for (int idx = 1; idx <= MAX_LOG_HISTORY; idx++)
    {
        position = (s_log_history_pos + idx) % MAX_LOG_HISTORY;

        current_log_size = safe_strlen(log_history[position], configLOGGING_MAX_MESSAGE_LENGTH);

        /* Ignore empty string but move forward through all the entries */
        if (current_log_size == 0)
            continue;

        total_log_size += current_log_size;
    }

    return total_log_size;
}

void log_history_buffer_create(uint8_t *logs_buffer, uint32_t logs_buffer_size)
{
    uint32_t total_log_size   = 0;
    uint32_t current_log_size = 0;
    uint8_t position          = s_log_history_pos;

    if (logs_buffer)
    {
        for (int idx = 1; idx <= MAX_LOG_HISTORY; idx++)
        {
            position = (s_log_history_pos + idx) % MAX_LOG_HISTORY;

            current_log_size = safe_strlen(log_history[position], configLOGGING_MAX_MESSAGE_LENGTH);

            /* Ignore empty string but move forward through all the entries */
            if (current_log_size == 0)
                continue;

            /* If no more space, we need to return */
            if ((current_log_size + total_log_size) > logs_buffer_size)
                break;

            /* copy the log */
            memcpy(logs_buffer + total_log_size, log_history[position], current_log_size);
            total_log_size += current_log_size;
        }
    }
}

void fault_context_print_logs(uint32_t addr, uint32_t size)
{
    uint32_t chunk                     = 0;
    uint8_t flash_buf[FLASH_PAGE_SIZE] = {0};

    while (size)
    {
        chunk = FLASH_PAGE_SIZE > size ? size : FLASH_PAGE_SIZE;

        memset(flash_buf, 0, FLASH_PAGE_SIZE);
        SLN_Read_Flash_At_Address(addr, flash_buf, chunk);

        SHELL_Write(s_shellHandle, (char *)flash_buf, chunk);

        size -= chunk;
        addr += chunk;
    }
}

void vAssertCalled(const char *const pcFileName, unsigned long ulLine)
{
    if (pcFileName)
    {
        log_shell_printf("[ASSERT] Triggered in file %s, line %lu\r\n", pcFileName, ulLine);
    }
}

void sln_shell_set_app_queue_handle(QueueHandle_t *handle)
{
    if ((NULL != handle) && (NULL != *handle))
    {
        s_appEventQueue = *handle;
    }
}

static void sln_shell_locales_changed_event(char *locale)
{
    app_events_t event;
    reconnectState_t currState = reconnection_task_get_state();

    event.category = kAvsEvents;
    event.event    = kLocaleChanged;
    event.data     = locale;

    /* If the device is offline, send an error */
    if ((kStartState == currState) || (kLinkUp == currState))
    {
        if (s_appEventQueue != NULL)
        {
            /* Add event onto the queue */
            while (errQUEUE_FULL == xQueueSend(s_appEventQueue, &event, (TickType_t)0))
            {
                vTaskDelay(portTICK_PERIOD_MS * 50);
                SHELL_Printf(s_shellHandle,
                             "\r\nWarning: Fail to send LocaleChange event, queue is full, trying again\r\n");
            }
        }
        else
        {
            SHELL_Printf(s_shellHandle, "\r\nERROR: Invalid event queue pointer!\r\n");
        }
    }
    else
    {
        SHELL_Printf(s_shellHandle, "\r\nERROR: Device is not connected try again later!\r\n");
    }
}
