/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "stdint.h"
#include "iot_gpio.h"
#include "fsl_gpio.h"
#include "rt106a_iot_log.h"

static const char *TAG = "rt106a-hal-gpio";

typedef struct {
    int32_t gpio_num;
    gpio_pin_config_t iot_gpio_conf;
    void (*func)(uint8_t arg1, void *arg2);
    void *arg2;
    uint8_t ucPinState;
    uint32_t set_function_val;
} gpio_ctx_t;

static volatile uint64_t gpio_status_bitmask; //BIT Mask for GPIO
static volatile uint64_t gpio_int_status_bitmask; //BIT Mask for GPIO

typedef struct {
    int32_t gpio_num;  //used for external application, has nothing with hardware
    int32_t pin_num;  //specify GPIO pin index
    GPIO_Type *base;  //specify GPIO
} pin_num_map_2_gpio_t;

enum gpio_index {
    GPIO_INDEX_START = 0,
    LED_RED,
    LED_BLUE,
    LED_GREEN,
    GPID_INDEX_END
};

const static pin_num_map_2_gpio_t gPinsMap[] = {
        {.gpio_num = LED_RED, .pin_num = 6, .base = GPIO4}, //Red LED
        {.gpio_num = LED_BLUE, .pin_num = 8, .base = GPIO4}, //Blue LED
        {.gpio_num = LED_GREEN, .pin_num = 0, .base = GPIO1} //Green LED
};

#define MAP_LEN (sizeof(gPinsMap)/sizeof(gPinsMap[0]))

static bool IsValidPinNumber(int32_t lGpioNumber)
{
    uint32_t i;
    for (i = 0; i < MAP_LEN; i++) {
        if (gPinsMap[i].gpio_num == lGpioNumber)
            break;
        }
    if (i <= MAP_LEN)
        return true;
    else
        return false;
}

static const pin_num_map_2_gpio_t* getPinMap(int32_t lGpioNumber)
{
    uint32_t i;
    for (i = 0; i < MAP_LEN; i++) {
        if (gPinsMap[i].gpio_num == lGpioNumber)
            break;
        }
    if (i <= MAP_LEN)
        return &gPinsMap[i];
    else
        return NULL;

}

IotGpioHandle_t iot_gpio_open( int32_t lGpioNumber )
{

    if (!IsValidPinNumber( lGpioNumber)) {
        RT106A_LOGE(TAG, " %s:Invalid arguments(%u)\r\n", __func__, lGpioNumber);
        return NULL;
    }
    if (!(0x01 & gpio_status_bitmask >> lGpioNumber)) {
        gpio_ctx_t *gpio_ctx = (gpio_ctx_t *) calloc(1, sizeof(gpio_ctx_t));
        if (gpio_ctx == NULL) {
            RT106A_LOGE(TAG, "Could not allocate memory for gpio context");
            return NULL;
        }

        memset(gpio_ctx, 0, sizeof(*gpio_ctx));
        gpio_ctx->iot_gpio_conf.direction = kGPIO_DigitalOutput;
        gpio_ctx->iot_gpio_conf.outputLogic = 0;
        gpio_ctx->iot_gpio_conf.interruptMode = kGPIO_NoIntmode;
        gpio_ctx->gpio_num = lGpioNumber;

        pin_num_map_2_gpio_t *map = NULL;
        map = getPinMap(lGpioNumber); //Here lGpioNumber is valid, so not needed to chech the avaliability of map
        GPIO_PinInit(map->base, map->pin_num, &(gpio_ctx->iot_gpio_conf));
        gpio_status_bitmask = gpio_status_bitmask | (1<<gpio_ctx->gpio_num);
        IotGpioHandle_t iot_gpio_handler = (void *)gpio_ctx;
        RT106A_LOGD(TAG, "%s GPIO Number %d, GPIO bitmask %d", __func__, lGpioNumber, gpio_status_bitmask);
        return iot_gpio_handler;
    }
    RT106A_LOGE(TAG, " %s:failed (%u)\r\n", __func__, lGpioNumber);
    return NULL;
}
#if 0
static void iot_hal_gpio_set_output_mode(IotGpioOutputMode_t *gpio_set_output_mode, gpio_pin_config_t *gpio_conf)
{
    switch (*gpio_set_output_mode) {
    RT106A_LOGD(TAG, "gpio output mode: %d", *gpio_set_output_mode);
    case eGpioOpenDrain :
        gpio_conf->mode = GPIO_MODE_OUTPUT_OD;
        break;
    case eGpioPushPull :
        gpio_conf->mode = GPIO_MODE_DEF_OUTPUT;
        break;
    default :
        RT106A_LOGD(TAG, "gpio output mode not supported");
        break;
    }
}

