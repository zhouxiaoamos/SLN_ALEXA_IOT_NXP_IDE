################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/ais_alerts.c \
../source/ais_buttons.c \
../source/ais_continuous_utterance.c \
../source/aisv2_app.c \
../source/app_aws_shadow.c \
../source/app_init.c \
../source/app_smart_home_task.c \
../source/audio_processing_task.c \
../source/aws_iot_demo_shadow.c \
../source/aws_ota_check.c \
../source/device_utils.c \
../source/dhcp_server.c \
../source/fault_handlers.c \
../source/ffs_provision_cli.c \
../source/iot_freertos_init.c \
../source/main.c \
../source/main_demo.c \
../source/mqtt_connection.c \
../source/network_connection.c \
../source/perf.c \
../source/reconnection_task.c \
../source/semihost_hardfault.c \
../source/sln_RT10xx_RGB_LED_driver.c \
../source/sln_RT10xx_RGB_LED_driver_pwm.c \
../source/sln_convert.c \
../source/sln_encrypt.c \
../source/sln_flash.c \
../source/sln_flash_config.c \
../source/sln_flash_mgmt.c \
../source/sln_iot_pkcs11_mbedtls.c \
../source/sln_iot_pkcs11_pal.c \
../source/sln_reset.c \
../source/sln_shell.c \
../source/sln_time_utils.c \
../source/streamer_pcm.c \
../source/switch.c \
../source/tcpip_manager.c \
../source/ux_attention_system.c \
../source/wifi_credentials.c \
../source/wifi_management.c 

OBJS += \
./source/ais_alerts.o \
./source/ais_buttons.o \
./source/ais_continuous_utterance.o \
./source/aisv2_app.o \
./source/app_aws_shadow.o \
./source/app_init.o \
./source/app_smart_home_task.o \
./source/audio_processing_task.o \
./source/aws_iot_demo_shadow.o \
./source/aws_ota_check.o \
./source/device_utils.o \
./source/dhcp_server.o \
./source/fault_handlers.o \
./source/ffs_provision_cli.o \
./source/iot_freertos_init.o \
./source/main.o \
./source/main_demo.o \
./source/mqtt_connection.o \
./source/network_connection.o \
./source/perf.o \
./source/reconnection_task.o \
./source/semihost_hardfault.o \
./source/sln_RT10xx_RGB_LED_driver.o \
./source/sln_RT10xx_RGB_LED_driver_pwm.o \
./source/sln_convert.o \
./source/sln_encrypt.o \
./source/sln_flash.o \
./source/sln_flash_config.o \
./source/sln_flash_mgmt.o \
./source/sln_iot_pkcs11_mbedtls.o \
./source/sln_iot_pkcs11_pal.o \
./source/sln_reset.o \
./source/sln_shell.o \
./source/sln_time_utils.o \
./source/streamer_pcm.o \
./source/switch.o \
./source/tcpip_manager.o \
./source/ux_attention_system.o \
./source/wifi_credentials.o \
./source/wifi_management.o 

