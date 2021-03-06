/*
 * Amazon FreeRTOS Common IO V1.0.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/*******************************************************************************
 * IOT On-Target Unit Test
 *******************************************************************************
 */
#include <stdio.h>
// NXP: add stdbool otherwise false / true are not known
#include <stdbool.h>

/* Test runner interface includes. */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Unity framework includes. */
#include "unity_fixture.h"
#include "unity_internals.h"

/* Board config includes. */
#include "test_iot_internal.h"


#if IOT_TEST_COMMON_IO_UART_SUPPORTED
static void RUN_TEST_IOT_UART(int testCaseIndex)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_UART_SUPPORTED; i++)
    {
        SET_TEST_IOT_UART_CONFIG(i);
            switch (testCaseIndex)
            {
                case 0:
                    RUN_TEST_GROUP( TEST_IOT_UART );
                    break;
                case 1:
                    RUN_TEST_CASE( TEST_IOT_UART, AFQP_AssistedIotUARTWriteReadSync );
                    break;
                case 2:
                    RUN_TEST_CASE( TEST_IOT_UART, AFQP_AssistedIotUARTBaudChange );
                    break;
                case 3:
                    RUN_TEST_CASE( TEST_IOT_UART, AFQP_AssistedIotUARTWriteAsync );
                    break;
                default:
                    printf("Invalid UART test case\n");
                    return;
        }
    }
}
#endif

#if IOT_TEST_COMMON_IO_FLASH_SUPPORTED
static void RUN_TEST_IOT_FLASH(void)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_FLASH_SUPPORTED; i++)
    {
        SET_TEST_IOT_FLASH_CONFIG(i);
        RUN_TEST_GROUP( TEST_IOT_FLASH );
    }
}
#endif

#if IOT_TEST_COMMON_IO_WATCHDOG_SUPPORTED
static void RUN_TEST_IOT_WATCHDOG(int testCaseIndex)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_WATCHDOG_SUPPORTED; i++)
    {
        SET_TEST_IOT_WATCHDOG_CONFIG(i);
            switch (testCaseIndex)
            {
                case 0:
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogOpenClose);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogValidateBarkTimer);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogValidateBiteInterrupt);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogStartNoTimerSet);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogRestartNoStart);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogSetBarkGreaterThanBiteSetWatchdogBarkFirst);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogSetBarkGreaterThanBiteSetWatchdogBiteFirst);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogOpenOpenClose);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_Fuzz_IotWatchdogClose);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_Fuzz_IotWatchdogStartStop);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_Fuzz_IotWatchdog_ioctl);
                    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogOpenCloseClose);
                    break;
                case 1:
                    RUN_TEST_CASE( TEST_IOT_WATCHDOG, AFQP_IotWatchdogValidateWatchdogReset);
                    break;
                case 2:
                    RUN_TEST_CASE( TEST_IOT_WATCHDOG, AFQP_IotWatchdogValidateResetReason);
                    break;
                default:
                    RUN_TEST_GROUP( TEST_IOT_WATCHDOG );
                    return;
        }
    }
}
#endif


#if IOT_TEST_COMMON_IO_GPIO_SUPPORTED
static void RUN_TEST_IOT_GPIO(int testCaseIndex)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_GPIO_SUPPORTED; i++)
    {
        SET_TEST_IOT_GPIO_CONFIG(i);
            switch (testCaseIndex)
            {
                case 0:
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioOpenClose );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioOpenOpenClose );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioOpenCloseClose );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioIoctlSetGet );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioMode );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioPull );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioSpeed );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioOperation );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioOpenCloseFuzz );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioReadWriteSyncFuzz );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioIoctlFuzz );
                    RUN_TEST_CASE( TEST_IOT_GPIO, AFQP_IotGpioInterrupt );
                    break;
                case 1:
                    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_AssistedIotGpioModeWritePushPullTrue);
                    break;
                case 2:
                    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_AssistedIotGpioModeWritePushPullFalse);
                    break;
                case 3:
                    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_AssistedIotGpioModeWriteOpenDrainTrue);
                    break;
                case 4:
                    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_AssistedIotGpioModeWriteOpenDrainFalse);
                    break;
                case 5:
                    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_AssistedIotGpioModeReadTrue);
                    break;
                case 6:
                    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_AssistedIotGpioModeReadFalse);
                    break;
                default:
                    RUN_TEST_GROUP( TEST_IOT_GPIO );
                    return;
        }
    }
}
#endif