static void iot_hal_gpio_get_output_mode(IotGpioOutputMode_t *gpio_get_output_mode, gpio_pin_config_t *gpio_conf)
{
    switch (gpio_conf->mode) {
    case GPIO_MODE_OUTPUT_OD :
        *gpio_get_output_mode = eGpioOpenDrain;
        break;
    case GPIO_MODE_DEF_OUTPUT :
        *gpio_get_output_mode = eGpioPushPull;
        break;
    default :
        RT106A_LOGD(TAG, "gpio output mode not supported");
        break;
    }
}

static void iot_hal_gpio_set_pullup_pulldown(IotGpioPull_t *gpio_set_pullup_pulldown, gpio_pin_config_t *gpio_conf)
{
    switch (*gpio_set_pullup_pulldown) {
    RT106A_LOGD(TAG, "gpio pullup/pulldown mode: %d", *gpio_set_pullup_pulldown);
    case eGpioPullNone :
        gpio_conf->pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_conf->pull_up_en = GPIO_PULLUP_DISABLE;
        break;
    case eGpioPullUp :
        gpio_conf->pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_conf->pull_up_en = GPIO_PULLUP_ENABLE;
        break;
    case eGpioPullDown :
        gpio_conf->pull_down_en = GPIO_PULLDOWN_ENABLE;
        gpio_conf->pull_up_en =  GPIO_PULLUP_DISABLE;
        break;
    default :
        RT106A_LOGD(TAG, "gpio set pullup/pulldown not supported");
        break;
    }
}

static void iot_hal_gpio_get_pullup_pulldown(IotGpioPull_t *gpio_get_pullup_pulldown, gpio_config_t *gpio_conf)
{
    if (gpio_conf->pull_down_en == GPIO_PULLDOWN_ENABLE) {
        *gpio_get_pullup_pulldown = eGpioPullDown;
    } else if (gpio_conf->pull_up_en == GPIO_PULLUP_ENABLE) {
        *gpio_get_pullup_pulldown = eGpioPullUp;
    } else {
        *gpio_get_pullup_pulldown = eGpioPullNone;
    }
}

#endif
static void iot_hal_gpio_set_interrupt(IotGpioInterrupt_t *gpio_set_interrupt, gpio_pin_config_t *gpio_conf)
{
    switch (*gpio_set_interrupt) {
    RT106A_LOGD(TAG, "gpio interrupt mode: %d", *gpio_set_interrupt);
    case eGpioInterruptNone :
        gpio_conf->interruptMode = kGPIO_NoIntmode;
        break;
    case eGpioInterruptRising :
        gpio_conf->interruptMode = kGPIO_IntRisingEdge;
        break;
    case eGpioInterruptFalling :
        gpio_conf->interruptMode = kGPIO_IntFallingEdge;
        break;
    case eGpioInterruptEdge :
        gpio_conf->interruptMode = kGPIO_IntRisingOrFallingEdge;
        break;
    case eGpioInterruptLow :
        gpio_conf->interruptMode = kGPIO_IntLowLevel;
        break;
    case eGpioInterruptHigh :
        gpio_conf->interruptMode = kGPIO_IntHighLevel;
        break;
    default :
        RT106A_LOGD(TAG, "gpio set interrupt not supported");
        break;
    }
}


static void iot_hal_gpio_get_interrupt(IotGpioInterrupt_t *gpio_get_interrupt, gpio_pin_config_t *gpio_conf)
{
    switch (gpio_conf->interruptMode) {
    case kGPIO_NoIntmode :
        *gpio_get_interrupt = eGpioInterruptNone;
        break;
    case kGPIO_IntRisingEdge :
        *gpio_get_interrupt = eGpioInterruptRising;
        break;
    case kGPIO_IntFallingEdge :
        *gpio_get_interrupt = eGpioInterruptFalling;
        break;
    case kGPIO_IntRisingOrFallingEdge :
        *gpio_get_interrupt = eGpioInterruptEdge;
        break;
    case kGPIO_IntLowLevel :
        *gpio_get_interrupt = eGpioInterruptLow;
        break;
    case kGPIO_IntHighLevel :
        *gpio_get_interrupt = eGpioInterruptHigh;
        break;
    default :
        RT106A_LOGD(TAG, "gpio set interrupt not supported");
        break;
    }
}


