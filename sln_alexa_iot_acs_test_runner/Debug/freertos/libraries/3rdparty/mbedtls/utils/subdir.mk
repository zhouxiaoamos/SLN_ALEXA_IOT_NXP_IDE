################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../freertos/libraries/3rdparty/mbedtls/utils/mbedtls_utils.c 

OBJS += \
./freertos/libraries/3rdparty/mbedtls/utils/mbedtls_utils.o 

C_DEPS += \
./freertos/libraries/3rdparty/mbedtls/utils/mbedtls_utils.d 


# Each subdirectory must supply rules for building sources it contributes
freertos/libraries/3rdparty/mbedtls/utils/%.o: ../freertos/libraries/3rdparty/mbedtls/utils/%.c freertos/libraries/3rdparty/mbedtls/utils/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DCPU_MIMXRT106ADVL6A -DCPU_MIMXRT106ADVL6A_cm7 -DSDK_DEBUGCONSOLE=1 -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 -DPRINTF_ADVANCED_ENABLE=0 -DSCANF_ADVANCED_ENABLE=0 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DXIP_BOOT_HEADER_DCD_ENABLE=1 -DAPP_MAJ_VER=0x02 -DAPP_MIN_VER=0x00 -DAPP_BLD_VER=0x0000 -DDEBUG_CONSOLE_ENABLE_ECHO -DUSB_CDC_SERIAL_MANAGER_RUN_NO_HOST=0 -DRELOCATE_VECTOR_TABLE=1 -DHAL_UART_TRANSFER_MODE=1 -DACE_CONFIG_OVERRIDES='"rt106a_ace_config.h"' -DACE_HAL_DEVICE_INFO_PRODUCT_NAME='"SLN_ALEXA_IOT"' -DACE_HAL_DEVICE_INFO_HARDWARE_NAME='"MIMXRT106A"' -DWICED_BLUETOOTH_PLATFORM -DERASE_PARTIAL_SECTORS=1 -D__SEMIHOST_HARDFAULT_DISABLE=1 -DSERIAL_PORT_TYPE_USBCDC=1 -DSERIAL_MANAGER_NON_BLOCKING_MODE=1 -DUART1_BLE_SERIAL_MANANGER_BLOCKING_MODE=1 -DUSB_STACK_FREERTOS_HEAP_SIZE=65536 -DUSB_STACK_FREERTOS -DBOARD_USE_VIRTUALCOM=1 -DDEBUG_CONSOLE_IO_USBCDC=1 -DDEBUG_CONSOLE_TRANSFER_NON_BLOCKING -DARM_MATH_CM7 -DCR_INTEGER_PRINTF -DSDK_I2C_BASED_COMPONENT_USED=1 -DUSE_RTOS=1 -DFSL_RTOS_FREE_RTOS -DLWIP_DNS=1 -DLWIP_DHCP=1 -DLWIP_TIMEVAL_PRIVATE=0 -DRTOS -DAMAZON_MBEDTLS_LWIP_PATCHES -DAMAZON_TESTS_ENABLE=1 -DCONFIG_FLEXRAM_AT_STARTUP=1 -DSDK_DEBUGCONSOLE_UART -DACE_HAL_DEVICE_INFO_MANUFACTURER_NAME='"NXP"' -DACE_HAL_DEVICE_INFO_OS_VERSION='"10.3.0"' -DACE_WIFI_ENABLE_MW_AUTO_CONNECT_NXP -DACE_WIFI_HAL_AUTO_CONNECT -DMBEDTLS_CONFIG_FILE='"aws_mbedtls_config.h"' -DAWS_LOGGING_INCLUDE='"iot_logging_task.h"' -DAWS_SYS_INIT_INCLUDE='"iot_system_init.h"' -DAWS_MQTT_AGENT_INCLUDE='"iot_mqtt_agent.h"' -DAWS_VERSION_INCLUDE='"iot_appversion32.h"' -DDEMO_NETWORK_TYPE=AWSIOT_NETWORK_TYPE_WIFI -Dmalloc=pvPortMalloc -Dfree=vPortFree -Dcalloc=pvPortCalloc -Drealloc=pvPortRealloc -DUSB_DEVICE_CONFIG_CDC_ACM=1 -DSDIO_ENABLED -DACE_COMPONENT_libace_connectivity_manager -DACE_COMPONENT_hal_factoryreset -DACE_COMPONENT_hal_device_info -DACE_COMPONENT_hal_kv_storage -DACE_COMPONENT_hal_dha -DSERIAL_PORT_TYPE_UART=1 -DWIFI_WICED -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\board" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\source" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\component\serial_manager" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\drivers" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\component\lists" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\component\serial_manager\usb_cdc_adapter" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\port\arch" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\compat\posix\arpa" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\compat\posix\net" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\compat\posix" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\compat\posix\sys" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\compat\stdc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\lwip" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\lwip\priv" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\lwip\prot" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\netif" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\netif\ppp" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\netif\ppp\polarssl" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\utilities" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\platforms\MURATA_TYPE1DX" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\usb\phy" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\usb\device\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\usb\device\source" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\component\osa" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\usb\device\source\ehci" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\codec" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\component\i2c" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\drivers\freertos" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\mbedtls\port\ksdk" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\freertos_kernel\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\freertos_kernel\portable\GCC\ARM_CM4F" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\aws\shadow\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\aws\shadow\include\types" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\aws\shadow\src" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\aws\shadow\src\private" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\standard\common\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\standard\common\include\private" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\standard\common\include\types" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\standard\mqtt\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\standard\mqtt\include\types" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\standard\mqtt\src\private" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\standard\serializer\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\demos\dev_mode_key_provisioning\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\freertos_plus\standard\utils\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\freertos_plus\standard\crypto\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\abstractions\secure_sockets\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\abstractions\platform\freertos\include\platform" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\abstractions\platform\include\platform" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\abstractions\platform\include\types" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\freertos_plus\standard\tls\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\freertos_plus\standard\pkcs11\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\3rdparty\pkcs11" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\demos\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\demos\network_manager" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\abstractions\wifi\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\sdmmc\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\sdmmc\host" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\device_info\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\dha\src\crypto_utils" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\dha\src\port" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\factory_reset\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\log" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\wifi\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\afw\common\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\afw\common\src" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\asd\asd_system_tools" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\asd\asd_utils" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\asd_logger" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\debug" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\freertos_kernel\portable\MemMang" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\mbedtls" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\utilities" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\include\internal" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include\ace" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include\arpa" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include\cJSON" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include\libwebsockets\abstract" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include\libwebsockets\abstract\protocols" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include\libwebsockets\abstract\transports" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include\libwebsockets" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include\nanopb" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include\sys" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include\toolchain" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\hal_impl" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\external\Unity\extras\fixture\src" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\afw\ace_hal\cli\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\common\asd\asd_cli" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\dpk_impl\rt106a\test\iot_tests" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\abstractions\common_io\test" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\tests\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\device" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\component\uart" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\xip" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\CMSIS" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\usb\device\class\cdc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\usb\device\class" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_BLE\bt_app_inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_BLE\wiced_bt\BTE\Components\stack\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_BLE\wiced_bt\BTE\Projects\bte\main" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_BLE\wiced_bt\BTE\WICED" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_BLE\wiced_bt\imxrt_port" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\WWD\internal\chips\4343W" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\RTOS\FreeRTOS\WWD" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\WWD\include\RTOS" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\WWD\include\network" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\WWD\include\platform" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\WWD\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\WWD\internal\bus_protocols\SDIO" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\WWD\internal\bus_protocols" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\WWD\internal" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\WWD" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\network\LwIP\WWD" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\platform\MCU\LPC" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\platform\MCU" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\platform\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\WICED\platform" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\wiced\43xxx_Wi-Fi\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\usb\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\sdmmc\osa" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\py_crc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\config_files" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\apps\common\startup\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\apps\common\test_runner\inc" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\apps\rt106a\common\common_init\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\apps\rt106a\common\freertos\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\apps\rt106a\test_runner\inc\config_files" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\apps\rt106a\test_runner\inc\test_config" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include\lwip\apps" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\source\ble" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\port" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\lwip\src\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\mbedtls\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\c_sdk\standard\mqtt\src" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\abstractions\platform\freertos\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\freertos\libraries\abstractions\platform\include" -I"C:\Users\amos.zhou\Documents\MCUXpressoIDE_11.5.0_7232\workspace\sln_alexa_iot_acs_test_runner\amazon_acs\ace\sdk\include" -O0 -fno-common -g -Wall -mno-unaligned-access  -fomit-frame-pointer  -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


