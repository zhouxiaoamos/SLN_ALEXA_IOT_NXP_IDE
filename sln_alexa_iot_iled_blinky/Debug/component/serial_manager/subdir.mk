################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/serial_manager/serial_manager.c \
../component/serial_manager/serial_port_uart.c 

OBJS += \
./component/serial_manager/serial_manager.o \
./component/serial_manager/serial_port_uart.o 

C_DEPS += \
./component/serial_manager/serial_manager.d \
./component/serial_manager/serial_port_uart.d 


# Each subdirectory must supply rules for building sources it contributes
component/serial_manager/%.o: ../component/serial_manager/%.c component/serial_manager/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT106ADVL6A -DCPU_MIMXRT106ADVL6A_cm7 -DSDK_DEBUGCONSOLE=1 -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 -DPRINTF_ADVANCED_ENABLE=0 -DSCANF_ADVANCED_ENABLE=0 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DXIP_BOOT_HEADER_DCD_ENABLE=1 -DCONFIG_FLEXRAM_AT_STARTUP=1 -DSERIAL_PORT_TYPE_UART=1 -DCR_INTEGER_PRINTF -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky\board" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky\source" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky\drivers" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky\device" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky\utilities" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky\component\uart" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky\component\serial_manager" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky\component\lists" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky\xip" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_iled_blinky\CMSIS" -O0 -fno-common -g3 -Wall -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


