/*
 * Copyright 2018-2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "switch.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

typedef enum _switch_id
{
    SWITCH_1 = 0,
    SWITCH_2,
    SWITCH_COUNT,
} switch_id_t;

typedef enum _switch_state
{
    SWITCH_PRESSED  = 0,
    SWITCH_RELEASED = 1,
} switch_state_t;

static switch_state_t last_state[SWITCH_COUNT] = {0};
static OsaTimeval last_debounce[SWITCH_COUNT]  = {0};

EventGroupHandle_t g_buttonPressed = NULL;

/*!
 * @brief Notify the button task about a Press/Release event.
 */
static void notifyButtonTask(switch_id_t sw_id, switch_state_t state)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (g_buttonPressed != NULL)
    {
        if (sw_id == SWITCH_1)
        {
            if (state == SWITCH_PRESSED)
            {
                xEventGroupSetBitsFromISR(g_buttonPressed, BIT_PRESS_1, &xHigherPriorityTaskWoken);
            }
            else if (state == SWITCH_RELEASED)
            {
                xEventGroupSetBitsFromISR(g_buttonPressed, BIT_RELEASE_1, &xHigherPriorityTaskWoken);
            }
        }
        else if (sw_id == SWITCH_2)
        {
            if (state == SWITCH_PRESSED)
            {
                xEventGroupSetBitsFromISR(g_buttonPressed, BIT_PRESS_2, &xHigherPriorityTaskWoken);
            }
            else if (state == SWITCH_RELEASED)
            {
                xEventGroupSetBitsFromISR(g_buttonPressed, BIT_RELEASE_2, &xHigherPriorityTaskWoken);
            }
        }
    }
}

/*!
 * @brief Handles a switch (SW1/SW2) press/release interrupt.
 * Since partial switch debounce is done in hardware (make sure hysteresis is enabled),
 * the following interrupts are triggered:
 * on button press:   0-5 interrupts SWITCH_RELEASED (these should be ignored)
 *                    followed by 1 interrupt SWITCH_PRESSED (this should be passed to button task)
 * on button release: 1 interrupt SWITCH_RELEASED (this should be passed to button task)
 */
static void processSwitchIrq(GPIO_Type *base, uint32_t pin, switch_id_t sw_id, OsaTimeval curr_time)
{
    uint32_t int_pin          = 0;
    switch_state_t curr_state = 0;

    int_pin = GPIO_PortGetInterruptFlags(base);
    if (((1U << pin) & int_pin) != 0)
    {
        GPIO_PortClearInterruptFlags(base, 1U << pin);

        curr_state = GPIO_PinRead(base, pin);
        if (curr_state != last_state[sw_id])
        {
            if (curr_state == SWITCH_PRESSED)
            {
                notifyButtonTask(sw_id, curr_state);
                last_state[sw_id] = curr_state;
            }
            else if (curr_state == SWITCH_RELEASED)
            {
                if (curr_time.tv_usec - last_debounce[sw_id].tv_usec > DEBOUNCE_TIME_US)
                {
                    notifyButtonTask(sw_id, curr_state);
                    last_state[sw_id] = curr_state;
                }
            }
        }

        last_debounce[sw_id] = curr_time;
    }
}

/*!
 * @brief Interrupt service function for switches: SW1 and SW2.
 * Get current time, perform a software debounce and if needed notify the button task.
 */
void GPIO_IRQHandler(void)
{
    OsaTimeval curr_time = {0};

    osa_time_get(&curr_time);

    processSwitchIrq(SW1_GPIO, SW1_GPIO_PIN, SWITCH_1, curr_time);
    processSwitchIrq(SW2_GPIO, SW2_GPIO_PIN, SWITCH_2, curr_time);

    SDK_ISR_EXIT_BARRIER;
}

void switchInit(void)
{
    /* Define the init structure for the input switch pin */
    gpio_pin_config_t sw_config = {
        kGPIO_DigitalInput,
        0,
        kGPIO_IntRisingOrFallingEdge,
    };

    if (g_buttonPressed == NULL)
    {
        g_buttonPressed = xEventGroupCreate();
        if (g_buttonPressed == NULL)
        {
            configPRINTF(("[ERROR] Could not allocate memory for g_buttonPressed event\r\n"));
        }
    }

    last_state[SWITCH_1] = SWITCH_RELEASED;
    last_state[SWITCH_2] = SWITCH_RELEASED;

    /* Init input switch GPIO. */
    NVIC_SetPriority(BOARD_USER_BUTTON_IRQ, configMAX_SYSCALL_INTERRUPT_PRIORITY - 1);
    EnableIRQ(BOARD_USER_BUTTON_IRQ);

    GPIO_PinInit(SW1_GPIO, SW1_GPIO_PIN, &sw_config);
    GPIO_PortClearInterruptFlags(SW1_GPIO, 1U << SW1_GPIO_PIN);

    GPIO_PinInit(SW2_GPIO, SW2_GPIO_PIN, &sw_config);
    GPIO_PortClearInterruptFlags(SW2_GPIO, 1U << SW2_GPIO_PIN);

    /* Enable GPIO pin interrupt */
    GPIO_PortEnableInterrupts(SW1_GPIO, 1U << SW1_GPIO_PIN);
    GPIO_PortEnableInterrupts(SW2_GPIO, 1U << SW2_GPIO_PIN);
}
