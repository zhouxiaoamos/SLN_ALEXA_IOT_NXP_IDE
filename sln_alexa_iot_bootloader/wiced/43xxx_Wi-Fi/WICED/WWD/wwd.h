/*
 * wwd.h
 *
 *  Created on: Jul 10, 2018
 *      Author: revathy
 */

#ifndef WWD_H_
#define WWD_H_


#include  "wwd_management.h"
#include "wwd_buffer.h"
#include "wwd_buffer_interface.h"
#include "wwd_wifi.h"

//NXP IMXRT Platform specific headers
#include "fsl_debug_console.h"
#include "fsl_sdmmc_host.h"
#include "fsl_sdio.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define WWD_SDIO_IRQ
/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
extern void  host_platform_sdio_irq_callback(void *userData);
/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/


#endif /* WWD_H_ */