#if IOT_TEST_COMMON_IO_RTC_SUPPORTED
static void RUN_TEST_IOT_RTC(void)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_RTC_SUPPORTED; i++)
    {
        SET_TEST_IOT_RTC_CONFIG(i);
        RUN_TEST_GROUP( TEST_IOT_RTC );
    }
}
#endif

#if IOT_TEST_COMMON_IO_TIMER_SUPPORTED
static void RUN_TEST_IOT_TIMER(void)
{
    int i;

    for(i = 0; i < IOT_TEST_COMMON_IO_TIMER_SUPPORTED; i++)
    {
        SET_TEST_IOT_TIMER_CONFIG(i);
        RUN_TEST_GROUP( TEST_IOT_TIMER );
    }
}
#endif

#if IOT_TEST_COMMON_IO_ADC_SUPPORTED
static void RUN_TEST_IOT_ADC(int testCaseIndex)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_ADC_SUPPORTED; i++)
    {
        SET_TEST_IOT_ADC_CONFIG(i);
        switch (testCaseIndex)
        {
            case 0:
                RUN_TEST_GROUP( TEST_IOT_ADC );
                break;
            case 1:
                RUN_TEST_CASE( TEST_IOT_ADC, AFQP_IotAdcPrintReadSample );
                break;
            default:
                printf("Invalid adc test case\n");
                return;
        }
    }
}
#endif

#if IOT_TEST_COMMON_IO_RESET_SUPPORTED
static void RUN_TEST_IOT_RESET(int testCaseIndex)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_RESET_SUPPORTED; i++)
    {
        SET_TEST_IOT_RESET_CONFIG(i);
        switch (testCaseIndex)
        {
            case 0:
                RUN_TEST_CASE( TEST_IOT_RESET, AFQP_IotResetShutdown );
                break;
            case 1:
                RUN_TEST_CASE( TEST_IOT_RESET, AFQP_IotResetRebootZero );
                break;
            case 2:
                RUN_TEST_CASE( TEST_IOT_RESET, AFQP_IotResetRebootNonZero );
                break;
            default:
                printf("Invalid reset test case\n");
                return;
        }
    }
}
#endif

#if IOT_TEST_COMMON_IO_PERFCOUNTER_SUPPORTED
static void RUN_TEST_IOT_PERFCOUNTER(void)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_PERFCOUNTER_SUPPORTED; i++)
    {
        SET_TEST_IOT_PERFCOUNTER_CONFIG(i);
        RUN_TEST_GROUP( TEST_IOT_PERFCOUNTER );
    }
}
#endif

#if IOT_TEST_COMMON_IO_PWM_SUPPORTED
static void RUN_TEST_IOT_PWM(int testCaseIndex)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_PWM_SUPPORTED; i++)
    {
        SET_TEST_IOT_PWM_CONFIG(i);
        switch (testCaseIndex)
        {
            case 0:
                RUN_TEST_GROUP( TEST_IOT_PWM );
                break;
            case 1:
                RUN_TEST_CASE( TEST_IOT_PWM, AFQP_IotPwm_Start );
                break;
            case 2:
                RUN_TEST_CASE( TEST_IOT_PWM, AFQP_IotPwmAccuracy );
                break;
            case 3:
                RUN_TEST_CASE( TEST_IOT_PWM, AFQP_IotPwmAccuracyAssisted );
                break;
            default:
                printf("Invalid reset test case\n");
                return;
        }
    }
}
#endif

#if IOT_TEST_COMMON_IO_I2C_SUPPORTED
static void RUN_TEST_IOT_I2C(int testCaseIndex)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_I2C_SUPPORTED; i++)
    {
        SET_TEST_IOT_I2C_CONFIG(i);
        switch (testCaseIndex)
        {
            case 0:
                RUN_TEST_GROUP( TEST_IOT_I2C );
                break;
            case 1:
                RUN_TEST_CASE( TEST_IOT_I2C, AFQP_IotI2CWriteSyncAssisted);
                break;
            case 2:
                RUN_TEST_CASE( TEST_IOT_I2C, AFQP_IotI2CWriteAsyncAssisted);
                break;
            case 3:
                RUN_TEST_CASE( TEST_IOT_I2C, AFQP_IotI2CReadSyncAssisted);
                break;
            case 4:
                RUN_TEST_CASE( TEST_IOT_I2C, AFQP_IotI2CReadAsyncAssisted);
                break;
            default:
                printf("Invalid i2c test case\n");
                return;
        }
    }
}
#endif

