/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef RT106A_ACE_CONFIG_H_
#define RT106A_ACE_CONFIG_H_

/***********************************************************************/
/*** below defines will overwrite defines from ace_cli_cmds_config.h ***/
/***********************************************************************/

/* - Get default integration test config */
#ifndef CLI_ace_mw_wifi_cli_get_test_config
#define CLI_ace_mw_wifi_cli_get_test_config 1
#endif

/* - Set integration test config */
#ifndef CLI_ace_mw_wifi_cli_set_test_config
#define CLI_ace_mw_wifi_cli_set_test_config 1
#endif

/* - run integration test */
#ifndef CLI_ace_mw_wifi_cli_test_integration
#define CLI_ace_mw_wifi_cli_test_integration 1
#endif

/* Start wifi hal integration test */
#ifndef CLI_ace_hal_wifi_test
#define CLI_ace_hal_wifi_test 1
#endif

/* ssid=<ssid> [psk=<psk> | wep0=<wep0> [wep1=<wep1> wep2=<wep2> wep3=<wep3>] wepIdx=<wepIdx>] [hidden=<0|1>] */
#ifndef CLI_ace_hal_wifi_set_test_config
#define CLI_ace_hal_wifi_set_test_config 1
#endif

/* Get current hal wifi test config */
#ifndef CLI_ace_hal_wifi_get_test_config
#define CLI_ace_hal_wifi_get_test_config 1
#endif

/* Common Test Cases */
#ifndef CLI_ace_core_events_test_common
#define CLI_ace_core_events_test_common 1
#endif

/* RTOS Test Cases */
#ifndef CLI_ace_core_events_test_rtos
#define CLI_ace_core_events_test_rtos 1
#endif

/* KVS Hal Cli */
#ifndef CLI_ace_hal_kvs_cli
#define CLI_ace_hal_kvs_cli 1
#endif

/* Test ACE OSAL IpMutex */
#ifndef CLI_ace_osal_ipmutex_test
#define CLI_ace_osal_ipmutex_test 1
#endif

/* Test ACE OSAL Time */
#ifndef CLI_ace_osal_time_test
#define CLI_ace_osal_time_test 1
#endif

/* Test ACE OSAL Thread */
#ifndef CLI_ace_osal_thread_test
#define CLI_ace_osal_thread_test 1
#endif

/* Test ACE OSAL Shared Memory */
#ifndef CLI_ace_osal_shmem_test
#define CLI_ace_osal_shmem_test 1
#endif

/* ACE OSAL Shared Memory Feature CLIs */
#ifndef CLI_ace_osal_shmem_cli
#define CLI_ace_osal_shmem_cli 1
#endif

/* Test ACE OSAL Semaphore */
#ifndef CLI_ace_osal_semaphore_test
#define CLI_ace_osal_semaphore_test 1
#endif

/* Test ACE OSAL Rand */
#ifndef CLI_ace_osal_rand_test
#define CLI_ace_osal_rand_test 1
#endif

/* Get random data */
#ifndef CLI_ace_osal_rand_get
#define CLI_ace_osal_rand_get 1
#endif

/* Test ACE OSAL Mutex */
#ifndef CLI_ace_osal_mutex_test
#define CLI_ace_osal_mutex_test 1
#endif

/* Test ACE OSAL Memory pool */
#ifndef CLI_ace_osal_mp_test
#define CLI_ace_osal_mp_test 1
#endif

/* Test ACE OSAL Message Queue */
#ifndef CLI_ace_osal_queue_test
#define CLI_ace_osal_queue_test 1
#endif

/* Test ACE OSAL Alloc */
#ifndef CLI_ace_osal_alloc_test
#define CLI_ace_osal_alloc_test 1
#endif

/* Metric Hal test */
#ifndef CLI_ace_hal_metric_test
#define CLI_ace_hal_metric_test 1
#endif

/* ATT */
#ifndef CLI_ace_att
#define CLI_ace_att 1
#endif

/* Perf instrumentation tests */
#ifndef CLI_ace_tools_instr_tests
#define CLI_ace_tools_instr_tests 1
#endif

/* test http get */
#ifndef CLI_ace_mw_ace_conn_test_simple_get
#define CLI_ace_mw_ace_conn_test_simple_get 1
#endif

/* test large file get using HTTP get */
#ifndef CLI_ace_mw_ace_conn_test_large_get
#define CLI_ace_mw_ace_conn_test_large_get 1
#endif

/* test http get return code */
#ifndef CLI_ace_mw_ace_conn_test_simple_get_rc
#define CLI_ace_mw_ace_conn_test_simple_get_rc 1
#endif

