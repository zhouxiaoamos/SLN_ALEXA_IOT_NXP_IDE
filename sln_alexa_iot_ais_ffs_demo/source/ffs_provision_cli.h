/*
 * Copyright 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef FFS_PROVISION_CLI_H_
#define FFS_PROVISION_CLI_H_

#include <stdint.h>
#include "fsl_shell.h"

typedef enum _ffs_provision_status
{
    kStatus_FFS_PROVISION_Success = 0,
    kStatus_FFS_PROVISION_Error,
    kStatus_FFS_PROVISION_CommandNotFound,
    kStatus_FFS_PROVISION_BadCommandParameters,
    kStatus_FFS_PROVISION_MemoryFail,
    kStatus_FFS_PROVISION_AnotherCommandRegistred,
    kStatus_FFS_PROVISION_NoCommandRegistred,
} ffs_provision_status_t;

/* Check the parameters of the incoming command and in case of a match,
 * save the necessary information about the command to a static structure
 * and let the shell task execute the command (using FFSPROVISION_execute_command function).
 * Only one command can be registered at a time. The command slot is freed and
 * ready to accept new command only after the previously stored command is executed.
 */
ffs_provision_status_t FFSPROVISION_register_command(int32_t argc, char **argv);

/* Execute the previously stored command (using FFSPROVISION_register_command function)
 * and free the slot for new command.
 * In case of ffs_provision "GET" type commands, print the data to the shell.
 */
ffs_provision_status_t FFSPROVISION_execute_command(shell_handle_t s_shellHandle);

/* Set the internal variable s_ffsCliProvisionReady to True.
 * Calling this function means that the device is ready to be provisioned.
 */
void FFSPROVISION_set_ready(void);

/* Get the internal variable s_ffsCliProvisionDone.
 * This variable is set to True after the device was provisioned (certificate saved).
 */
bool FFSPROVISION_get_done(void);

#endif /* FFS_PROVISION_CLI_H_ */