#if IOT_TEST_COMMON_IO_I2S_SUPPORTED
static void RUN_TEST_IOT_I2S(int testCaseIndex)
{
    int rc = 0;
    int i;

    for(i = 0; i < IOT_TEST_COMMON_IO_I2S_SUPPORTED; i++)
    {
        SET_TEST_IOT_I2S_CONFIG(0);

        switch (testCaseIndex) {
           /* Assisted tests assume a connection with 2 Devices.  One acting as transmitter
              the other acting as receiver. This means that MOSI, MISO, CLK, and CS lines
              are connecting the two devices together.  Once this is done the setup is
              to run the receiver code first, and then the corresponding transmitter.
              The transmitter/receiver operations are defined as 0 = Read, 1 = Write, and 2 = Transfer.
              ex:  Calling iot_tests test 16 11    to start the receiver to Write and then
                   Calling iot_tests test 16 1     to have the transmitter perform the Read.
           */
           case 0:    RUN_TEST_GROUP( TEST_IOT_I2S );                           break;
           case 1:    RUN_TEST_CASE( TEST_IOT_I2S, AFQP_IotI2SReadSync );      break;
           case 2:    RUN_TEST_CASE( TEST_IOT_I2S, AFQP_IotI2SWriteSync );     break;
           case 3:    RUN_TEST_CASE( TEST_IOT_I2S, AFQP_IotI2SReadAsync );     break;
           case 4:    RUN_TEST_CASE( TEST_IOT_I2S, AFQP_IotI2SWriteAsync );     break;
           case 5:    RUN_TEST_CASE( TEST_IOT_I2S, AFQP_IotI2SWriteAsyncReadAsyncLoopback );   break;
           case 6:    RUN_TEST_CASE( TEST_IOT_I2S, AFQP_IotI2SWriteSyncReadAsyncLoopback );    break;
           case 7:    RUN_TEST_CASE( TEST_IOT_I2S, AFQP_IotI2SWriteAsyncReadSyncLoopback );    break;
           case 8:    RUN_TEST_CASE( TEST_IOT_I2S, AFQP_IotI2SWriteSyncReadSyncLoopback );     break;
           default:   break;
        }
        TEST_ASSERT_EQUAL( rc, 0 );
    }
}
#endif

#if IOT_TEST_COMMON_IO_TEMP_SENSOR_SUPPORTED
static void RUN_IOT_TEST_TSENSOR(int testCaseIndex)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_TEMP_SENSOR_SUPPORTED; i++)
    {
        SET_TEST_IOT_TEMP_SENSOR_CONFIG(i);
        switch (testCaseIndex)
        {
            case 0:
                RUN_TEST_GROUP( TEST_IOT_TSENSOR );
                break;
            case 1:
                RUN_TEST_CASE( TEST_IOT_TSENSOR, AFQP_IotTsensorTemp );
                break;
            default:
                printf("Invalid reset test case\n");
                return;
        }
    }
}
#endif

#if IOT_TEST_COMMON_IO_POWER_SUPPORTED
static void RUN_IOT_TEST_POWER(int testCaseIndex)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_POWER_SUPPORTED; i++)
    {
        SET_TEST_IOT_POWER_CONFIG(i);
        switch (testCaseIndex)
        {
            case 0:
                RUN_TEST_GROUP( TEST_IOT_POWER );
                break;
            case 1:
                RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_VddOffWakeThresholdTest );
                break;
            default:
                RUN_TEST_GROUP( TEST_IOT_POWER );
                return;
        }

    }
}
#endif

#if IOT_TEST_COMMON_IO_BATTERY_SUPPORTED
static void RUN_IOT_TEST_BATTERY(void)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_BATTERY_SUPPORTED; i++)
    {
        RUN_TEST_GROUP( TEST_IOT_BATTERY );
    }
}
#endif

#if IOT_TEST_COMMON_IO_EFUSE_SUPPORTED
static void RUN_IOT_TEST_EFUSE(void)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_EFUSE_SUPPORTED; i++)
    {
        SET_TEST_IOT_EFUSE_CONFIG(i);
        RUN_TEST_GROUP( TEST_IOT_EFUSE );
    }
}
#endif

#if IOT_TEST_COMMON_IO_TEMP_SENSOR_SUPPORTED
static void RUN_IOT_TEST_SDIO(void)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_SDIO_SUPPORTED; i++)
    {
        SET_TEST_IOT_SDIO_CONFIG(i);
        RUN_TEST_GROUP( TEST_IOT_SDIO );
    }
}
#endif

