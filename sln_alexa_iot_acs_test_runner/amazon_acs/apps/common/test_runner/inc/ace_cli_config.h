/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file. This file is a
 * Modifiable File, as defined in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */
/*
 *
 */

/**
 * ACE CLIs configs
 *
 * The naming convention to enable an ACE CLI with macro is:
 *
 * #define CLI_xxx 1
 *     xxx stands for the "_" concatenated ace cli name, for example:
 *     if cli entry is: "ace mw wifi_cli scan", the macro should be
 *CLI_ace_mw_wifi_cli_scan
 *
 * The full list of names is under
 *  "$installed_ace/ace/include/ace/ace_cli_cmds_config.h"
 **/

/* Show heap stats */
#ifndef CLI_ace_tools_sys_heap
#define CLI_ace_tools_sys_heap 1
#endif

/* Show task stats */
#ifndef CLI_ace_tools_sys_task
#define CLI_ace_tools_sys_task 1
#endif

/*Enable FFS CLIs */
#ifndef CLI_ace_mw_ffs
#define CLI_ace_mw_ffs 1
#endif

/* - perform a scan */
#ifndef CLI_ace_mw_wifi_cli_scan
#define CLI_ace_mw_wifi_cli_scan 1
#endif

/* ssid=<ssid> [psk=<psk> | wep0=<wep0>\n [wep1=<wep1> wep2=<wep2>\n
 * wep3=<wep3>] wepIdx=<wepIdx>]\n [hidden=<0 | 1>]\n - create a profile */
#ifndef CLI_ace_mw_wifi_cli_add_network
#define CLI_ace_mw_wifi_cli_add_network 1
#endif

/* <ssid> [open|wep|wpa|wpa2|eap]\n - delete a profile */
#ifndef CLI_ace_mw_wifi_cli_remove_network
#define CLI_ace_mw_wifi_cli_remove_network 1
#endif

/* - save config */
#ifndef CLI_ace_mw_wifi_cli_save_config
#define CLI_ace_mw_wifi_cli_save_config 1
#endif

/* <ssid> [open|wep|wpa|wpa2|eap]\n - connect to <ssid> network */
#ifndef CLI_ace_mw_wifi_cli_connect
#define CLI_ace_mw_wifi_cli_connect 1
#endif

/* - disconnect current network */
#ifndef CLI_ace_mw_wifi_cli_disconnect
#define CLI_ace_mw_wifi_cli_disconnect 1
#endif

/* <code>\n - set country code to <code> */
#ifndef CLI_ace_mw_wifi_cli_set_country
#define CLI_ace_mw_wifi_cli_set_country 1
#endif

/* [ssid=<ssid> [psk=<psk>] [freq=<freq>]]\n - create soft AP (create default w/
 * no args) */
#ifndef CLI_ace_mw_wifi_cli_create_ap
#define CLI_ace_mw_wifi_cli_create_ap 1
#endif

/* - remove current soft AP */
#ifndef CLI_ace_mw_wifi_cli_remove_ap
#define CLI_ace_mw_wifi_cli_remove_ap 1
#endif

/* - enable all networks */
#ifndef CLI_ace_mw_wifi_cli_enable_all
#define CLI_ace_mw_wifi_cli_enable_all 1
#endif

/* - disable all networks */
#ifndef CLI_ace_mw_wifi_cli_disable_all
#define CLI_ace_mw_wifi_cli_disable_all 1
#endif

/* <srcIP> <srcPort> <destIP> <destPort>\n - enable TCP tunnel */
#ifndef CLI_ace_mw_wifi_cli_enable_tcp
#define CLI_ace_mw_wifi_cli_enable_tcp 1
#endif

/* - disable TCP tunnel */
#ifndef CLI_ace_mw_wifi_cli_disable_tcp
#define CLI_ace_mw_wifi_cli_disable_tcp 1
#endif

/* - enable NAT */
#ifndef CLI_ace_mw_wifi_cli_enable_nat
#define CLI_ace_mw_wifi_cli_enable_nat 1
#endif

/* - disable NAT */
#ifndef CLI_ace_mw_wifi_cli_disable_nat
#define CLI_ace_mw_wifi_cli_disable_nat 1
#endif

