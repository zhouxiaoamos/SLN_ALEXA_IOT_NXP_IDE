/*
 * Copyright 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ffs_provision_cli.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "event_groups.h"

#include "hal_device_info.h"
#include "internal_device_info.h"
#include "hal_dha.h"
#include "afw_error.h"

#define CMD_PROVISION_READY "ready"           /* ffs_provision ready                     */
#define CMD_PROVISION_START "start"           /* ffs_provision start                     */
#define CMD_PROVISION_STOP  "stop"            /* ffs_provision stop                      */
#define CMD_PARAM_BEGIN     "param_begin"     /* ffs_provision param_begin [param size]  */
#define CMD_PARAM_CHUNK     "param_chunk"     /* ffs_provision param_chunk [param chunk] */
#define CMD_PARAM_NEWLINE   "param_newline"   /* ffs_provision param_newline             */
#define CMD_DEVICE_INFO_GET "device_info_get" /* ffs_provision device_info_get [entry]   */
#define CMD_DEVICE_INFO_SET "device_info_set" /* ffs_provision device_info_set [entry]   */
#define CMD_DHA_GET_FIELD   "dha_get_field"   /* ffs_provision dha_get_field   [entry]   */
#define CMD_DHA_KEYGEN      "dha_keygen"      /* ffs_provision dha_keygen                */
#define CMD_DHA_CERT_SET    "dha_cert_set"    /* ffs_provision dha_cert_set              */

#define CMD_VALUE_MAX_LEN 64

#define COMMAND_WORK_TODO_EVENT    (1 << 0)
#define COMMAND_WORK_SUCCESS_EVENT (1 << 1)
#define COMMAND_WORK_FAIL_EVENT    (1 << 2)

/*! @brief FFS PROVISION CLI Task settings */
#define FFS_PROVISION_CLI_TASK_NAME       "ffs_provision_cli_task"
#define FFS_PROVISION_CLI_TASK_STACK_SIZE 4096
#define FFS_PROVISION_CLI_TASK_PRIORITY   tskIDLE_PRIORITY + 6

typedef enum _ffs_provision_cmd_type
{
    kCMD_PROVISION_READY = 0,
    kCMD_PROVISION_START,
    kCMD_PROVISION_STOP,
    kCMD_PARAM_BEGIN,
    kCMD_PARAM_CHUNK,
    kCMD_PARAM_NEWLINE,
    kCMD_DEVICE_INFO_GET,
    kCMD_DEVICE_INFO_SET,
    kCMD_DHA_GET_FIELD,
    kCMD_DHA_KEYGEN,
    kCMD_DHA_SET_CERT,
} ffs_provision_cmd_type_t;

typedef struct _ffs_device_info_command
{
    bool task_initialized;
    ffs_provision_cmd_type_t cmd;
    bool registred;
    uint16_t entry;
    uint16_t entry_len;
    char *value;
    uint16_t param_size;
    uint16_t param_written;
    char *param_data;
    char *result;
} ffs_device_info_command_t;

static ffs_device_info_command_t s_ffs_provision_cmd = {0};
static TaskHandle_t xProvision_TaskHandle            = NULL;
static EventGroupHandle_t xProvision_Event           = NULL;

static bool s_ffsCliProvisionReady = false;
static bool s_ffsCliProvisionDone  = false;

/* Copy the command parameter to a local buffer during command registration.
 * This value is a chunk of a bigger parameter and will be copied to a bigger buffer param_data.
 * This function is called during "ffs_provision param_chunk [param chunk]" command registration
 */
static ffs_provision_status_t ffsprovision_set_value_field(char *value)
{
    ffs_provision_status_t status;

    if ((strlen(value) < CMD_VALUE_MAX_LEN) && (s_ffs_provision_cmd.value != NULL))
    {
        memcpy(s_ffs_provision_cmd.value, value, strlen(value));
        s_ffs_provision_cmd.value[strlen(value)] = '\0';
        status                                   = kStatus_FFS_PROVISION_Success;
    }
    else
    {
        status = kStatus_FFS_PROVISION_MemoryFail;
    }

    return status;
}

/* Convert the string DeviceInfo entry to the corresponding integer value.
 * The entry ID is passed using "ffs_provision device_info_get [entry]" shell command.
 */