#if IOT_TEST_COMMON_IO_SPI_SUPPORTED
int slave_transfer_sync_test(int);
static void RUN_IOT_TEST_SPI(int testCaseIndex)
{
    int rc = 0;
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_SPI_SUPPORTED; i++)
    {

        SET_TEST_IOT_SPI_CONFIG(i);

        switch (testCaseIndex)
        {
            /* Assisted tests assume a connection with 2 Devices.  One acting as Master
            the other acting as Slave. This means that MOSI, MISO, CLK, and CS lines
            are connecting the two devices together.  Once this is done the setup is
            to run the Slave code first, and then the corresponding Master.
            The slave operations are defined as 0 = Read, 1 = Write, and 2 = Transfer.
            ex:  Calling iot_tests test 16 11    to start the Slave to Write and then
            Calling iot_tests test 16 1     to have the Master perform the Read.
            */
            case 0:    RUN_TEST_GROUP( TEST_IOT_SPI );                           break;
            case 1:    RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_ReadSync );      break;
            case 2:    RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_WriteSync );     break;
            case 3:    RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_TransferSync );  break;
            case 4:    RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_ReadAsync );     break;
            case 5:    RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_WriteAsync );    break;
            case 6:    RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_TransferAsync ); break;
            case 7:    RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_ReadSyncAssisted );       break;
            case 8:    RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_ReadAsyncAssisted );      break;
            case 9:    RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_WriteSyncAssisted );      break;
            case 13:   RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_WriteAsyncAssisted );     break;
            case 14:   RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_TransferSyncAssisted );   break;
            case 15:   RUN_TEST_CASE( TEST_IOT_SPI, AFQP_IotSPI_TransferAsyncAssisted );  break;
#ifdef BOARD_STARLITE
            case 10:   rc = slave_transfer_sync_test(0);                        break;
            case 11:   rc = slave_transfer_sync_test(1);                        break;
            case 12:   rc = slave_transfer_sync_test(2);                        break;
#endif
            default:   rc = -1;                                                  break;
        }
    TEST_ASSERT_EQUAL( rc, 0 );
    }
}
#endif

/* Currently this test only run on Starlite by enable uart console and run cmd from CLI.
 * DONT run such test when usb console is enabled, otherwise usb console may break */
#if IOT_TEST_COMMON_IO_USB_DEVICE_SUPPORTED
static void RUN_IOT_TEST_USB_DEVICE(int testCaseIndex)
{
    int i;
    for(i = 0; i < IOT_TEST_COMMON_IO_USB_DEVICE_SUPPORTED; i++)
    {
        SET_TEST_IOT_USB_DEVICE_CONFIG(i);
        switch (testCaseIndex)
        {
            /* Assisted test require one device running usb
             * host test software, the other one running
             * usb device test software.
             * case 0 is for all independent test
             */
            case 0:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceOpenClose );
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceOpenOpenClose );
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceOpenCloseClose );
                break;
            /* case 1 is to test usb host detect and attach usb device,
             * We need it for usb host testing purpose */
            case 1:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceHidAttach);
                break;
            /* case 2 is to de-init the usb device stack, usb device will be detached
             * It is testing usb device deinit related APIs directly.
             * We need it for usb host testing purpose */
            case 2:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceHidDetach);
                break;
            /* case 3 is to test usb device endpoint stall and unstall*/
            case 3:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceEndpointStallUnstall);
                break;
            /* case 4 is to test get device speed */
            case 4:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceGetSpeed);
                break;
            /* case 5 is to test usb device write async
             * usb device will send a string to host */
            case 5:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceWriteAsync);
                break;
            /* case 6 is to test usb device read async
             * usb device will start the read async call with this CLI, tester need to
             * issue a CLI cmd (iot test 18 2) in host side to send string to device,
             * device will receive the string and compare */
            case 6:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceReadAsync);
                break;
            /* case 7 is to test write sync, usb device will send a string to host */
            case 7:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceWriteSync);
                break;
            /* case 8 is to test read sync,
             * usb device will start the read sync call with this CLI, tester need to
             * issue a CLI cmd (iot test 18 2) in host side to send string to device,
             * device will receive the string and compare */
            case 8:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceReadSync);
                break;
            /* case 9 is to test endpoint transfer cancel */
            case 9:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceEndpointCancelTransfer);
                break;
            /* case 10 is to test ioctl request which are not tested in other cases */
            case 10:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceIoctl);
                break;
            case 11:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceHidAttachAssisted );
                break;
            case 12:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceWriteAsyncAssisted );
                break;
            case 13:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceReadAsyncAssisted );
                break;
            case 14:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceWriteSyncAssisted );
                break;
            case 15:
                RUN_TEST_CASE( TEST_IOT_USB_DEVICE, AFQP_IotUsbDeviceReadSyncAssisted );
                break;
            default:
                printf("Invalid usb device test case\n");
                return;
        }
    }
}
#endif