static void iot_hal_gpio_set_direction(IotGpioDirection_t *gpio_set_direction, gpio_pin_config_t *gpio_conf)
{
    switch (*gpio_set_direction) {
    RT106A_LOGD(TAG, "gpio direction: %d", *gpio_set_direction);
    case eGpioDirectionInput:
        gpio_conf->direction= kGPIO_DigitalInput;
        break;
    case eGpioDirectionOutput:
        gpio_conf->direction = kGPIO_DigitalOutput;
        break;
    default :
        RT106A_LOGD(TAG, "gpio set direction not supported");
        break;
    }
}

static void iot_hal_gpio_get_direction(IotGpioDirection_t *gpio_get_direction, gpio_pin_config_t *gpio_conf)
{
    switch (gpio_conf->direction) {
    case kGPIO_DigitalInput:
        *gpio_get_direction = eGpioDirectionInput;
        break;
    case kGPIO_DigitalOutput:
        *gpio_get_direction = eGpioDirectionOutput;
        break;
    default :
        RT106A_LOGD(TAG, "gpio get direction not supported");
        break;
    }
}

int32_t iot_gpio_ioctl( IotGpioHandle_t const pxGpio,
                        IotGpioIoctlRequest_t xRequest,
                        void *const pvBuffer )
{
    int32_t ret = IOT_GPIO_FUNCTION_NOT_SUPPORTED;
    if (pxGpio == NULL || pvBuffer == NULL) {
        RT106A_LOGD(TAG, "Invalid  arguments %s", __func__);
        return IOT_GPIO_INVALID_VALUE;
    }
    gpio_ctx_t *gpio_ctx = (gpio_ctx_t *) pxGpio;
    int32_t gpio_num = gpio_ctx->gpio_num;
    pin_num_map_2_gpio_t *map = NULL;
    //gpio_config_t io_conf = {0};
    switch (xRequest) {
    case eSetGpioFunction: {
        gpio_ctx->set_function_val = *(uint8_t *)pvBuffer;
        //FIXME what is this ?
        ret = IOT_GPIO_SUCCESS;
        //return IOT_GPIO_FUNCTION_NOT_SUPPORTED;
        break;
    }
    case eSetGpioDirection: {
        IotGpioDirection_t *set_direction = (IotGpioDirection_t *)pvBuffer;
        iot_hal_gpio_set_direction(set_direction, &gpio_ctx->iot_gpio_conf);
        map = getPinMap( gpio_num);
        if (map == NULL) {
            ret = IOT_GPIO_INVALID_VALUE;
        } else {
            GPIO_PinInit(map->base, map->pin_num, &(gpio_ctx->iot_gpio_conf));;
        ret = IOT_GPIO_SUCCESS;
        }
        break;
    }
    case eSetGpioPull: {
        IotGpioPull_t *gpio_set_push_pull = (IotGpioPull_t *)pvBuffer;

        //TODO NEED fsl_gpio.c to provid valid API
        RT106A_LOGD(TAG, "Need fsl_gpio.c to provid valid API to set GPIO push pull mode\r\n");
        #if 0
        iot_hal_gpio_set_pullup_pulldown(gpio_set_push_pull, &gpio_ctx->iot_gpio_conf);
        if (gpio_ctx->iot_gpio_conf.pull_up_en) {
            gpio_pullup_en(gpio_ctx->gpio_num);
        } else {
            gpio_pullup_dis(gpio_ctx->gpio_num);
        }
        if (gpio_ctx->iot_gpio_conf.pull_down_en) {
            gpio_pulldown_en(gpio_ctx->gpio_num);
        } else {
            gpio_pulldown_dis(gpio_ctx->gpio_num);
        }
        #endif
        ret = IOT_GPIO_FUNCTION_NOT_SUPPORTED;
        break;
    }
    case eSetGpioOutputMode: {
        IotGpioOutputMode_t *gpio_set_op_mode = (IotGpioOutputMode_t *)pvBuffer;

        //TODO NEED fsl_gpio.c to provid valid API
        RT106A_LOGD(TAG, "Need fsl_gpio.c to provid valid API to set GPIO output mode\r\n");
        #if 0
        iot_hal_gpio_set_output_mode(gpio_set_op_mode, &gpio_ctx->iot_gpio_conf);
        #endif
        ret = IOT_GPIO_FUNCTION_NOT_SUPPORTED;
        break;
    }
    case eSetGpioInterrupt: {
        IotGpioInterrupt_t *gpio_set_interrupt = (IotGpioInterrupt_t *)pvBuffer;
        iot_hal_gpio_set_interrupt(gpio_set_interrupt, &gpio_ctx->iot_gpio_conf);
        map = getPinMap( gpio_num);
        if (map == NULL) {
            ret = IOT_GPIO_INVALID_VALUE;
        } else {
            GPIO_PinInit(map->base, map->pin_num, &(gpio_ctx->iot_gpio_conf));;
        ret = IOT_GPIO_SUCCESS;
        }
        break;
    }
    case eSetGpioSpeed: {
        //TODO need to check the existing APIS
        RT106A_LOGD(TAG, "Need fsl_gpio.c to provid valid API to set Pin speed\r\n");
        ret = IOT_GPIO_FUNCTION_NOT_SUPPORTED;
        break;
    }
    case eSetGpioDriveStrength: {
        //TODO NEED fsl_gpio.c to provid valid API
        RT106A_LOGD(TAG, "Need fsl_gpio.c to provid valid API to set Pin strength\r\n");
        ret = IOT_GPIO_FUNCTION_NOT_SUPPORTED;
        break;
    }
    case eGetGpioFunction: {
        *(uint32_t *)pvBuffer = gpio_ctx->set_function_val;
        //TODO NEED fsl_gpio.c to provid valid API
        RT106A_LOGD(TAG, "Need fsl_gpio.c to provid valid API to get GPIO features\r\n");
        ret =  IOT_GPIO_FUNCTION_NOT_SUPPORTED;
        break;
    }
    case eGetGpioDirection: {
        IotGpioDirection_t *get_direction = (IotGpioDirection_t *)pvBuffer;
        iot_hal_gpio_get_direction(get_direction, &gpio_ctx->iot_gpio_conf);
        map = getPinMap( gpio_num);
        if (map == NULL) {
            ret = IOT_GPIO_INVALID_VALUE;
        } else {
            GPIO_PinInit(map->base, map->pin_num, &(gpio_ctx->iot_gpio_conf));;
        ret = IOT_GPIO_SUCCESS;
        }
        break;
    }
    case eGetGpioPull: {
        IotGpioPull_t *gpio_get_push_pull = (IotGpioPull_t *)pvBuffer;

        //TODO NEED fsl_gpio.c to provid valid API
        RT106A_LOGD(TAG, "Need fsl_gpio.c to provid valid API to get GPIO Pull feature\r\n");
        #if 0
        iot_hal_gpio_get_pullup_pulldown(gpio_get_push_pull, &gpio_ctx->iot_gpio_conf);
        #endif
        ret =  IOT_GPIO_FUNCTION_NOT_SUPPORTED;
        break;
    }
    case eGetGpioOutputType: {
        IotGpioOutputMode_t *gpio_get_op_mode = (IotGpioOutputMode_t *)pvBuffer;
        //TODO NEED fsl_gpio.c to provid valid API
        RT106A_LOGD(TAG, "Need fsl_gpio.c to provid valid API to get GPIO output type\r\n");
        #if 0
        iot_hal_gpio_get_output_mode(gpio_get_op_mode, &gpio_ctx->iot_gpio_conf);
        #endif
        ret = IOT_GPIO_FUNCTION_NOT_SUPPORTED;
        break;
    }
    case eGetGpioInterrupt: {
        IotGpioInterrupt_t *gpio_get_interrupt = (IotGpioInterrupt_t *)pvBuffer;
        iot_hal_gpio_get_interrupt(gpio_get_interrupt, &gpio_ctx->iot_gpio_conf);
        ret = IOT_GPIO_SUCCESS;
        break;
    }
    case eGetGpioSpeed: {
        //TODO NEED fsl_gpio.c to provid valid API
        RT106A_LOGD(TAG, "Need fsl_gpio.c to provid valid API to get GPIO speed\r\n");
        ret = IOT_GPIO_FUNCTION_NOT_SUPPORTED;
        break;
    }
    case eGetGpioDriveStrength: {

        //TODO NEED fsl_gpio.c to provid valid API
        RT106A_LOGD(TAG, "Need fsl_gpio.c to provid valid API to get GPIO speed\r\n");
        #if 0
        gpio_drive_cap_t *gpio_drive_strength = (gpio_drive_cap_t *)pvBuffer;
        ret = gpio_get_drive_capability(gpio_ctx->gpio_num, gpio_drive_strength);
        if (ret != 0) {
            RT106A_LOGD(TAG, "Failed in getting drive capability %d", ret);
            return IOT_GPIO_READ_FAILED;
        }
        #endif
        ret = IOT_GPIO_FUNCTION_NOT_SUPPORTED;
        break;
    }
    default: {
        ret = IOT_GPIO_INVALID_VALUE;
        break;
    }
    }
    return ret;
}

