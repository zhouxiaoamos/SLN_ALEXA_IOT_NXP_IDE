/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef TCPIP_MANAGER_H_
#define TCPIP_MANAGER_H_

#include "lwip/ip_addr.h"

/* Default IP address configuration.  Used in ipconfigUSE_DHCP is set to 0, or
 * ipconfigUSE_DHCP is set to 1 but a DNS server cannot be contacted. */
#define configIP_ADDR0                       192
#define configIP_ADDR1                       168
#define configIP_ADDR2                       1
#define configIP_ADDR3                       222

/* Default gateway IP address configuration.  Used in ipconfigUSE_DHCP is set to
 * 0, or ipconfigUSE_DHCP is set to 1 but a DNS server cannot be contacted. */
#define configGATEWAY_ADDR0                  192
#define configGATEWAY_ADDR1                  168
#define configGATEWAY_ADDR2                  1
#define configGATEWAY_ADDR3                  1

/* Default DNS server configuration.  OpenDNS addresses are 208.67.222.222 and
 * 208.67.220.220.  Used in ipconfigUSE_DHCP is set to 0, or ipconfigUSE_DHCP is
 * set to 1 but a DNS server cannot be contacted.*/
#define configDNS_SERVER_ADDR0               208
#define configDNS_SERVER_ADDR1               67
#define configDNS_SERVER_ADDR2               222
#define configDNS_SERVER_ADDR3               222

/* Default netmask configuration.  Used in ipconfigUSE_DHCP is set to 0, or
 * ipconfigUSE_DHCP is set to 1 but a DNS server cannot be contacted. */
#define configNET_MASK0                      255
#define configNET_MASK1                      255
#define configNET_MASK2                      0
#define configNET_MASK3                      0

typedef enum
{
    eTCPIPSuccess          = 0,
    eTCPIPFailure          = 1,
    eTCPIPDhcpCLientFailed = 2,
} TCPIPReturnCode_t;

typedef struct {
    ip4_addr_t ip;
    ip4_addr_t netmask;
    ip4_addr_t gw;
} tcpip_manager_ip_info_t;

TCPIPReturnCode_t TCPIP_MANAGER_init(void);

TCPIPReturnCode_t TCPIP_MANAGER_start_sta_interface(bool dhcp_use, volatile bool *connection_state);
TCPIPReturnCode_t TCPIP_MANAGER_quit_sta_interface(void);
TCPIPReturnCode_t TCPIP_MANAGER_get_ip_sta_interface(tcpip_manager_ip_info_t *ip_info);
TCPIPReturnCode_t TCPIP_MANAGER_get_ip_only_sta_interface(uint32_t *ip);
TCPIPReturnCode_t TCPIP_MANAGER_link_sta_up(void);
TCPIPReturnCode_t TCPIP_MANAGER_link_sta_down(void);

TCPIPReturnCode_t TCPIP_MANAGER_start_ap_interface(void);
TCPIPReturnCode_t TCPIP_MANAGER_quit_ap_interface(void);

#endif /* TCPIP_MANAGER_H_ */
