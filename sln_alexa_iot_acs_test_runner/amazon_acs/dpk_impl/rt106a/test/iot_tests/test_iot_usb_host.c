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
 * IOT Functional Unit Test - USB Host
 *******************************************************************************
 */

#ifdef BOARD_STARLITE
/* Todo: remove all nxp code and implement the host test based on iot usb host hal */
/* Test includes */
#include "unity.h"
#include "unity_fixture.h"

#include "fsl_device_registers.h"
#include "fsl_common.h"
#include "pin_mux.h"
#include "board.h"
#include "usb_phy.h"
#include "fsl_power.h"
#include "clock_config.h"
#include "usb_host_config.h"
#include "usb_host.h"
#include "usb_host_hid.h"
/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "semphr.h"
#include "string.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "semphr.h"

#define testIotUsbHost_CONTROLLER_ID kUSB_ControllerIp3516Hs0
#define testIotUsbHost_INTERRUPT_PRIORITY (6U)

#define testIotUsbHost_HID_GENERIC_IN_BUFFER_SIZE (100U)
#define testIotUsbHost_HID_GENERIC_OUT_BUFFER_SIZE (8U)

#define testIotUsbHost_TASK_NAME "usb_host"
#define testIotUsb_TASK_NAME "usb_host_app"

#define testIotUsbHost_TEST_WAIT_TIME (1000) /* in ms */

/*! @brief host app device attach/detach status */
typedef enum
{
    eUSBHostStatusDevIdle = 0, /*!< there is no device attach/detach */
    eUSBHostStatusDevAttached, /*!< device is attached */
    eUSBHostStatusDevDetached, /*!< device is detached */
} USBHostAppState_t;

typedef enum
{
    eUSBHostHidRundIdle = 0,                /*!< idle */
    eUSBHostHidRunSetInterface,            /*!< execute set interface code */
    eUSBHostHidRunWaitSetInterface,        /*!< wait set interface done */
    eUSBHostHidRunSetInterfaceDone,        /*!< set interface is done, execute next step */
    eUSBHostHidRunWaitSetIdle,             /*!< wait set idle done */
    eUSBHostHidRunSetIdleDone,             /*!< set idle is done, execute next step */
    eUSBHostHidRunWaitGetReportDescriptor, /*!< wait get report descriptor done */
    eUSBHostHidRunGetReportDescriptorDone, /*!< get report descriptor is done, execute next step */
    eUSBHostHidRunWaitSetProtocol,         /*!< wait set protocol done */
    eUSBHostHidRunSetProtocolDone,         /*!< set protocol is done, execute next step */
    eUSBHostHidRunWaitDataReceived,        /*!< wait interrupt in data */
    eUSBHostHidRunDataReceived,            /*!< interrupt in data received */
    eUSBHostHidRunPrimeDataReceive,        /*!< prime interrupt in receive */
} USBHostHidGenericRunState_t;

typedef struct
{
    usb_host_configuration_handle xConfigHandle; /*!< the hid generic's configuration handle */
    usb_device_handle xDeviceHandle;             /*!< the hid generic's device handle */
    usb_host_class_handle xClassHandle;          /*!< the hid generic's class handle */
    usb_host_interface_handle xInterfaceHandle;  /*!< the hid generic's interface handle */
    uint8_t ucDeviceState;                        /*!< device attach/detach status */
    uint8_t ucPrevState;                          /*!< device attach/detach previous status */
    uint8_t ucRunState;                           /*!< hid generic application run status */
    uint8_t
        ucRunWaitState; /*!< hid generic application wait status, go to next run status when the wait status success */
    uint16_t usInMaxPacketSize;  /*!< Interrupt in max packet size */
    uint16_t usOutMaxPacketSize; /*!< Interrupt out max packet size */
    uint8_t *pucGenericInBuffer;
    uint8_t *pucGenericOutBuffer;
    uint16_t usSendIndex; /*!< data sending position */
} USBHostHidGenericInstance_t;

void USB_HostApplicationInit(void);
void USB_HostHidGenericTask(void *param);
usb_status_t USB_HostHidGenericEvent(usb_device_handle xDeviceHandle,
                                            usb_host_configuration_handle configurationHandle,
                                            uint32_t eventCode);