/* test http get cancel */
#ifndef CLI_ace_mw_ace_conn_test_simple_get_cancel
#define CLI_ace_mw_ace_conn_test_simple_get_cancel 1
#endif

/* test http get retry */
#ifndef CLI_ace_mw_ace_conn_test_simple_get_retry
#define CLI_ace_mw_ace_conn_test_simple_get_retry 1
#endif

/* test http get timeout */
#ifndef CLI_ace_mw_ace_conn_test_simple_get_timeout
#define CLI_ace_mw_ace_conn_test_simple_get_timeout 1
#endif

/* test http get large header */
#ifndef CLI_ace_mw_ace_conn_test_simple_get_large_header
#define CLI_ace_mw_ace_conn_test_simple_get_large_header 1
#endif

/* test http post */
#ifndef CLI_ace_mw_ace_conn_test_simple_post
#define CLI_ace_mw_ace_conn_test_simple_post 1
#endif

/* test https get */
#ifndef CLI_ace_mw_ace_conn_test_tls_get
#define CLI_ace_mw_ace_conn_test_tls_get 1
#endif

/* test multiple https get */
#ifndef CLI_ace_mw_ace_conn_test_multi_tls_get
#define CLI_ace_mw_ace_conn_test_multi_tls_get 1
#endif

/* sample http post */
#ifndef CLI_ace_mw_ace_conn_sample_http_post
#define CLI_ace_mw_ace_conn_sample_http_post 1
#endif

/* test platform time() api */
#ifndef CLI_ace_mw_ace_conn_test_time
#define CLI_ace_mw_ace_conn_test_time 1
#endif

/* test LWS http get without ACM */
#ifndef CLI_ace_mw_ace_conn_test_lws
#define CLI_ace_mw_ace_conn_test_lws 1
#endif

/* ACM integration tests */
#ifndef CLI_ace_mw_ace_conn_test_integration
#define CLI_ace_mw_ace_conn_test_integration 1
#endif

/* execute http/s URL */
#ifndef CLI_ace_mw_ace_conn_test_url
#define CLI_ace_mw_ace_conn_test_url 1
#endif

/* execute debug command assert/config_dump/nullptr */
#ifndef CLI_ace_mw_ace_conn_test_debug_cmd
#define CLI_ace_mw_ace_conn_test_debug_cmd 1
#endif


/**************************************************************/
/*** below defines will overwrite defines from att_common.h ***/
/**************************************************************/

#ifndef ATT_THREAD_STACK_SIZE
#define ATT_THREAD_STACK_SIZE 8192
#endif


/******************************************************************/
/*** below defines will overwrite defines from att_api_config.h ***/
/******************************************************************/

#ifndef ATT_wifi_mgr_test_get_default_config
#define ATT_wifi_mgr_test_get_default_config 1
#endif

#ifndef ATT_wifi_test_setup
#define ATT_wifi_test_setup 1
#endif

#ifndef ATT_wifi_test_teardown
#define ATT_wifi_test_teardown 1
#endif

#ifndef ATT_halWifi_setTestConfig
#define ATT_halWifi_setTestConfig 1
#endif

#ifndef ATT_test_halWifi_initAndStart
#define ATT_test_halWifi_initAndStart 1
#endif

#ifndef ATT_test_halWifi_getMacAddress
#define ATT_test_halWifi_getMacAddress 1
#endif

#ifndef ATT_test_halWifi_disableAllNetworks
#define ATT_test_halWifi_disableAllNetworks 1
#endif

#ifndef ATT_test_halWifi_getConnectionInfo_invalid
#define ATT_test_halWifi_getConnectionInfo_invalid 1
#endif

#ifndef ATT_test_halWifi_getScanRes
#define ATT_test_halWifi_getScanRes 1
#endif

#ifndef ATT_test_halWifi_getDetailedScanRes
#define ATT_test_halWifi_getDetailedScanRes 1
#endif

#ifndef ATT_test_halWifi_connect_invalid
#define ATT_test_halWifi_connect_invalid 1
#endif

#ifndef ATT_test_halWifi_addAndRmNetwork
#define ATT_test_halWifi_addAndRmNetwork 1
#endif

#ifndef ATT_test_halWifi_connect
#define ATT_test_halWifi_connect 1
#endif

#ifndef ATT_acehal_halWifi_tests_setUp
#define ATT_acehal_halWifi_tests_setUp 1
#endif

#ifndef ATT_acehal_halWifi_tests_tearDown
#define ATT_acehal_halWifi_tests_tearDown 1
#endif

#ifndef ATT_acehal_wifi_tests_main
#define ATT_acehal_wifi_tests_main 1
#endif

#ifndef ATT_aceWifiMgr_init
#define ATT_aceWifiMgr_init 1
#endif