/* - evaluate captive portal */
#ifndef CLI_ace_mw_wifi_cli_eval_captive
#define CLI_ace_mw_wifi_cli_eval_captive 1
#endif

/* - get scan results */
#ifndef CLI_ace_mw_wifi_cli_get_scan_results
#define CLI_ace_mw_wifi_cli_get_scan_results 1
#endif

/* - get detailed scan results */
#ifndef CLI_ace_mw_wifi_cli_get_detailed_scan_results
#define CLI_ace_mw_wifi_cli_get_detailed_scan_results 1
#endif

/* - get mac_address */
#ifndef CLI_ace_mw_wifi_cli_get_mac
#define CLI_ace_mw_wifi_cli_get_mac 1
#endif

/* - get configured networks */
#ifndef CLI_ace_mw_wifi_cli_get_config
#define CLI_ace_mw_wifi_cli_get_config 1
#endif

/* - get is wifi ready */
#ifndef CLI_ace_mw_wifi_cli_get_wifi_ready
#define CLI_ace_mw_wifi_cli_get_wifi_ready 1
#endif

/* - get connection info */
#ifndef CLI_ace_mw_wifi_cli_get_conn_info
#define CLI_ace_mw_wifi_cli_get_conn_info 1
#endif

/* - get network state */
#ifndef CLI_ace_mw_wifi_cli_get_net_state
#define CLI_ace_mw_wifi_cli_get_net_state 1
#endif

/* - get IP info */
#ifndef CLI_ace_mw_wifi_cli_get_ip_info
#define CLI_ace_mw_wifi_cli_get_ip_info 1
#endif

/* - get Statistic info */
#ifndef CLI_ace_mw_wifi_cli_get_statistic
#define CLI_ace_mw_wifi_cli_get_statistic 1
#endif

/* - get Capability info */
#ifndef CLI_ace_mw_wifi_cli_get_capability
#define CLI_ace_mw_wifi_cli_get_capability 1
#endif

/* - set ace wifi log level */
#ifndef CLI_ace_mw_wifi_cli_set_loglevel
#define CLI_ace_mw_wifi_cli_set_loglevel 1
#endif

/* - get ace wifi log level */
#ifndef CLI_ace_mw_wifi_cli_get_loglevel
#define CLI_ace_mw_wifi_cli_get_loglevel 1
#endif

/* - <x.x.x.x> [<num> [<interval_ms>]]\n- ping IP address */
#ifndef CLI_ace_mw_wifi_cli_ping
#define CLI_ace_mw_wifi_cli_ping 1
#endif

/* - DNS look up\n- DNS look up */
#ifndef CLI_ace_mw_wifi_cli_get_host_ip
#define CLI_ace_mw_wifi_cli_get_host_ip 1
#endif

/* ACE Maplite commands */
#ifndef CLI_ace_mw_maplite
#define CLI_ace_mw_maplite 1
#endif

/* Dump dispatcher info */
#ifndef CLI_ace_dispatcher_debug
#define CLI_ace_dispatcher_debug 1
#endif

/* Set verbosity for dispatcher */
#ifndef CLI_ace_dispatcher_set_verbosity
#define CLI_ace_dispatcher_set_verbosity 1
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

/* Test ACE OSAL Mutex */
#ifndef CLI_ace_osal_mutex_test
#define CLI_ace_osal_mutex_test 1
#endif

/* Test ACE OSAL Message Queue */
#ifndef CLI_ace_osal_queue_test
#define CLI_ace_osal_queue_test 1
#endif

/* LED Hal test */
#ifndef CLI_ace_hal_led_test
#define CLI_ace_hal_led_test 1
#endif

/* KVS Hal test */
#ifndef CLI_ace_hal_kvs_test
#define CLI_ace_hal_kvs_test 1
#endif

/* DHA Hal test */
#ifndef CLI_ace_hal_dha_test
#define CLI_ace_hal_dha_test 1
#endif

/* Generate DHA Key Pair */
#ifndef CLI_ace_hal_dha_key_gen
#define CLI_ace_hal_dha_key_gen 1
#endif

