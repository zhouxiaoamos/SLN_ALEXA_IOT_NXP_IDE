/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOSConfig.h"
#include "dhcp_server.h"
#include "lwip/dhcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/prot/dhcp.h"
#include "lwip/tcpip.h"
#include "tcpip_manager.h"
#include "wwd_constants.h"
#include "wwd_network.h"

#define DHCP_RETRIES_CNT 150

static struct netif g_sta_interface;
static struct netif g_ap_interface;


TCPIPReturnCode_t TCPIP_MANAGER_init(void)
{
    static bool inited = false;

    if (inited == false)
    {
        tcpip_init(NULL, NULL);
        inited = true;
    }

    return eTCPIPSuccess;
}

TCPIPReturnCode_t TCPIP_MANAGER_start_sta_interface(bool dhcp_use, volatile bool *connection_state)
{
    TCPIPReturnCode_t status;
    struct netif *netif_status;
    err_t dhcp_status;
    ip4_addr_t ipaddr, netmask, gw;
    struct dhcp *dhcp;
    uint8_t retries;

    if (dhcp_use)
    {
        IP4_ADDR(&ipaddr,  0, 0, 0, 0);
        IP4_ADDR(&netmask, 0, 0, 0, 0);
        IP4_ADDR(&gw,      0, 0, 0, 0);
    }
    else
    {
        IP4_ADDR(&ipaddr,  configIP_ADDR0,       configIP_ADDR1,       configIP_ADDR2,       configIP_ADDR3);
        IP4_ADDR(&netmask, configNET_MASK0,      configNET_MASK1,      configNET_MASK2,      configNET_MASK3);
        IP4_ADDR(&gw,      configGATEWAY_ADDR0,  configGATEWAY_ADDR1,  configGATEWAY_ADDR2,  configGATEWAY_ADDR3);
    }

    netif_status = netif_add(&g_sta_interface, &ipaddr, &netmask, &gw,
            (void *)WWD_STA_INTERFACE, wlanif_init, tcpip_input);
    if (netif_status != NULL)
    {
        netif_set_default(&g_sta_interface);
        netif_set_up(&g_sta_interface);
        status = eTCPIPSuccess;
    }
    else
    {
        status = eTCPIPFailure;
    }

    if ((status == eTCPIPSuccess) && (dhcp_use == true))
    {
        dhcp_status = dhcp_start(&g_sta_interface);
        if (dhcp_status == ERR_OK)
        {
            dhcp = (struct dhcp *)netif_get_client_data(&g_sta_interface, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
            for (retries = 0; retries < DHCP_RETRIES_CNT; retries++)
            {
                if (dhcp->state == DHCP_STATE_BOUND)
                {
                    break;
                }
                else
                {
                    if ((bool)WICED_FALSE == *connection_state)
                    {
                        status = eTCPIPDhcpCLientFailed;
                        break;
                    }
                }
                vTaskDelay(100);
            }
            if (dhcp->state != DHCP_STATE_BOUND)
            {
                status = eTCPIPDhcpCLientFailed;
            }
        }
        else
        {
            status = eTCPIPFailure;
        }
    }

    return status;
}

TCPIPReturnCode_t TCPIP_MANAGER_quit_sta_interface(void)
{
    dhcp_release_and_stop(&g_sta_interface);
    netif_remove(&g_sta_interface);

    return eTCPIPSuccess;
}

TCPIPReturnCode_t TCPIP_MANAGER_get_ip_sta_interface(tcpip_manager_ip_info_t *ip_info)
{
    TCPIPReturnCode_t status;

    if (ip_info != NULL)
    {
        ip4_addr_copy(ip_info->ip,      g_sta_interface.ip_addr);
        ip4_addr_copy(ip_info->gw,      g_sta_interface.gw);
        ip4_addr_copy(ip_info->netmask, g_sta_interface.netmask);

        status = eTCPIPSuccess;
    }
    else
    {
        status = eTCPIPFailure;
    }

    return status;
}

TCPIPReturnCode_t TCPIP_MANAGER_get_ip_only_sta_interface(uint32_t *ip)
{
    TCPIPReturnCode_t status;

    if (ip != NULL)
    {
        *ip = g_sta_interface.ip_addr.addr;

        status = eTCPIPSuccess;
    }
    else
    {
        status = eTCPIPFailure;
    }

    return status;
}

TCPIPReturnCode_t TCPIP_MANAGER_link_sta_up(void)
{
    netif_set_link_up(&g_sta_interface);
    return eTCPIPSuccess;
}

TCPIPReturnCode_t TCPIP_MANAGER_link_sta_down(void)
{
    netif_set_link_down(&g_sta_interface);
    return eTCPIPSuccess;
}

TCPIPReturnCode_t TCPIP_MANAGER_start_ap_interface(void)
{
    TCPIPReturnCode_t status;
    struct netif *netif_status;
    ip4_addr_t ipaddr, netmask, gw;

    IP4_ADDR(&ipaddr,  configGATEWAY_ADDR0,  configGATEWAY_ADDR1,  configGATEWAY_ADDR2,  configGATEWAY_ADDR3);
    IP4_ADDR(&netmask, configNET_MASK0,      configNET_MASK1,      configNET_MASK2,      configNET_MASK3);
    IP4_ADDR(&gw,      configGATEWAY_ADDR0,  configGATEWAY_ADDR1,  configGATEWAY_ADDR2,  configGATEWAY_ADDR3);

    netif_status = netif_add(&g_ap_interface, &ipaddr, &netmask, &gw,
            (void *)WWD_AP_INTERFACE, wlanif_init, tcpip_input);
    if (netif_status != NULL)
    {
        netif_set_default(&g_ap_interface);
        netif_set_up(&g_ap_interface);
        status = eTCPIPSuccess;
    }
    else
    {
        status = eTCPIPFailure;
    }

    /* Start the DHCP server*/
    if (status == eTCPIPSuccess)
    {
        start_dhcp_server(ipaddr.addr);
    }

    return status;
}

TCPIPReturnCode_t TCPIP_MANAGER_quit_ap_interface(void)
{
    quit_dhcp_server();
    netif_remove(&g_ap_interface);
    return eTCPIPSuccess;
}