#ifndef ATT_aceWifiMgr_deinit
#define ATT_aceWifiMgr_deinit 1
#endif

#ifndef ATT_aceWifiMgr_isWifiReady
#define ATT_aceWifiMgr_isWifiReady 1
#endif

#ifndef ATT_aceWifiMgr_startScan
#define ATT_aceWifiMgr_startScan 1
#endif

#ifndef ATT_aceWifiMgr_getScanResults
#define ATT_aceWifiMgr_getScanResults 1
#endif

#ifndef ATT_aceWifiMgr_getDetailedScanResults
#define ATT_aceWifiMgr_getDetailedScanResults 1
#endif

#ifndef ATT_aceWifiMgr_getConfiguredNetworks
#define ATT_aceWifiMgr_getConfiguredNetworks 1
#endif

#ifndef ATT_aceWifiMgr_addNetwork
#define ATT_aceWifiMgr_addNetwork 1
#endif

#ifndef ATT_aceWifiMgr_removeNetwork
#define ATT_aceWifiMgr_removeNetwork 1
#endif

#ifndef ATT_aceWifiMgr_saveConfig
#define ATT_aceWifiMgr_saveConfig 1
#endif

#ifndef ATT_aceWifiMgr_enableAllNetworks
#define ATT_aceWifiMgr_enableAllNetworks 1
#endif

#ifndef ATT_aceWifiMgr_disableAllNetworks
#define ATT_aceWifiMgr_disableAllNetworks 1
#endif

#ifndef ATT_aceWifiMgr_connect
#define ATT_aceWifiMgr_connect 1
#endif

#ifndef ATT_aceWifiMgr_disconnect
#define ATT_aceWifiMgr_disconnect 1
#endif

#ifndef ATT_aceWifiMgr_createSoftAP
#define ATT_aceWifiMgr_createSoftAP 1
#endif

#ifndef ATT_aceWifiMgr_removeSoftAP
#define ATT_aceWifiMgr_removeSoftAP 1
#endif

#ifndef ATT_aceWifiMgr_getNetworkState
#define ATT_aceWifiMgr_getNetworkState 1
#endif

#ifndef ATT_aceWifiMgr_getConnectionInfo
#define ATT_aceWifiMgr_getConnectionInfo 1
#endif

#ifndef ATT_aceWifiMgr_getIpInfo
#define ATT_aceWifiMgr_getIpInfo 1
#endif

#ifndef ATT_aceWifiMgr_getStatisticInfo
#define ATT_aceWifiMgr_getStatisticInfo 1
#endif

#ifndef ATT_aceWifiMgr_getCapabilityInfo
#define ATT_aceWifiMgr_getCapabilityInfo 1
#endif

#ifndef ATT_aceWifiMgr_enableTcpTunnel
#define ATT_aceWifiMgr_enableTcpTunnel 1
#endif

#ifndef ATT_aceWifiMgr_disableTcpTunnel
#define ATT_aceWifiMgr_disableTcpTunnel 1
#endif

#ifndef ATT_aceWifiMgr_evaluateCaptive
#define ATT_aceWifiMgr_evaluateCaptive 1
#endif

#ifndef ATT_aceWifiMgr_enableNAT
#define ATT_aceWifiMgr_enableNAT 1
#endif

#ifndef ATT_aceWifiMgr_disableNAT
#define ATT_aceWifiMgr_disableNAT 1
#endif

#ifndef ATT_aceWifiMgr_getMacAddress
#define ATT_aceWifiMgr_getMacAddress 1
#endif

#ifndef ATT_aceWifiMgr_setCountryCode
#define ATT_aceWifiMgr_setCountryCode 1
#endif

#ifndef ATT_aceWifiMgr_getCountryCode
#define ATT_aceWifiMgr_getCountryCode 1
#endif

#ifndef ATT_aceWifiMgr_getLogLevel
#define ATT_aceWifiMgr_getLogLevel 1
#endif

#ifndef ATT_aceWifiMgr_setLogLevel
#define ATT_aceWifiMgr_setLogLevel 1
#endif

#ifndef ATT_mtsWifi_setTestConfig
#define ATT_mtsWifi_setTestConfig 1
#endif

#ifndef ATT_test_init_deinit
#define ATT_test_init_deinit 1
#endif

#ifndef ATT_test_addNetwork_01
#define ATT_test_addNetwork_01 1
#endif

#ifndef ATT_test_addNetwork_02
#define ATT_test_addNetwork_02 1
#endif

#ifndef ATT_test_connect_03
#define ATT_test_connect_03 1
#endif

#ifndef ATT_test_registerWifiReadyEvent_09
#define ATT_test_registerWifiReadyEvent_09 1
#endif