static void gpio_cb (void *arg)
{
    gpio_ctx_t *gpio_ctx = (gpio_ctx_t *) arg;
    if (gpio_ctx->func) {
        gpio_ctx->func(gpio_ctx->ucPinState, gpio_ctx->arg2);
    }
}

void iot_gpio_set_callback( IotGpioHandle_t const pxGpio,
                            IotGpioCallback_t xGpioCallback,
                            void *pvUserContext )
{
    if (pxGpio == NULL) {
        RT106A_LOGD(TAG, "Invalid  arguments %s", __func__);
        return;
    }
    gpio_ctx_t *gpio_ctx = (gpio_ctx_t *) pxGpio;
//   gpio_uninstall_isr_service();
    gpio_int_status_bitmask = gpio_int_status_bitmask | (1 << gpio_ctx->gpio_num);
    gpio_ctx->func = xGpioCallback;
    gpio_ctx->arg2 = pvUserContext;
//   gpio_isr_handler_add(gpio_ctx->gpio_num, gpio_cb, (void *)gpio_ctx);
//TODO fsl_gpio.c needs to provid interrupt installing function

    RT106A_LOGD(TAG, "%s GPIO interrupt bitmask %d", __func__, gpio_int_status_bitmask);
}

int32_t iot_gpio_read_sync( IotGpioHandle_t const pxGpio,
                            uint8_t *pucPinState )
{
    if (pxGpio == NULL) {
        RT106A_LOGD(TAG, "Invalid  arguments %s", __func__);
        return IOT_GPIO_INVALID_VALUE;
    }
    gpio_ctx_t *gpio_ctx = (gpio_ctx_t *) pxGpio;
//TODO fsl_gpio.c needs to provid relevant API to read Pin state
    #if 0
    *pucPinState = gpio_get_level(gpio_ctx->gpio_num);
    #endif

    return IOT_GPIO_FUNCTION_NOT_SUPPORTED;
}