C_DEPS += \
./source/ais_alerts.d \
./source/ais_buttons.d \
./source/ais_continuous_utterance.d \
./source/aisv2_app.d \
./source/app_aws_shadow.d \
./source/app_init.d \
./source/app_smart_home_task.d \
./source/audio_processing_task.d \
./source/aws_iot_demo_shadow.d \
./source/aws_ota_check.d \
./source/device_utils.d \
./source/dhcp_server.d \
./source/fault_handlers.d \
./source/ffs_provision_cli.d \
./source/iot_freertos_init.d \
./source/main.d \
./source/main_demo.d \
./source/mqtt_connection.d \
./source/network_connection.d \
./source/perf.d \
./source/reconnection_task.d \
./source/semihost_hardfault.d \
./source/sln_RT10xx_RGB_LED_driver.d \
./source/sln_RT10xx_RGB_LED_driver_pwm.d \
./source/sln_convert.d \
./source/sln_encrypt.d \
./source/sln_flash.d \
./source/sln_flash_config.d \
./source/sln_flash_mgmt.d \
./source/sln_iot_pkcs11_mbedtls.d \
./source/sln_iot_pkcs11_pal.d \
./source/sln_reset.d \
./source/sln_shell.d \
./source/sln_time_utils.d \
./source/streamer_pcm.d \
./source/switch.d \
./source/tcpip_manager.d \
./source/ux_attention_system.d \
./source/wifi_credentials.d \
./source/wifi_management.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DCPU_MIMXRT106ADVL6A -DCPU_MIMXRT106ADVL6A_cm7 -DSDK_DEBUGCONSOLE=1 -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 -DPRINTF_ADVANCED_ENABLE=0 -DSCANF_ADVANCED_ENABLE=0 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DXIP_BOOT_HEADER_DCD_ENABLE=1 -DAPP_MAJ_VER=0x02 -DAPP_MIN_VER=0x00 -DAPP_BLD_VER=0x0000 -DDEBUG_CONSOLE_ENABLE_ECHO -DUSB_CDC_SERIAL_MANAGER_RUN_NO_HOST=1 -DRELOCATE_VECTOR_TABLE=1 -DHAL_UART_TRANSFER_MODE=1 -DTFA9XXX_DEV_NUM=1 -DUSE_TFA=1 -DUSE_TFA9894_PUI=1 -DWICED_BLUETOOTH_PLATFORM -DACE_HAL_DEVICE_INFO_PRODUCT_NAME='"SLN_ALEXA_IOT"' -DACE_HAL_DEVICE_INFO_HARDWARE_NAME='"MIMXRT106A"' -DERASE_PARTIAL_SECTORS=1 -D__SEMIHOST_HARDFAULT_DISABLE=1 -DSERIAL_PORT_TYPE_USBCDC=1 -DSERIAL_MANAGER_NON_BLOCKING_MODE=1 -DUART1_BLE_SERIAL_MANANGER_BLOCKING_MODE=1 -DUSB_STACK_FREERTOS_HEAP_SIZE=65536 -DUSB_STACK_FREERTOS -DBOARD_USE_VIRTUALCOM=1 -DDEBUG_CONSOLE_IO_USBCDC=1 -DDEBUG_CONSOLE_TRANSFER_NON_BLOCKING -DARM_MATH_CM7 -DCR_INTEGER_PRINTF -DSDK_I2C_BASED_COMPONENT_USED=1 -DSDK_SAI_BASED_COMPONENT_USED=1 -DUSE_RTOS=1 -DFSL_RTOS_FREE_RTOS -DLWIP_DNS=1 -DLWIP_DHCP=1 -DLWIP_TIMEVAL_PRIVATE=0 -DRTOS -DAMAZON_MBEDTLS_LWIP_PATCHES -DCONFIG_FLEXRAM_AT_STARTUP=1 -DSNTP_SERVER_DNS=1 -DSDK_DEBUGCONSOLE_UART -DACE_HAL_DEVICE_INFO_MANUFACTURER_NAME='"NXP"' -DACE_HAL_DEVICE_INFO_OS_VERSION='"10.3.0"' -DACE_WIFI_ENABLE_MW_AUTO_CONNECT_NXP -DMBEDTLS_CONFIG_FILE='"aws_mbedtls_config.h"' -DAWS_LOGGING_INCLUDE='"iot_logging_task.h"' -DAWS_SYS_INIT_INCLUDE='"iot_system_init.h"' -DAWS_MQTT_AGENT_INCLUDE='"iot_mqtt_agent.h"' -DAWS_VERSION_INCLUDE='"iot_appversion32.h"' -DDEMO_NETWORK_TYPE=AWSIOT_NETWORK_TYPE_WIFI -Dmalloc=pvPortMalloc -Dfree=vPortFree -Dcalloc=pvPortCalloc -Drealloc=pvPortRealloc -DFFS_ENABLED -DUSB_DEVICE_CONFIG_CDC_ACM=1 -DDEBUG_CONSOLE_RX_ENABLE=0 -DACE_COMPONENT_libace_connectivity_manager -DACE_COMPONENT_hal_factoryreset -DACE_COMPONENT_hal_device_info -DACE_COMPONENT_hal_kv_storage -DACE_COMPONENT_hal_dha -DSLN_AFE_LIB -DSLN_DSP_TOOLBOX_LIB -DSDIO_ENABLED -DSERIAL_PORT_TYPE_UART=1 -DBOARD_USE_CODEC=1 -DCODEC_TFA9XXX_ENABLE -DWIFI_WICED -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\board" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\source" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\component\serial_manager" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\drivers" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\component\lists" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\component\serial_manager\usb_cdc_adapter" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\codec" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\port\arch" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\compat\posix\arpa" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\compat\posix\net" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\compat\posix" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\compat\posix\sys" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\compat\stdc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\lwip" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\lwip\priv" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\lwip\prot" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\netif" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\netif\ppp" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\netif\ppp\polarssl" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\utilities" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\platforms\MURATA_TYPE1DX" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\freertos_kernel\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\freertos_kernel\portable\GCC\ARM_CM4F" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\usb\phy" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\usb\device\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\usb\device\source" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\component\osa" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\usb\device\source\ehci" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\component\i2c" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\drivers\freertos" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\device_info\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\dha\src\crypto_utils" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\dha\src\port" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\factory_reset\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\log" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\wifi\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\afw\common\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\afw\common\src" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\asd\asd_system_tools" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\asd\asd_utils" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\asd_logger" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\debug" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\freertos_kernel\portable\MemMang" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\mbedtls" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\common\utilities" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\include\internal" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include\ace" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include\arpa" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include\cJSON" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include\libwebsockets\abstract" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include\libwebsockets\abstract\protocols" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include\libwebsockets\abstract\transports" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include\libwebsockets" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include\nanopb" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include\sys" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include\toolchain" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\dpk_impl\rt106a\hal_impl" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\aws_ais\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio_streamer\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio_streamer\streamer\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\mbedtls\port\ksdk" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\amazon" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\aws\shadow\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\aws\shadow\include\types" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\aws\shadow\src" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\aws\shadow\src\private" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\standard\common\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\standard\common\include\private" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\standard\common\include\types" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\standard\mqtt\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\standard\mqtt\include\types" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\standard\mqtt\src\private" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\standard\serializer\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\demos\dev_mode_key_provisioning\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\3rdparty\pkcs11" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\abstractions\wifi\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\abstractions\secure_sockets\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\abstractions\platform\freertos\include\platform" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\abstractions\platform\include\platform" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\abstractions\platform\include\types" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\freertos_plus\standard\crypto\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\freertos_plus\standard\pkcs11\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\freertos_plus\standard\tls\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\freertos_plus\standard\utils\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\demos\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\demos\network_manager" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\voice" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\sdmmc\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\sdmmc\host" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\avs\smart_home" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\device" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\component\uart" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\xip" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\CMSIS" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\usb\device\class\cdc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\usb\device\class" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\codec\tfa9xxx" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\codec\tfa9xxx\vas_tfa_drv" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_BLE\bt_app_inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_BLE\wiced_bt\BTE\Components\stack\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_BLE\wiced_bt\BTE\Projects\bte\main" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_BLE\wiced_bt\BTE\WICED" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_BLE\wiced_bt\imxrt_port" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\WWD\internal\chips\4343W" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\RTOS\FreeRTOS\WWD" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\WWD\include\RTOS" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\WWD\include\network" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\WWD\include\platform" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\WWD\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\WWD\internal\bus_protocols\SDIO" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\WWD\internal\bus_protocols" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\WWD\internal" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\WWD" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\network\LwIP\WWD" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\platform\MCU\LPC" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\platform\MCU" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\platform\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\WICED\platform" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\wiced\43xxx_Wi-Fi\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\usb\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\py_crc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\sdmmc\osa" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\de_DE" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\en_AU" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\en_CA" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\en_GB" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\en_IN" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\en_US" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\es_ES" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\es_MX" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\es_US" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\fr_CA" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\fr_FR" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\hi_IN" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\it_IT" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\ja_JP" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\audio\avs_sound_library\pt_BR" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\config_files" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\apps\common\startup\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\apps\common\test_runner\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\apps\rt106a\common\freertos\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\apps\rt106a\test_runner\inc\config_files" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include\lwip\apps" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\source\ble" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\port" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\lwip\src\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\amazon_acs\ace\sdk\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\mbedtls\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\c_sdk\standard\mqtt\src" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\abstractions\platform\freertos\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_ais_ffs_demo\freertos\libraries\abstractions\platform\include" -O2 -fno-common -g -Wall -mno-unaligned-access  -fomit-frame-pointer  -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