static void USB_HostHidGenericProcessBuffer(USBHostHidGenericInstance_t *genericInstance);


static usb_status_t USB_HostEvent(usb_device_handle xDeviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode);

/* device IRQ*/
extern void USB1_IRQHandler(void);
/* host IRQ */
void USB0_HostIRQHandler(void);
/*!
 * @brief app initialization.
 */
void USB_HostApplicationInit(void);

extern void USB_HostClockInit(void);
extern void USB_HostIsrEnable(void);
extern void USB_HostTaskFn(void *param);
/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t ucUsbHostEnabled = 0;
USBHostHidGenericInstance_t xHostHidGeneric; /* hid generic instance */
uint8_t ucTestDataHost[] = "Test string from host\r\n";
uint8_t ucTestDataDevice[] = "Test string from device\r\n";
/* string to send to device */
uint8_t ucTestDataWrite[] = "receive from host\r\n";
uint8_t ucTestDataRead[] = "send from device\r\n";
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t ucGenericInBuffer[testIotUsbHost_HID_GENERIC_IN_BUFFER_SIZE]; /*!< use to receive report descriptor and data */
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t ucGenericOutBuffer[testIotUsbHost_HID_GENERIC_IN_BUFFER_SIZE]; /*!< use to send data */
static void USB_HostHidControlCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status);
static void USB_HostHidInCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status);
static void USB_HostHidOutCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status);
static usb_status_t USB_HostHidGenericPrepareOutData(USBHostHidGenericInstance_t *genericInstance);
static SemaphoreHandle_t xtestIotUsbHostSemaphore = NULL;

extern USBHostHidGenericInstance_t xHostHidGeneric;
usb_host_handle g_HostHandle;

void USB0_IRQHandler(void)
{
    if (ucUsbHostEnabled)
    {
        USB0_HostIRQHandler();
    }
    else
    {
        USB1_IRQHandler();
    }
}

static void USB_HostHidGenericProcessBuffer(USBHostHidGenericInstance_t *genericInstance)
{
    genericInstance->pucGenericInBuffer[genericInstance->usInMaxPacketSize] = 0;
}

static void USB_HostHidControlCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    USBHostHidGenericInstance_t *genericInstance = (USBHostHidGenericInstance_t *)param;

    if (kStatus_USB_TransferStall == status)
    {
        printf("device don't support this ruquest \r\n");
    }
    else if (kStatus_USB_Success != status)
    {
        printf("control transfer failed\r\n");
    }
    else
    {
    }

    if (genericInstance->ucRunWaitState == eUSBHostHidRunWaitSetInterface) /* set interface finish */
    {
        genericInstance->ucRunState = eUSBHostHidRunSetInterfaceDone;
    }
    else if (genericInstance->ucRunWaitState == eUSBHostHidRunWaitSetIdle) /* hid set idle finish */
    {
        genericInstance->ucRunState = eUSBHostHidRunSetIdleDone;
    }
    else if (genericInstance->ucRunWaitState ==
             eUSBHostHidRunWaitGetReportDescriptor) /* hid get report descriptor finish */
    {
        genericInstance->ucRunState = eUSBHostHidRunGetReportDescriptorDone;
    }
    else if (genericInstance->ucRunWaitState == eUSBHostHidRunWaitSetProtocol) /* hid set protocol finish */
    {
        genericInstance->ucRunState = eUSBHostHidRunSetProtocolDone;
    }
    else
    {
    }
}

static void USB_HostHidInCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    USBHostHidGenericInstance_t *genericInstance = (USBHostHidGenericInstance_t *)param;

    if (genericInstance->ucRunWaitState == eUSBHostHidRunWaitDataReceived)
    {
        if (status == kStatus_USB_Success)
        {
            genericInstance->ucRunState = eUSBHostHidRunDataReceived; /* go to process data */
        }
        else
        {
            if (genericInstance->ucDeviceState == eUSBHostStatusDevAttached)
            {
                genericInstance->ucRunState = eUSBHostHidRunPrimeDataReceive; /* go to prime next receiving */
            }
        }
    }
}

static void USB_HostHidOutCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{ /* NULL */
}