int32_t iot_gpio_write_sync( IotGpioHandle_t const pxGpio,
                             uint8_t ucPinState )
{
    int32_t ret;
    gpio_ctx_t *gpio_ctx;
    const pin_num_map_2_gpio_t *map;

    if (pxGpio != NULL)
    {
        gpio_ctx = (gpio_ctx_t *) pxGpio;
        map = getPinMap(gpio_ctx->gpio_num);
        if (map != NULL)
        {
            GPIO_PinWrite(map->base, map->pin_num, ucPinState);
            ret = IOT_GPIO_SUCCESS;
        }
        else
        {
            ret = IOT_GPIO_INVALID_VALUE;
        }
    }
    else
    {
        RT106A_LOGD(TAG, "Invalid  arguments %s", __func__);
        ret = IOT_GPIO_INVALID_VALUE;
    }

    return ret;
}

int32_t iot_gpio_close( IotGpioHandle_t const pxGpio )
{

    if (pxGpio == NULL) {
        RT106A_LOGD(TAG, "Invalid  arguments %s", __func__);
        return IOT_GPIO_INVALID_VALUE;
    }
    gpio_ctx_t *gpio_ctx = (gpio_ctx_t *) pxGpio;
    if (gpio_ctx->gpio_num >= 0 && IsValidPinNumber(gpio_ctx->gpio_num) && (0x01 & gpio_status_bitmask >> gpio_ctx->gpio_num)) {
        RT106A_LOGD(TAG, "%s GPIO Number %d, GPIO bitmask %d, GPIO interrupt bitmask %d",
            __func__, gpio_ctx->gpio_num, gpio_status_bitmask, gpio_int_status_bitmask);
        if (0x01 & gpio_int_status_bitmask >> gpio_ctx->gpio_num) {
            //gpio_isr_handler_remove(gpio_ctx->gpio_num);
           // gpio_uninstall_isr_service();
            gpio_int_status_bitmask = gpio_int_status_bitmask & ~(1 << gpio_ctx->gpio_num);
        }
        gpio_status_bitmask = gpio_status_bitmask & ~(1 << gpio_ctx->gpio_num);
        free(pxGpio);
        return IOT_GPIO_SUCCESS;
    } else {
        return IOT_GPIO_INVALID_VALUE;
    }
}