#ifndef ATT_test_registerWifiReadyEvent_10
#define ATT_test_registerWifiReadyEvent_10 1
#endif

#ifndef ATT_test_setCountryCode_11
#define ATT_test_setCountryCode_11 1
#endif

#ifndef ATT_test_setCountryCode_12
#define ATT_test_setCountryCode_12 1
#endif

#ifndef ATT_test_scan_13
#define ATT_test_scan_13 1
#endif

#ifndef ATT_test_scan_detailed_scan_result_14
#define ATT_test_scan_detailed_scan_result_14 1
#endif

#ifndef ATT_test_getConfiguredNetwork_15
#define ATT_test_getConfiguredNetwork_15 1
#endif

#ifndef ATT_test_networkStateEvent_16
#define ATT_test_networkStateEvent_16 1
#endif

#ifndef ATT_test_deregisterNetworkStateEvent_17
#define ATT_test_deregisterNetworkStateEvent_17 1
#endif

#ifndef ATT_test_getConnectionInfo_18
#define ATT_test_getConnectionInfo_18 1
#endif

#ifndef ATT_test_enableAllNetworks_19
#define ATT_test_enableAllNetworks_19 1
#endif

#ifndef ATT_test_enableAllNetworks_20
#define ATT_test_enableAllNetworks_20 1
#endif

#ifndef ATT_test_disableAllNetworks_21
#define ATT_test_disableAllNetworks_21 1
#endif

#ifndef ATT_test_getMacAddress_23
#define ATT_test_getMacAddress_23 1
#endif

#ifndef ATT_wifi_tests_mgr_setUp
#define ATT_wifi_tests_mgr_setUp 1
#endif

#ifndef ATT_wifi_tests_mgr_tearDown
#define ATT_wifi_tests_mgr_tearDown 1
#endif

#ifndef ATT_aceBTCli_init
#define ATT_aceBTCli_init 1
#endif

#ifndef ATT_aceBTCli_deinit
#define ATT_aceBTCli_deinit 1
#endif

#ifndef ATT_aceBTCli_enable
#define ATT_aceBTCli_enable 1
#endif

#ifndef ATT_aceBTCli_disable
#define ATT_aceBTCli_disable 1
#endif

#ifndef ATT_do_getAdapterName
#define ATT_do_getAdapterName 1
#endif

#ifndef ATT_do_setAdapterName
#define ATT_do_setAdapterName 1
#endif

#ifndef ATT_do_getAdapterProp
#define ATT_do_getAdapterProp 1
#endif

#ifndef ATT_do_get_radio_state
#define ATT_do_get_radio_state 1
#endif

#ifndef ATT_test_shmem_getsize_test
#define ATT_test_shmem_getsize_test 1
#endif

#ifndef ATT_test_shmem_close_test
#define ATT_test_shmem_close_test 1
#endif

#ifndef ATT_aceBTCli_ble_registerBeaconClient
#define ATT_aceBTCli_ble_registerBeaconClient 1
#endif

#ifndef ATT_aceBTCli_ble_deregisterBeaconClient
#define ATT_aceBTCli_ble_deregisterBeaconClient 1
#endif

#ifndef ATT_aceBTCli_ble_startBeacon
#define ATT_aceBTCli_ble_startBeacon 1
#endif

#ifndef ATT_aceBTCli_ble_stopBeacon
#define ATT_aceBTCli_ble_stopBeacon 1
#endif

#ifndef ATT_aceBTCli_ble_createService
#define ATT_aceBTCli_ble_createService 1
#endif

#ifndef ATT_aceBTCli_ble_addIncludedService
#define ATT_aceBTCli_ble_addIncludedService 1
#endif

#ifndef ATT_aceBTCli_ble_addCharacteristics
#define ATT_aceBTCli_ble_addCharacteristics 1
#endif

#ifndef ATT_aceBTCli_ble_addCharacteristicsWithDesc
#define ATT_aceBTCli_ble_addCharacteristicsWithDesc 1
#endif

#ifndef ATT_aceBTCli_ble_registerGattServer
#define ATT_aceBTCli_ble_registerGattServer 1
#endif

#ifndef ATT_aceBTCli_ble_deregisterGattServer
#define ATT_aceBTCli_ble_deregisterGattServer 1
#endif

#ifndef ATT_do_send_response
#define ATT_do_send_response 1
#endif

#ifndef ATT_do_send_notification
#define ATT_do_send_notification 1
#endif

#ifndef ATT_do_get_mtu_req
#define ATT_do_get_mtu_req 1
#endif

#ifndef ATT_do_conn_prio_req
#define ATT_do_conn_prio_req 1
#endif

#endif /* RT106A_ACE_CONFIG_H_ */