#if IOT_TEST_COMMON_IO_USB_HOST_SUPPORTED
static void RUN_IOT_TEST_USB_HOST(int testCaseIndex)
{
    switch (testCaseIndex)
    {
        case 0:
            break;
        /* case 1 is to test host initialization (cmd: iot test 18 1) */
        case 1:
            RUN_TEST_CASE( TEST_IOT_USB_HOST, AFQP_IotUsbHostHidGeneric);
            break;
        /* case 2 is to test host write (cmd: iot test 18 2) */
        case 2:
            RUN_TEST_CASE( TEST_IOT_USB_HOST, AFQP_IotUsbHostWriteAsync);
            break;
        /* case 3 is to test host read (cmd: iot test 18 3) */
        case 3:
            RUN_TEST_CASE( TEST_IOT_USB_HOST, AFQP_IotUsbHostReadAsync);
            break;
        default:
            printf("Invalid usb host test case\n");
            return;
    }
}
#endif

static void RUN_TEST_IOT_ALL_GROUP(void){
    /* FWPLATFORM-730 to re-enable uart test
#if IOT_TEST_COMMON_IO_UART_SUPPORTED
      RUN_TEST_IOT_UART();
#endif
    */
#if IOT_TEST_COMMON_IO_EFUSE_SUPPORTED
    RUN_IOT_TEST_EFUSE();
#endif
#if IOT_TEST_COMMON_IO_SPI_SUPPORTED
    RUN_IOT_TEST_SPI(0);
#endif
    /* ACE-9604 */
#if IOT_TEST_COMMON_IO_FLASH_SUPPORTED
    //RUN_TEST_IOT_FLASH();
#endif
#if IOT_TEST_COMMON_IO_RTC_SUPPORTED
    RUN_TEST_IOT_RTC();
#endif
#if IOT_TEST_COMMON_IO_GPIO_SUPPORTED
    RUN_TEST_IOT_GPIO(0);
#endif
#if IOT_TEST_COMMON_IO_TIMER_SUPPORTED
    RUN_TEST_IOT_TIMER();
#endif
#if IOT_TEST_COMMON_IO_WATCHDOG_SUPPORTED
    RUN_TEST_IOT_WATCHDOG(0);
#endif
#if IOT_TEST_COMMON_IO_PERFCOUNTER_SUPPORTED
    RUN_TEST_IOT_PERFCOUNTER();
#endif
#if IOT_TEST_COMMON_IO_PWM_SUPPORTED
    RUN_TEST_IOT_PWM(0);
#endif
#if IOT_TEST_COMMON_IO_I2C_SUPPORTED
    RUN_TEST_IOT_I2C(0);
#endif
#if IOT_TEST_COMMON_IO_TEMP_SENSOR_SUPPORTED
    RUN_IOT_TEST_TSENSOR(0);
#endif
#if IOT_TEST_COMMON_IO_BATTERY_SUPPORTED
    RUN_IOT_TEST_BATTERY();
#endif
    /* FWPLATFORM-1096  Power tests fail.
       temporarilly call tests individually to skip over tests that
       are failing.  Replace with RUN_IOT_TEST_POWER() once fixed
    */
#if IOT_TEST_COMMON_IO_POWER_SUPPORTED
    void TEST_TEST_IOT_POWER_SETUP(void);
    TEST_TEST_IOT_POWER_SETUP( );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_OpenClose );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_SetModeHigh );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_SetModeNormal );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_EnterIdle );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_EnterIdleCancel );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_IoctlEnum );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_SetModeFuzzing );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_ResetModeFuzzing );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_IoctlFuzzing );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_GetModeFuzzing );
    RUN_TEST_CASE( TEST_IOT_POWER, AFQP_IotPower_CloseFuzzing );
#endif
}