static usb_status_t USB_HostHidGenericPrepareOutData(USBHostHidGenericInstance_t *genericInstance)
{
    uint16_t index = 0;

    if (genericInstance->usSendIndex < (sizeof(ucTestDataHost) - 1)) /* usSendIndex indicate the current position of ucTestDataHost */
    {
        /* get the max packet data, note: the data should be 0 when there is no actual data to send */
        for (index = 0; ((index + genericInstance->usSendIndex) < (sizeof(ucTestDataHost) - 1)) &&
                        (index < genericInstance->usOutMaxPacketSize);
             ++index)
        {
            genericInstance->pucGenericOutBuffer[index] = ucTestDataHost[index + genericInstance->usSendIndex];
        }
        for (; index < genericInstance->usOutMaxPacketSize; ++index)
        {
            genericInstance->pucGenericOutBuffer[index] = 0x00;
        }
        genericInstance->usSendIndex += genericInstance->usOutMaxPacketSize;

        return kStatus_USB_Success;
    }
    else
    {
        return kStatus_USB_Error; /* there is no data to send */
    }
}

void USB_HostHidGenericTask(void *param)
{
    usb_status_t status;
    usb_host_hid_descriptor_t *hidDescriptor;
    uint32_t hidReportLength = 0;
    uint8_t *descriptor;
    uint32_t endPosition;
    BaseType_t xHigherPriorityTaskWoken;
    USBHostHidGenericInstance_t *genericInstance = (USBHostHidGenericInstance_t *)param;

    /* device state changes, process once for each state */
    if (genericInstance->ucDeviceState != genericInstance->ucPrevState)
    {
        genericInstance->ucPrevState = genericInstance->ucDeviceState;
        switch (genericInstance->ucDeviceState)
        {
            case eUSBHostStatusDevIdle:
                break;

            case eUSBHostStatusDevAttached: /* deivce is attached and numeration is done */
                genericInstance->ucRunState = eUSBHostHidRunSetInterface;
                if (USB_HostHidInit(genericInstance->xDeviceHandle, &genericInstance->xClassHandle) !=
                    kStatus_USB_Success)
                {
                    printf("host hid class initialize fail\r\n");
                }
                else
                {
                    printf("hid generic attached\r\n");
                }
                genericInstance->usSendIndex = 0;
                break;

            case eUSBHostStatusDevDetached: /* device is detached */
                genericInstance->ucDeviceState = eUSBHostStatusDevIdle;
                genericInstance->ucRunState = eUSBHostHidRundIdle;
                USB_HostHidDeinit(genericInstance->xDeviceHandle, genericInstance->xClassHandle);
                genericInstance->xClassHandle = NULL;
                printf("hid generic detached\r\n");
                break;

            default:
                break;
        }
    }

    /* run state */
    switch (genericInstance->ucRunState)
    {
        case eUSBHostHidRundIdle:
            break;

        case eUSBHostHidRunSetInterface: /* 1. set hid interface */
            genericInstance->ucRunWaitState = eUSBHostHidRunWaitSetInterface;
            genericInstance->ucRunState = eUSBHostHidRundIdle;
            if (USB_HostHidSetInterface(genericInstance->xClassHandle, genericInstance->xInterfaceHandle, 0,
                                        USB_HostHidControlCallback, genericInstance) != kStatus_USB_Success)
                {
                printf("set interface error\r\n");
            }
            break;

        case eUSBHostHidRunSetInterfaceDone: /* 2. hid set idle */
            genericInstance->usInMaxPacketSize =
                USB_HostHidGetPacketsize(genericInstance->xClassHandle, USB_ENDPOINT_INTERRUPT, USB_IN);
            genericInstance->usOutMaxPacketSize =
                USB_HostHidGetPacketsize(genericInstance->xClassHandle, USB_ENDPOINT_INTERRUPT, USB_OUT);

            /* first: set idle */
            genericInstance->ucRunWaitState = eUSBHostHidRunWaitSetIdle;
            genericInstance->ucRunState = eUSBHostHidRundIdle;
            if (USB_HostHidSetIdle(genericInstance->xClassHandle, 0, 0, USB_HostHidControlCallback, genericInstance) !=
                kStatus_USB_Success)
            {
                printf("Error in USB_HostHidSetIdle\r\n");
            }
            break;

        case eUSBHostHidRunSetIdleDone: /* 3. hid get report descriptor */
            /* get report descriptor's length */
            hidDescriptor = NULL;
            descriptor = (uint8_t *)((usb_host_interface_t *)genericInstance->xInterfaceHandle)->interfaceExtension;
            endPosition = (uint32_t)descriptor +
                          ((usb_host_interface_t *)genericInstance->xInterfaceHandle)->interfaceExtensionLength;

            while ((uint32_t)descriptor < endPosition)
            {
                if (*(descriptor + 1) == USB_DESCRIPTOR_TYPE_HID) /* descriptor type */
                {
                    hidDescriptor = (usb_host_hid_descriptor_t *)descriptor;
                    break;
                }
                else
                {
                    descriptor = (uint8_t *)((uint32_t)descriptor + (*descriptor)); /* next descriptor */
                }
            }

            if (hidDescriptor != NULL)
            {
                usb_host_hid_class_descriptor_t *hidClassDescriptor;
                hidClassDescriptor = (usb_host_hid_class_descriptor_t *)&(hidDescriptor->bHidDescriptorType);
                for (uint8_t index = 0; index < hidDescriptor->bNumDescriptors; ++index)
                {
                    hidClassDescriptor += index;
                    if (hidClassDescriptor->bHidDescriptorType == USB_DESCRIPTOR_TYPE_HID_REPORT)
                    {
                        hidReportLength =
                            (uint16_t)USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(hidClassDescriptor->wDescriptorLength);
                        break;
                    }
                }
            }
            if (hidReportLength > testIotUsbHost_HID_GENERIC_IN_BUFFER_SIZE)
            {
                printf("hid buffer is too small\r\n");
                genericInstance->ucRunState = eUSBHostHidRundIdle;
                return;
            }

            genericInstance->ucRunWaitState = eUSBHostHidRunWaitGetReportDescriptor;
            genericInstance->ucRunState = eUSBHostHidRundIdle;
            /* second: get report descriptor */
            USB_HostHidGetReportDescriptor(genericInstance->xClassHandle, genericInstance->pucGenericInBuffer,
                                           hidReportLength, USB_HostHidControlCallback, genericInstance);
            break;

        case eUSBHostHidRunGetReportDescriptorDone: /* 4. hid set protocol */
            genericInstance->ucRunWaitState = eUSBHostHidRunWaitSetProtocol;
            genericInstance->ucRunState = eUSBHostHidRundIdle;
            /* third: set protocol */
            if (USB_HostHidSetProtocol(genericInstance->xClassHandle, USB_HOST_HID_REQUEST_PROTOCOL_REPORT,
                                       USB_HostHidControlCallback, genericInstance) != kStatus_USB_Success)
            {
                printf("Error in USB_HostHidSetProtocol\r\n");
            }
            break;

        case eUSBHostHidRunSetProtocolDone: /* 5. start to receive data and send data */
            genericInstance->ucRunWaitState = eUSBHostHidRunWaitDataReceived;
            genericInstance->ucRunState = eUSBHostHidRundIdle;
            if (USB_HostHidRecv(genericInstance->xClassHandle, genericInstance->pucGenericInBuffer,
                                genericInstance->usInMaxPacketSize, USB_HostHidInCallback,
                                genericInstance) != kStatus_USB_Success)
            {
                printf("Error in USB_HostHidRecv\r\n");
            }
            status = USB_HostHidGenericPrepareOutData(genericInstance);
            break;
        case eUSBHostHidRunDataReceived: /* process received data, receive next data and send next data */
            xSemaphoreGiveFromISR(xtestIotUsbHostSemaphore, &xHigherPriorityTaskWoken);
            break;

        case eUSBHostHidRunPrimeDataReceive: /* receive next data and send next data */
            genericInstance->ucRunWaitState = eUSBHostHidRunWaitDataReceived;
            genericInstance->ucRunState = eUSBHostHidRundIdle;
            if (USB_HostHidRecv(genericInstance->xClassHandle, genericInstance->pucGenericInBuffer,
                                genericInstance->usInMaxPacketSize, USB_HostHidInCallback,
                                genericInstance) != kStatus_USB_Success)
            {
                printf("Error in USB_HostHidRecv\r\n");
            }
            status = USB_HostHidGenericPrepareOutData(genericInstance);
            break;
        default:
            break;
    }
}