/* Generate Public key from DHA Key Pair */
#ifndef CLI_ace_hal_dha_get_pub
#define CLI_ace_hal_dha_get_pub 1
#endif

/* Sign SHA256 Digest of a message */
#ifndef CLI_ace_hal_dha_sign
#define CLI_ace_hal_dha_sign 1
#endif

/* Verify SHA256 Digest of a message */
#ifndef CLI_ace_hal_dha_verify
#define CLI_ace_hal_dha_verify 1
#endif

/* Get data fields */
#ifndef CLI_ace_hal_dha_get_field
#define CLI_ace_hal_dha_get_field 1
#endif

/* Set certificate chain */
#ifndef CLI_ace_hal_dha_set_cert
#define CLI_ace_hal_dha_set_cert 1
#endif

/* Device Info Hal Cli */
#ifndef CLI_ace_hal_device_info_cli
#define CLI_ace_hal_device_info_cli 1
#endif

/* Device Info Hal test */
#ifndef CLI_ace_hal_device_info_test
#define CLI_ace_hal_device_info_test 1
#endif

/* Button Hal test */
#ifndef CLI_ace_hal_button_test
#define CLI_ace_hal_button_test 1
#endif

/* IoT Device Tester Hal AFQP test */
#ifndef CLI_ace_hal_idt_test_afqp
#define CLI_ace_hal_idt_test_afqp 1
#endif

/* IoT Device Tester Hal Stress test */
#ifndef CLI_ace_hal_idt_test_stress
#define CLI_ace_hal_idt_test_stress 1
#endif

/* IoT Device Tester Hal KPI test */
#ifndef CLI_ace_hal_idt_test_kpi
#define CLI_ace_hal_idt_test_kpi 1
#endif

/* IoT Device Tester Hal Integration test */
#ifndef CLI_ace_hal_idt_test_integration
#define CLI_ace_hal_idt_test_integration 1
#endif

/* IoT Device Tester Hal Stack Init test */
#ifndef CLI_ace_hal_idt_test_stack_init
#define CLI_ace_hal_idt_test_stack_init 1
#endif

/* Bluetooth Hal Stress test */
#ifndef CLI_ace_hal_bt_test_stress
#define CLI_ace_hal_bt_test_stress 1
#endif

/* Bluetooth Hal KPI test */
#ifndef CLI_ace_hal_bt_test_kpi
#define CLI_ace_hal_bt_test_kpi 1
#endif

/* Bluetooth Hal Integration test */
#ifndef CLI_ace_hal_bt_test_integration
#define CLI_ace_hal_bt_test_integration 1
#endif

/* Bluetooth Hal Stack Init test */
#ifndef CLI_ace_hal_bt_test_stack_init
#define CLI_ace_hal_bt_test_stack_init 1
#endif

/* Enable wifi hal commands */
#ifndef CLI_ace_hal_wifi
#define CLI_ace_hal_wifi 1
#endif

/* Start wifi hal integration test */
#ifndef CLI_ace_hal_wifi_test
#define CLI_ace_hal_wifi_test 1
#endif

/* Factory Reset CLI commands */
#ifndef CLI_ace_mw_factory_reset
#define CLI_ace_mw_factory_reset 1
#endif

/* KVS Hal Cli */
#ifndef CLI_ace_hal_kvs_cli
#define CLI_ace_hal_kvs_cli 1
#endif


/* capture <Time> [Print Out Samples] [Print Delay in ms] */
#ifndef CLI_ace_hal_audio_capture
#define CLI_ace_hal_audio_capture 1
#endif

/* loop <Time> */
#ifndef CLI_ace_hal_audio_loop
#define CLI_ace_hal_audio_loop 1
#endif

/* set <fs 48000>/<fc/l_fc/r_fc 125>/<tx port>/<rx port>/<m_vol/l_vol/r_vol 16384> */
#ifndef CLI_ace_hal_audio_set
#define CLI_ace_hal_audio_set 1
#endif

/* AUDIO Hal unit test */
#ifndef CLI_ace_hal_audio_test
#define CLI_ace_hal_audio_test 1
#endif

/* tone <Time> */
#ifndef CLI_ace_hal_audio_tone
#define CLI_ace_hal_audio_tone 1
#endif