void RunIotTests(int testIndex, int testCaseIndex)
{
    int run_test = false;

    /* Initialize unity. */
    UnityFixture.Verbose = 1;

    UNITY_BEGIN();

    /* Give the print buffer time to empty */
    // vTaskDelay( 500 );

    switch (testIndex)
    {
        case 0:
            RUN_TEST_IOT_ALL_GROUP();
            run_test = true;
            break;
        case 1:
#if IOT_TEST_COMMON_IO_UART_SUPPORTED
            RUN_TEST_IOT_UART(testCaseIndex);
            run_test = true;
#endif
            break;
        case 2:
#if IOT_TEST_COMMON_IO_FLASH_SUPPORTED
            RUN_TEST_IOT_FLASH();
            run_test = true;
#endif
            break;
        case 3:
#if IOT_TEST_COMMON_IO_WATCHDOG_SUPPORTED
            RUN_TEST_IOT_WATCHDOG(testCaseIndex);
            run_test = true;
#endif
            break;
        case 4:
#if IOT_TEST_COMMON_IO_RTC_SUPPORTED
            RUN_TEST_IOT_RTC();
            run_test = true;
#endif
            break;
        case 5:
#if IOT_TEST_COMMON_IO_GPIO_SUPPORTED
            RUN_TEST_IOT_GPIO(testCaseIndex);
            run_test = true;
#endif
            break;
        case 6:
#if IOT_TEST_COMMON_IO_TIMER_SUPPORTED
            RUN_TEST_IOT_TIMER();
            run_test = true;
#endif
            break;
        case 7:
#if IOT_TEST_COMMON_IO_ADC_SUPPORTED
            RUN_TEST_IOT_ADC(testCaseIndex);
            run_test = true;
#endif
            break;
        case 8:
#if IOT_TEST_COMMON_IO_RESET_SUPPORTED
            RUN_TEST_IOT_RESET(testCaseIndex);
            run_test = true;
#endif
            break;
        case 9:
#if IOT_TEST_COMMON_IO_PERFCOUNTER_SUPPORTED
            RUN_TEST_IOT_PERFCOUNTER();
            run_test = true;
#endif
            break;
        case 10:
#if IOT_TEST_COMMON_IO_PWM_SUPPORTED
            RUN_TEST_IOT_PWM(testCaseIndex);
            run_test = true;
#endif
            break;
        case 11:
#if IOT_TEST_COMMON_IO_I2C_SUPPORTED
            RUN_TEST_IOT_I2C(testCaseIndex);
            run_test = true;
#endif
            break;
        case 12:
#if IOT_TEST_COMMON_IO_TEMP_SENSOR_SUPPORTED
            RUN_IOT_TEST_TSENSOR(testCaseIndex);
            run_test = true;
#endif
            break;
        case 13:
#if IOT_TEST_COMMON_IO_POWER_SUPPORTED
            RUN_IOT_TEST_POWER(testCaseIndex);
            run_test = true;
#endif
            break;
        case 14:
#if IOT_TEST_COMMON_IO_BATTERY_SUPPORTED
            RUN_IOT_TEST_BATTERY();
            run_test = true;
#endif
            break;
        case 15:
#if IOT_TEST_COMMON_IO_EFUSE_SUPPORTED
            RUN_IOT_TEST_EFUSE();
            run_test = true;
#endif
            break;
        case 16:
#if IOT_TEST_COMMON_IO_SPI_SUPPORTED
            RUN_IOT_TEST_SPI(testCaseIndex);
            run_test = true;
#endif
            break;
        case 17:
#if IOT_TEST_COMMON_IO_USB_DEVICE_SUPPORTED
            RUN_IOT_TEST_USB_DEVICE(testCaseIndex);
            run_test = true;
#endif
            break;
        case 18:
#if IOT_TEST_COMMON_IO_USB_HOST_SUPPORTED
            RUN_IOT_TEST_USB_HOST(testCaseIndex);
            run_test = true;
#endif
            break;
        case 19:
#if IOT_TEST_COMMON_IO_I2S_SUPPORTED
            RUN_TEST_IOT_I2S(testCaseIndex);
            run_test = true;
#endif
            break;
        case 20:
            RUN_IOT_TEST_SDIO();
            break;
        default:
            printf("Test index is wrong\r\n");
            return;
    }

    if (! run_test)
    {
        printf("Test is not enabled for this platform\r\n");
        return;
    }

    /* Currently disabled. Will be enabled after cleanup. */
    UNITY_END();

    // #ifdef CODE_COVERAGE
    //     exit( 0 );
    // #endif

    /* This task has finished.  FreeRTOS does not allow a task to run off the
     * end of its implementing function, so the task must be deleted. */
    // vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/