usb_status_t USB_HostHidGenericEvent(usb_device_handle xDeviceHandle,
                                     usb_host_configuration_handle configurationHandle,
                                     uint32_t eventCode)
{
    uint32_t pid;
    uint32_t vid;
    usb_host_configuration_t *configuration;
    usb_host_interface_t *interface;
    uint32_t infoValue;
    usb_status_t status = kStatus_USB_Success;
    uint8_t interfaceIndex;
    uint8_t id;

    switch (eventCode)
    {
        case kUSB_HostEventAttach:
            /* judge whether is configurationHandle supported */
            configuration = (usb_host_configuration_t *)configurationHandle;
            for (interfaceIndex = 0; interfaceIndex < configuration->interfaceCount; ++interfaceIndex)
            {
                interface = &configuration->interfaceList[0];
                id = interface->interfaceDesc->bInterfaceClass;
                if (id != USB_HOST_HID_CLASS_CODE)
                {
                    continue;
                }
                id = interface->interfaceDesc->bInterfaceSubClass;
                if ((id != USB_HOST_HID_SUBCLASS_CODE_NONE) && (id != USB_HOST_HID_SUBCLASS_CODE_BOOT))
                {
                    continue;
                }
                USB_HostHelperGetPeripheralInformation(xDeviceHandle, kUSB_HostGetDevicePID, &pid);
                USB_HostHelperGetPeripheralInformation(xDeviceHandle, kUSB_HostGetDeviceVID, &vid);
                if ((pid == 0x00a2) && (vid == 0x1fc9))
                {
                    if (xHostHidGeneric.ucDeviceState == eUSBHostStatusDevIdle)
                    {
                        /* the interface is supported by the application */
                        xHostHidGeneric.pucGenericInBuffer = ucGenericInBuffer;
                        xHostHidGeneric.pucGenericOutBuffer = ucGenericOutBuffer;
                        xHostHidGeneric.xDeviceHandle = xDeviceHandle;
                        xHostHidGeneric.xInterfaceHandle = interface;
                        xHostHidGeneric.xConfigHandle = configurationHandle;
                        return kStatus_USB_Success;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
            status = kStatus_USB_NotSupported;
            break;

        case kUSB_HostEventNotSupported:
            break;
        case kUSB_HostEventEnumerationDone:
            if (xHostHidGeneric.xConfigHandle == configurationHandle)
            {
                if ((xHostHidGeneric.xDeviceHandle != NULL) && (xHostHidGeneric.xInterfaceHandle != NULL))
                {
                    /* the device enumeration is done */
                    if (xHostHidGeneric.ucDeviceState == eUSBHostStatusDevIdle)
                    {
                        xHostHidGeneric.ucDeviceState = eUSBHostStatusDevAttached;

                        USB_HostHelperGetPeripheralInformation(xDeviceHandle, kUSB_HostGetDevicePID, &infoValue);
                        printf("hid generic attached:pid=0x%x", infoValue);
                        USB_HostHelperGetPeripheralInformation(xDeviceHandle, kUSB_HostGetDeviceVID, &infoValue);
                        printf("vid=0x%x ", infoValue);
                        USB_HostHelperGetPeripheralInformation(xDeviceHandle, kUSB_HostGetDeviceAddress, &infoValue);
                        printf("address=%d\r\n", infoValue);
                    }
                    else
                    {
                        printf("not idle generic instance\r\n");
                        status = kStatus_USB_Error;
                    }
                }
            }
            break;

        case kUSB_HostEventDetach:
            if (xHostHidGeneric.xConfigHandle == configurationHandle)
            {
                /* the device is detached */
                xHostHidGeneric.xConfigHandle = NULL;
                if (xHostHidGeneric.ucDeviceState != eUSBHostStatusDevIdle)
                {
                    xHostHidGeneric.ucDeviceState = eUSBHostStatusDevDetached;
                }
            }
            break;

        default:
            break;
    }
    return status;
}

void USB0_HostIRQHandler(void)
{
    USB_HostIp3516HsIsrFunction(g_HostHandle);
}

void USB_HostClockInit(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL, BOARD_USB_PHY_TXCAL45DP, BOARD_USB_PHY_TXCAL45DM,
    };
    /* enable USB IP clock */
    CLOCK_EnableUsbhsHostClock();
    /*Make sure USDHC ram buffer has power up*/
    POWER_DisablePD(kPDRUNCFG_APD_USBHS_SRAM);
    POWER_DisablePD(kPDRUNCFG_PPD_USBHS_SRAM);
    POWER_ApplyPD();
#if CPU_MIMXRT685SEVKA
    /* enable usb1 phy */
    SYSCTL0->PDRUNCFG0_CLR |= SYSCTL0_PDRUNCFG0_USB_PD_MASK;
#endif
    CLOCK_EnableUsbhsPhyClock();

#if ((defined FSL_FEATURE_USBHSH_USB_RAM) && (FSL_FEATURE_USBHSH_USB_RAM > 0U))

    for (int i = 0; i < (FSL_FEATURE_USBHSH_USB_RAM >> 2); i++)
    {
        ((uint32_t *)FSL_FEATURE_USBHSH_USB_RAM_BASE_ADDRESS)[i] = 0U;
    }
#endif
    USB_EhciPhyInit(testIotUsbHost_CONTROLLER_ID, BOARD_XTAL_SYS_CLK_HZ, &phyConfig);
}
void USB_HostIsrEnable(void)
{
    uint8_t irqNumber;

    uint8_t usbHOSTEhciIrq[] = USBHSH_IRQS;
    irqNumber = usbHOSTEhciIrq[testIotUsbHost_CONTROLLER_ID - kUSB_ControllerIp3516Hs0];
    /* USB_HOST_CONFIG_EHCI */

    /* Install isr, set priority, and enable IRQ. */
    NVIC_SetPriority((IRQn_Type)irqNumber, testIotUsbHost_INTERRUPT_PRIORITY);

    EnableIRQ((IRQn_Type)irqNumber);
}

void USB_HostTaskFn(void *param)
{
#if ((defined USB_HOST_CONFIG_IP3516HS) && (USB_HOST_CONFIG_IP3516HS > 0U))
    USB_HostIp3516HsTaskFunction(param);
#endif /* USB_HOST_CONFIG_IP3516HS */
}

static usb_status_t USB_HostEvent(usb_device_handle xDeviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode)
{
    usb_status_t status = kStatus_USB_Success;

    switch (eventCode)
    {
        case kUSB_HostEventAttach:
            status = USB_HostHidGenericEvent(xDeviceHandle, configurationHandle, eventCode);
            break;

        case kUSB_HostEventNotSupported:
            usb_echo("device not supported.\r\n");
            break;

        case kUSB_HostEventEnumerationDone:
            status = USB_HostHidGenericEvent(xDeviceHandle, configurationHandle, eventCode);
            break;

        case kUSB_HostEventDetach:
            status = USB_HostHidGenericEvent(xDeviceHandle, configurationHandle, eventCode);
            break;

        default:
            break;
    }
    return status;
}

void USB_HostApplicationInit(void)
{
    usb_status_t status = kStatus_USB_Success;

    USB_HostClockInit();

#if ((defined FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT))
    SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

    status = USB_HostInit(testIotUsbHost_CONTROLLER_ID, &g_HostHandle, USB_HostEvent);
    if (status != kStatus_USB_Success)
    {
        usb_echo("host init error\r\n");
        return;
    }
    USB_HostIsrEnable();

    usb_echo("host init done\r\n");

    ucUsbHostEnabled = true;
}

void USB_HostTask(void *handle)
{
    while(1)
    {
        USB_HostTaskFn(handle);
        vTaskDelay(50);
    }
}

void USB_HostAppTask(void *handle)
{
    usb_status_t error = kStatus_USB_Error;
    USB_HostApplicationInit();
    TaskHandle_t usbHostTaskHandle;

    if (g_HostHandle)
    {
        if (xTaskCreate(USB_HostTask, testIotUsbHost_TASK_NAME,
            5000L / sizeof(portSTACK_TYPE), g_HostHandle,
            4, &usbHostTaskHandle) != pdPASS)
        {
            printf("USB Host Task creation failed!.\r\n");
            while (1)
                ;
        }
        else
        {
            printf("USB Host Task creation ok.\r\n");
        }
    }
    while (1)
    {
        USB_HostHidGenericTask(&xHostHidGeneric);
        vTaskDelay(50);

    }
}

void USB_Init(void)

{
    TaskHandle_t usbTaskHandle;
    printf("\r\nStarting NXP USB stack\r\n");
    SYSCTL0->PDRUNCFG1_CLR = SYSCTL0_PDRUNCFG1_USBHS_SRAM_APD_MASK | SYSCTL0_PDRUNCFG1_USBHS_SRAM_PPD_MASK;
#if CPU_MIMXRT685SEVKA
    PMC->CTRL |= PMC_CTRL_APPLYCFG_MASK;
    while (PMC->STATUS & PMC_STATUS_ACTIVEFSM_MASK)
    {
    }
#else
    POWER_ApplyPD();
#endif
    if (xTaskCreate(USB_HostAppTask, testIotUsb_TASK_NAME,
            5000L / sizeof(portSTACK_TYPE), NULL,
            4, &usbTaskHandle) != pdPASS)
    {
        printf("USB Host App Task creation failed!.\r\n");
        while (1)
            ;
        return -1;
    }
    else
    {
        printf("USB Init Task ok\r\n");
    }
}

void USB_HostWriteAsyncTest(void)
{
    int32_t lRetVal;
    int32_t error = kStatus_USB_Error;
    error = USB_HostHidSend(xHostHidGeneric.xClassHandle, ucTestDataWrite,
        xHostHidGeneric.usOutMaxPacketSize, USB_HostHidOutCallback,
        &xHostHidGeneric);
    TEST_ASSERT_EQUAL( error, kStatus_USB_Success);
    //lRetVal = xSemaphoreTake( xtestIotUsbHostSemaphore, testIotUsbHost_TEST_WAIT_TIME);
    //TEST_ASSERT_EQUAL( pdTRUE, lRetVal );
}

void USB_HostReadAsyncTest(void)
{
    int32_t lRetVal;
    int32_t error = kStatus_USB_Error;
    error = USB_HostHidRecv(xHostHidGeneric.xClassHandle, xHostHidGeneric.pucGenericInBuffer,
        xHostHidGeneric.usOutMaxPacketSize, USB_HostHidInCallback,
        &xHostHidGeneric);
    TEST_ASSERT_EQUAL( error, kStatus_USB_Success);
    lRetVal = xSemaphoreTake( xtestIotUsbHostSemaphore, testIotUsbHost_TEST_WAIT_TIME);
    TEST_ASSERT_EQUAL( pdTRUE, lRetVal );
    TEST_ASSERT_EQUAL_STRING(ucTestDataRead, xHostHidGeneric.pucGenericInBuffer);
}

/* Define Test Group. */
TEST_GROUP( TEST_IOT_USB_HOST );

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_IOT_USB_HOST )
{
    xtestIotUsbHostSemaphore = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_EQUAL( NULL, xtestIotUsbHostSemaphore);
}

/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_IOT_USB_HOST )
{
}

/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_IOT_USB_HOST )
{
    RUN_TEST_CASE( TEST_IOT_USB_HOST, AFQP_IotUsbHostHidGeneric );
	RUN_TEST_CASE( TEST_IOT_USB_HOST, AFQP_IotUsbHostWriteAsync);
	RUN_TEST_CASE( TEST_IOT_USB_HOST, AFQP_IotUsbHostReadAsync);
}

/**
 * @brief Function to test hid class.
 */
TEST( TEST_IOT_USB_HOST, AFQP_IotUsbHostHidGeneric)
{
    USB_Init();
}

/**
 * @brief Function to test hid send.
 */
TEST( TEST_IOT_USB_HOST, AFQP_IotUsbHostWriteAsync)
{
	USB_HostWriteAsyncTest();
}

/**
 * @brief Function to test hid recv.
 */
TEST( TEST_IOT_USB_HOST, AFQP_IotUsbHostReadAsync)
{
    USB_HostReadAsyncTest();
}

#endif