static ffs_provision_status_t ffsprovision_device_info_convert_entry(char *entry_name)
{
    ffs_provision_status_t status = kStatus_FFS_PROVISION_Success;

    if (strcmp("dsn", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry     = ACE_DEVICE_INFO_DEVICE_SERIAL;
        s_ffs_provision_cmd.entry_len = DEVICE_INFO_DEVICE_SERIAL_MAX_LEN;
    }
    else if (strcmp("type", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry     = ACE_DEVICE_INFO_DEVICE_TYPE_ID;
        s_ffs_provision_cmd.entry_len = DEVICE_INFO_DEVICE_TYPE_ID_MAX_LEN;
    }
    else if (strcmp("ffs_pid", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry     = ACE_DEVICE_INFO_PRODUCT_ID;
        s_ffs_provision_cmd.entry_len = DEVICE_INFO_PRODUCT_ID_MAX_LEN;
    }
    else if (strcmp("ffs_pin", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry     = ACE_DEVICE_INFO_FFS_PIN;
        s_ffs_provision_cmd.entry_len = DEVICE_INFO_FFS_PIN_MAX_LEN;
    }
    else if (strcmp("dss_pub_key", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry     = ACE_DEVICE_INFO_DSS_PUB_KEY;
        s_ffs_provision_cmd.entry_len = DEVICE_INFO_DSS_PUB_KEY_MAX_LEN;
    }
    else if (strcmp("bt_name", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry     = ACE_DEVICE_INFO_BT_DEVICE_NAME;
        s_ffs_provision_cmd.entry_len = DEVICE_INFO_BT_DEVICE_NAME_MAX_LEN;
    }
    else if (strcmp("client_id", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry     = ACE_DEVICE_INFO_CLIENT_ID;
        s_ffs_provision_cmd.entry_len = DEVICE_INFO_CLIENT_ID_NAME_MAX_LEN;
    }
    else
    {
        status = kStatus_FFS_PROVISION_BadCommandParameters;
    }

    return status;
}

/* Get the value of a device_info entry.
 * This value will be printed by the shell task, because the content can be very long.
 * The entry ID is passed using "ffs_provision device_info_get [entry]" shell command.
 */
static ffs_provision_status_t ffsprovision_device_info_get(void)
{
    ffs_provision_status_t status = kStatus_FFS_PROVISION_Success;
    int device_info_status;
    uint16_t data_size;

    data_size                  = s_ffs_provision_cmd.entry_len + 1;
    s_ffs_provision_cmd.result = (char *)pvPortMalloc(data_size);
    if (s_ffs_provision_cmd.result != NULL)
    {
        memset(s_ffs_provision_cmd.result, 0, data_size);
        device_info_status =
            aceDeviceInfoDsHal_getEntry(s_ffs_provision_cmd.entry, s_ffs_provision_cmd.result, data_size);
        if (device_info_status >= 0)
        {
            status = kStatus_FFS_PROVISION_Success;
        }
        else
        {
            status = kStatus_FFS_PROVISION_Error;
        }
    }
    else
    {
        status = kStatus_FFS_PROVISION_MemoryFail;
    }

    return status;
}

/* Set value for a device_info entry.
 * The new value should have been passed previously by "ffs_provision param_chunk [param chunk]"
 * The entry ID passed using "ffs_provision device_info_set [entry]" shell command.
 */
static ffs_provision_status_t ffsprovision_device_info_set(void)
{
    ffs_provision_status_t status = kStatus_FFS_PROVISION_Success;
    ace_status_t device_info_status;

    if ((s_ffs_provision_cmd.param_data != NULL) &&
        (s_ffs_provision_cmd.param_size == s_ffs_provision_cmd.param_written) &&
        (s_ffs_provision_cmd.param_size <= s_ffs_provision_cmd.entry_len))
    {
        device_info_status =
            internal_deviceInfo_prototype_setEntry(s_ffs_provision_cmd.entry, s_ffs_provision_cmd.param_data);
        if (device_info_status == ACE_STATUS_OK)
        {
            status = kStatus_FFS_PROVISION_Success;
        }
        else
        {
            status = kStatus_FFS_PROVISION_Error;
        }
    }
    else
    {
        status = kStatus_FFS_PROVISION_MemoryFail;
    }

    return status;
}

/* Convert the string DHA entry to the corresponding integer value.
 * The entry ID is passed using "ffs_provision dha_get_field [entry]" shell command.
 */
static ffs_provision_status_t ffsprovision_dha_convert_entry(char *entry_name)
{
    ffs_provision_status_t status = kStatus_FFS_PROVISION_Success;

    if (strcmp("soc_id", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry = ACE_DHA_HAL_SOC_ID;
    }
    else if (strcmp("dsn", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry = ACE_DHA_HAL_DSN;
    }
    else if (strcmp("csr", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry = ACE_DHA_HAL_CSR;
    }
    else if (strcmp("crt_chain", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry = ACE_DHA_HAL_CERTIFICATE_CHAIN;
    }
    else if (strcmp("leaf_crt", entry_name) == 0)
    {
        s_ffs_provision_cmd.entry = ACE_DHA_HAL_LEAF_CERTIFICATE;
    }
    else
    {
        status = kStatus_FFS_PROVISION_BadCommandParameters;
    }

    return status;
}

/* Get a DHA entry.
 * This value will be printed by the shell task, because the content can be very long.
 * The entry ID is passed using "ffs_provision dha_get_field [entry]" shell command.
 */
static ffs_provision_status_t ffsprovision_dha_get_field(void)
{
    ffs_provision_status_t status = kStatus_FFS_PROVISION_Success;
    ace_status_t dha_status;
    char *data = NULL;
    uint32_t data_len;

    dha_status = aceDhaHal_open();

    if (dha_status == ACE_STATUS_OK)
    {
        dha_status = aceDhaHal_get_field(s_ffs_provision_cmd.entry, (void **)&data, (size_t *)&data_len);
    }

    if (dha_status == ACE_STATUS_OK)
    {
        if (data != NULL)
        {
            s_ffs_provision_cmd.result = (char *)pvPortMalloc(strlen(data) + 1);
            if (s_ffs_provision_cmd.result != NULL)
            {
                memcpy(s_ffs_provision_cmd.result, data, strlen(data));
                s_ffs_provision_cmd.result[strlen(data)] = '\0';
            }
            else
            {
                dha_status = ACE_STATUS_GENERAL_ERROR;
            }
            free(data);
        }
        else
        {
            dha_status = ACE_STATUS_GENERAL_ERROR;
        }
    }

    if (dha_status == ACE_STATUS_OK)
    {
        dha_status = aceDhaHal_close();
    }

    if (dha_status == ACE_STATUS_OK)
    {
        status = kStatus_FFS_PROVISION_Success;
    }
    else
    {
        status = kStatus_FFS_PROVISION_Error;
    }

    return status;
}

/* Generate a pair of keys for DHA.
 * This is called during execution of the "ffs_provision dha_keygen" shell command.
 */
static ffs_provision_status_t ffsprovision_dha_keygen(void)
{
    ffs_provision_status_t status = kStatus_FFS_PROVISION_Success;
    ace_status_t dha_status;

    dha_status = aceDhaHal_open();

    if (dha_status == ACE_STATUS_OK)
    {
        /* If the DHA key already exists on the device, it will not be created again
         * aceDhaHal_generate_dha_key will return -AFW_EROFS if DHA key already exist.
         */
        dha_status = aceDhaHal_generate_dha_key();
        if (dha_status == -AFW_EROFS)
        {
            dha_status = ACE_STATUS_OK;
        }
    }

    if (dha_status == ACE_STATUS_OK)
    {
        dha_status = aceDhaHal_close();
    }

    if (dha_status == ACE_STATUS_OK)
    {
        status = kStatus_FFS_PROVISION_Success;
    }
    else
    {
        status = kStatus_FFS_PROVISION_Error;
    }

    return status;
}

/* Save to KVS the previously received Certificate chain.
 * The new value should have been passed previously by "ffs_provision param_chunk [param chunk]"
 * This is called during execution of the "ffs_provision dha_cert_set" shell command.
 */
static ffs_provision_status_t ffsprovision_dha_set_cert(void)
{
    ffs_provision_status_t status = kStatus_FFS_PROVISION_Success;

    if ((s_ffs_provision_cmd.param_data != NULL) &&
        (s_ffs_provision_cmd.param_size == s_ffs_provision_cmd.param_written))
    {
        status = kStatus_FFS_PROVISION_Success;
        ace_status_t dha_status;

        dha_status = aceDhaHal_open();

        if (dha_status == ACE_STATUS_OK)
        {
            dha_status = aceDhaHal_set_certificate(s_ffs_provision_cmd.param_data);
        }

        if (dha_status == ACE_STATUS_OK)
        {
            dha_status = aceDhaHal_close();
        }

        if (dha_status == ACE_STATUS_OK)
        {
            s_ffsCliProvisionDone = true;
            status                = kStatus_FFS_PROVISION_Success;
        }
        else
        {
            status = kStatus_FFS_PROVISION_Error;
        }
    }
    else
    {
        status = kStatus_FFS_PROVISION_Error;
    }

    return status;
}

/* In case it is needed to pass a parameter longer than shell_command_buffer length, this parameter will be split and
 * sent in chunks. The first step is to get the total parameter length "ffs_provision param_begin [param size]". The
 * next step will be to receive the parameter chunks "ffs_provision param_chunk [param chunk]". Convert the total
 * long_parameter length from string to integer. This size will be used to allocate a buffer for the entire parameter.
 */
static ffs_provision_status_t ffsprovision_set_param_length(char *value)
{
    ffs_provision_status_t status;

    char *rest_arg  = NULL;
    char *new_value = NULL;
    int32_t new_value_int;

    new_value     = value;
    new_value_int = strtoul(new_value, &rest_arg, 0);

    if ((new_value == rest_arg) || *rest_arg)
    {
        status = kStatus_FFS_PROVISION_BadCommandParameters;
    }
    else
    {
        if ((new_value_int >= 0) && (new_value_int <= 0xFFFF))
        {
            s_ffs_provision_cmd.param_size = (uint16_t)new_value_int;
            status                         = kStatus_FFS_PROVISION_Success;
        }
        else
        {
            status = kStatus_FFS_PROVISION_BadCommandParameters;
        }
    }

    return status;
}

/* Allocate the parameter buffer.
 * The total parameter length is indicated in the "ffs_provision param_begin [param size]" shell command.
 */
static ffs_provision_status_t ffsprovision_alloc_param(void)
{
    ffs_provision_status_t status;

    s_ffs_provision_cmd.param_written = 0;
    s_ffs_provision_cmd.param_data    = (char *)pvPortMalloc(s_ffs_provision_cmd.param_size + 1);
    if (s_ffs_provision_cmd.param_data != NULL)
    {
        memset(s_ffs_provision_cmd.param_data, 0, s_ffs_provision_cmd.param_size + 1);
        status = kStatus_FFS_PROVISION_Success;
    }
    else
    {
        status = kStatus_FFS_PROVISION_MemoryFail;
    }

    return status;
}

/* Copy the incoming chunk of the parameter to the previously allocated parameter buffer.
 * The incoming parameter chunk is indicated in the "ffs_provision param_chunk [param chunk]" shell command.
 */
static ffs_provision_status_t ffsprovision_copy_param_chunk(void)
{
    ffs_provision_status_t status;

    if ((s_ffs_provision_cmd.param_data != NULL) && (s_ffs_provision_cmd.value != NULL))
    {
        if ((s_ffs_provision_cmd.param_written + strlen(s_ffs_provision_cmd.value)) <= s_ffs_provision_cmd.param_size)
        {
            memcpy(s_ffs_provision_cmd.param_data + s_ffs_provision_cmd.param_written, s_ffs_provision_cmd.value,
                   strlen(s_ffs_provision_cmd.value));
            s_ffs_provision_cmd.param_written += strlen(s_ffs_provision_cmd.value);

            status = kStatus_FFS_PROVISION_Success;
        }
        else
        {
            status = kStatus_FFS_PROVISION_BadCommandParameters;
        }
    }
    else
    {
        status = kStatus_FFS_PROVISION_MemoryFail;
    }

    return status;
}

/* The shell does not accept newlines as parameters, so append a newline to the parameter
 * in case the "ffs_provision param_newline" shell command was called.
 */
static ffs_provision_status_t ffsprovision_copy_param_newline(void)
{
    ffs_provision_status_t status;

    if ((s_ffs_provision_cmd.param_data != NULL) && (s_ffs_provision_cmd.value != NULL))
    {
        if ((s_ffs_provision_cmd.param_written + 1) <= s_ffs_provision_cmd.param_size)
        {
            s_ffs_provision_cmd.param_data[s_ffs_provision_cmd.param_written++] = '\n';
            status                                                              = kStatus_FFS_PROVISION_Success;
        }
        else
        {
            status = kStatus_FFS_PROVISION_BadCommandParameters;
        }
    }
    else
    {
        status = kStatus_FFS_PROVISION_MemoryFail;
    }

    return status;
}

/* Free the parameter buffer(in case previously allocated). */
static void ffsprovision_free_param(void)
{
    s_ffs_provision_cmd.param_written = 0;
    if (s_ffs_provision_cmd.param_data != NULL)
    {
        vPortFree(s_ffs_provision_cmd.param_data);
        s_ffs_provision_cmd.param_data = NULL;
    }
}

/* This is the handler of a dedicated task which handles the ffs_provision shell commands.
 * This task is needed because some device_info and DHA acs functions require a bigger stack size
 * than shell_task has.
 */
static void ffsprovision_provision_task_handler(void *parameters)
{
    ffs_provision_status_t status;

    while (1)
    {
        xEventGroupWaitBits(xProvision_Event, COMMAND_WORK_TODO_EVENT, pdTRUE, pdFALSE, portMAX_DELAY);
        if (s_ffs_provision_cmd.cmd == kCMD_PROVISION_READY)
        {
            status = kStatus_FFS_PROVISION_Success;
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_PROVISION_START)
        {
            status = kStatus_FFS_PROVISION_Success;
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_PROVISION_STOP)
        {
            status = kStatus_FFS_PROVISION_Success;
            break;
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_PARAM_BEGIN)
        {
            ffsprovision_free_param();
            status = ffsprovision_alloc_param();
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_PARAM_CHUNK)
        {
            status = ffsprovision_copy_param_chunk();
            if (status != kStatus_FFS_PROVISION_Success)
            {
                ffsprovision_free_param();
            }
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_PARAM_NEWLINE)
        {
            status = ffsprovision_copy_param_newline();
            if (status != kStatus_FFS_PROVISION_Success)
            {
                ffsprovision_free_param();
            }
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_DEVICE_INFO_GET)
        {
            status = ffsprovision_device_info_get();
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_DEVICE_INFO_SET)
        {
            status = ffsprovision_device_info_set();
            ffsprovision_free_param();
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_DHA_GET_FIELD)
        {
            status = ffsprovision_dha_get_field();
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_DHA_KEYGEN)
        {
            status = ffsprovision_dha_keygen();
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_DHA_SET_CERT)
        {
            status = ffsprovision_dha_set_cert();
            ffsprovision_free_param();
        }
        else
        {
            status = kStatus_FFS_PROVISION_BadCommandParameters;
        }

        if (status == kStatus_FFS_PROVISION_Success)
        {
            xEventGroupSetBits(xProvision_Event, COMMAND_WORK_SUCCESS_EVENT);
        }
        else
        {
            xEventGroupSetBits(xProvision_Event, COMMAND_WORK_FAIL_EVENT);
        }
    }

    xEventGroupSetBits(xProvision_Event, COMMAND_WORK_SUCCESS_EVENT);

    vTaskSuspend(NULL);
}

/* Wake up the dedicated ffs provisioning task to execute the registered commands.
 * Wait until it finishes to notify the shell_task about the result.
 */
static ffs_provision_status_t ffsprovision_execute_cmd_on_provision_task(void)
{
    ffs_provision_status_t status;
    volatile EventBits_t event_completed = 0U;

    if ((xProvision_Event != NULL) && (xProvision_TaskHandle != NULL) && (s_ffs_provision_cmd.task_initialized))
    {
        xEventGroupSetBits(xProvision_Event, COMMAND_WORK_TODO_EVENT);

        event_completed = xEventGroupWaitBits(xProvision_Event, (COMMAND_WORK_SUCCESS_EVENT | COMMAND_WORK_FAIL_EVENT),
                                              pdTRUE, pdFALSE, portMAX_DELAY);
        if (event_completed & COMMAND_WORK_SUCCESS_EVENT)
        {
            status = kStatus_FFS_PROVISION_Success;
        }
        else if (event_completed & COMMAND_WORK_FAIL_EVENT)
        {
            status = kStatus_FFS_PROVISION_Error;
        }
        else
        {
            status = kStatus_FFS_PROVISION_Error;
        }
    }
    else
    {
        status = kStatus_FFS_PROVISION_Error;
    }

    return status;
}

/* Stop the dedicated ffs_provisioning task. Notify it about the coming deletion,
 * wait until it frees any resource and delete it.
 */
static void ffsprovision_stop_task(void)
{
    ffs_provision_cmd_type_t original_cmd;

    if ((xProvision_Event != NULL) && (xProvision_TaskHandle != NULL))
    {
        original_cmd            = s_ffs_provision_cmd.cmd;
        s_ffs_provision_cmd.cmd = kCMD_PROVISION_STOP;
        ffsprovision_execute_cmd_on_provision_task();
        s_ffs_provision_cmd.cmd = original_cmd;
    }

    if (xProvision_TaskHandle != NULL)
    {
        vTaskDelete(xProvision_TaskHandle);
        xProvision_TaskHandle = NULL;
    }

    if (xProvision_Event != NULL)
    {
        vEventGroupDelete(xProvision_Event);
        xProvision_Event = NULL;
    }

    if (s_ffs_provision_cmd.value != NULL)
    {
        vPortFree(s_ffs_provision_cmd.value);
        s_ffs_provision_cmd.value = NULL;
    }

    if (s_ffs_provision_cmd.result != NULL)
    {
        vPortFree(s_ffs_provision_cmd.result);
        s_ffs_provision_cmd.result = NULL;
    }

    ffsprovision_free_param();

    s_ffs_provision_cmd.task_initialized = false;
    s_ffs_provision_cmd.entry            = 0;
    s_ffs_provision_cmd.entry_len        = 0;
    s_ffs_provision_cmd.param_size       = 0;
}

/* Start the dedicated ffs_provisioning task. Create an Event in order
 * to notify it the dedicated task about work to do.
 */
static ffs_provision_status_t ffsprovision_start_task(void)
{
    ffs_provision_status_t status;
    BaseType_t task_status;

    ffsprovision_stop_task();

    s_ffs_provision_cmd.value = (char *)pvPortMalloc(CMD_VALUE_MAX_LEN);
    if (s_ffs_provision_cmd.value != NULL)
    {
        status = kStatus_FFS_PROVISION_Success;
    }
    else
    {
        status = kStatus_FFS_PROVISION_MemoryFail;
    }

    if (status == kStatus_FFS_PROVISION_Success)
    {
        if (xProvision_Event == NULL)
        {
            xProvision_Event = xEventGroupCreate();
            if (xProvision_Event != NULL)
            {
                status = kStatus_FFS_PROVISION_Success;
            }
            else
            {
                status = kStatus_FFS_PROVISION_Error;
            }
        }
        else
        {
            status = kStatus_FFS_PROVISION_Error;
        }
    }

    if (status == kStatus_FFS_PROVISION_Success)
    {
        if (xProvision_TaskHandle == NULL)
        {
            task_status = xTaskCreate(ffsprovision_provision_task_handler, FFS_PROVISION_CLI_TASK_NAME,
                                      FFS_PROVISION_CLI_TASK_STACK_SIZE, (void *)NULL, FFS_PROVISION_CLI_TASK_PRIORITY,
                                      &xProvision_TaskHandle);
            if (task_status == pdTRUE)
            {
                status = kStatus_FFS_PROVISION_Success;
            }
            else
            {
                status = kStatus_FFS_PROVISION_Error;
            }
        }
        else
        {
            status = kStatus_FFS_PROVISION_Error;
        }
    }

    if (status == kStatus_FFS_PROVISION_Success)
    {
        s_ffs_provision_cmd.task_initialized = true;
        status                               = ffsprovision_execute_cmd_on_provision_task();
    }
    else
    {
        ffsprovision_stop_task();
    }

    return status;
}

/* Check the parameters of the incoming function. */
static ffs_provision_status_t ffsprovision_check_command(int32_t argc, char **argv, char *cmd, int32_t cmd_argc)
{
    ffs_provision_status_t status;

    if ((strlen(argv[1]) == strlen(cmd)) && (strncmp(argv[1], cmd, strlen(cmd)) == 0))
    {
        if (argc == cmd_argc)
        {
            if (s_ffs_provision_cmd.registred == false)
            {
                status = kStatus_FFS_PROVISION_Success;
            }
            else
            {
                status = kStatus_FFS_PROVISION_AnotherCommandRegistred;
            }
        }
        else
        {
            status = kStatus_FFS_PROVISION_BadCommandParameters;
        }
    }
    else
    {
        status = kStatus_FFS_PROVISION_CommandNotFound;
    }

    return status;
}

ffs_provision_status_t FFSPROVISION_register_command(int32_t argc, char **argv)
{
    ffs_provision_status_t status;

    if (argc >= 2)
    {
        status = ffsprovision_check_command(argc, argv, CMD_PROVISION_READY, 2);
        if (status == kStatus_FFS_PROVISION_Success)
        {
            s_ffs_provision_cmd.cmd       = kCMD_PROVISION_READY;
            s_ffs_provision_cmd.registred = true;
            status                        = kStatus_FFS_PROVISION_Success;
        }

        if (status == kStatus_FFS_PROVISION_CommandNotFound)
        {
            status = ffsprovision_check_command(argc, argv, CMD_PROVISION_START, 2);
            if (status == kStatus_FFS_PROVISION_Success)
            {
                s_ffs_provision_cmd.cmd       = kCMD_PROVISION_START;
                s_ffs_provision_cmd.registred = true;
                status                        = kStatus_FFS_PROVISION_Success;
            }
        }

        if (status == kStatus_FFS_PROVISION_CommandNotFound)
        {
            status = ffsprovision_check_command(argc, argv, CMD_PROVISION_STOP, 2);
            if (status == kStatus_FFS_PROVISION_Success)
            {
                s_ffs_provision_cmd.cmd       = kCMD_PROVISION_STOP;
                s_ffs_provision_cmd.registred = true;
                status                        = kStatus_FFS_PROVISION_Success;
            }
        }

        if (status == kStatus_FFS_PROVISION_CommandNotFound)
        {
            status = ffsprovision_check_command(argc, argv, CMD_PARAM_BEGIN, 3);
            if (status == kStatus_FFS_PROVISION_Success)
            {
                s_ffs_provision_cmd.cmd = kCMD_PARAM_BEGIN;
                status                  = ffsprovision_set_param_length(argv[2]);
                if (status == kStatus_FFS_PROVISION_Success)
                {
                    s_ffs_provision_cmd.registred = true;
                }
            }
        }

        if (status == kStatus_FFS_PROVISION_CommandNotFound)
        {
            status = ffsprovision_check_command(argc, argv, CMD_PARAM_CHUNK, 3);
            if (status == kStatus_FFS_PROVISION_Success)
            {
                s_ffs_provision_cmd.cmd = kCMD_PARAM_CHUNK;
                status                  = ffsprovision_set_value_field(argv[2]);
                if (status == kStatus_FFS_PROVISION_Success)
                {
                    s_ffs_provision_cmd.registred = true;
                }
            }
        }

        if (status == kStatus_FFS_PROVISION_CommandNotFound)
        {
            status = ffsprovision_check_command(argc, argv, CMD_PARAM_NEWLINE, 2);
            if (status == kStatus_FFS_PROVISION_Success)
            {
                s_ffs_provision_cmd.cmd       = kCMD_PARAM_NEWLINE;
                s_ffs_provision_cmd.registred = true;
                status                        = kStatus_FFS_PROVISION_Success;
            }
        }

        if (status == kStatus_FFS_PROVISION_CommandNotFound)
        {
            status = ffsprovision_check_command(argc, argv, CMD_DEVICE_INFO_GET, 3);
            if (status == kStatus_FFS_PROVISION_Success)
            {
                s_ffs_provision_cmd.cmd = kCMD_DEVICE_INFO_GET;
                status                  = ffsprovision_device_info_convert_entry(argv[2]);
                if (status == kStatus_FFS_PROVISION_Success)
                {
                    s_ffs_provision_cmd.registred = true;
                }
            }
        }

        if (status == kStatus_FFS_PROVISION_CommandNotFound)
        {
            status = ffsprovision_check_command(argc, argv, CMD_DEVICE_INFO_SET, 3);
            if (status == kStatus_FFS_PROVISION_Success)
            {
                s_ffs_provision_cmd.cmd = kCMD_DEVICE_INFO_SET;
                status                  = ffsprovision_device_info_convert_entry(argv[2]);
                if (status == kStatus_FFS_PROVISION_Success)
                {
                    s_ffs_provision_cmd.registred = true;
                }
            }
        }

        if (status == kStatus_FFS_PROVISION_CommandNotFound)
        {
            status = ffsprovision_check_command(argc, argv, CMD_DHA_GET_FIELD, 3);
            if (status == kStatus_FFS_PROVISION_Success)
            {
                s_ffs_provision_cmd.cmd = kCMD_DHA_GET_FIELD;
                status                  = ffsprovision_dha_convert_entry(argv[2]);
                if (status == kStatus_FFS_PROVISION_Success)
                {
                    s_ffs_provision_cmd.registred = true;
                }
            }
        }

        if (status == kStatus_FFS_PROVISION_CommandNotFound)
        {
            status = ffsprovision_check_command(argc, argv, CMD_DHA_KEYGEN, 2);
            if (status == kStatus_FFS_PROVISION_Success)
            {
                s_ffs_provision_cmd.cmd       = kCMD_DHA_KEYGEN;
                s_ffs_provision_cmd.registred = true;
                status                        = kStatus_FFS_PROVISION_Success;
            }
        }

        if (status == kStatus_FFS_PROVISION_CommandNotFound)
        {
            status = ffsprovision_check_command(argc, argv, CMD_DHA_CERT_SET, 2);
            if (status == kStatus_FFS_PROVISION_Success)
            {
                s_ffs_provision_cmd.cmd       = kCMD_DHA_SET_CERT;
                s_ffs_provision_cmd.registred = true;
                status                        = kStatus_FFS_PROVISION_Success;
            }
        }
    }
    else
    {
        status = kStatus_FFS_PROVISION_BadCommandParameters;
    }

    return status;
}

ffs_provision_status_t FFSPROVISION_execute_command(shell_handle_t s_shellHandle)
{
    ffs_provision_status_t status;

    if (s_ffs_provision_cmd.registred == true)
    {
        if (s_ffs_provision_cmd.cmd == kCMD_PROVISION_READY)
        {
            SHELL_Printf(s_shellHandle, "ffs provision ready = %d\r\n", s_ffsCliProvisionReady);
            status = kStatus_FFS_PROVISION_Success;
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_PROVISION_START)
        {
            status = ffsprovision_start_task();
        }
        else if (s_ffs_provision_cmd.cmd == kCMD_PROVISION_STOP)
        {
            ffsprovision_stop_task();
            status = kStatus_FFS_PROVISION_Success;
        }
        else
        {
            status = ffsprovision_execute_cmd_on_provision_task();
        }

        if (status == kStatus_FFS_PROVISION_Success)
        {
            if ((s_ffs_provision_cmd.cmd == kCMD_DEVICE_INFO_GET) || (s_ffs_provision_cmd.cmd == kCMD_DHA_GET_FIELD))
            {
                if (s_ffs_provision_cmd.result != NULL)
                {
                    SHELL_Printf(s_shellHandle, "%s\r\n", s_ffs_provision_cmd.result);
                    vPortFree(s_ffs_provision_cmd.result);
                    s_ffs_provision_cmd.result = NULL;
                }
                else
                {
                    status = kStatus_FFS_PROVISION_Error;
                }
            }
        }

        s_ffs_provision_cmd.registred = false;
    }
    else
    {
        status = kStatus_FFS_PROVISION_NoCommandRegistred;
    }

    return status;
}

void FFSPROVISION_set_ready(void)
{
    s_ffsCliProvisionReady = true;
}

bool FFSPROVISION_get_done(void)
{
    return s_ffsCliProvisionDone;
}
