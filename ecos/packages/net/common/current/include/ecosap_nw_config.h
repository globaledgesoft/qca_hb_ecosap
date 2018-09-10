/*
 * Copyright (c) 2018, Global Edge Software Ltd.
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Company mentioned in the Copyright nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER AND CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ECOSAP_NW_CONFIG_H
#define __ECOSAP_NW_CONFIG_H

#include <pkgconf/system.h>

#define WAN "eth0"

/* Set option for Bridge support here; Enabled by default*/

#ifdef CYGPKG_COMPONENT_BRIDGE
#define BRIDGE_SUPPORT 1
#endif

#ifdef BRIDGE_SUPPORT

#define NUM_OF_PORTS 2

#define BRIDGE "br0"
#define BRIDGE_PORT0 "eth1"
//#define BRIDGE_PORT1 "eth2"
#define Server_Int BRIDGE

#else
/* if no bridge support */

#define LAN1 "eth1"
#define Server_Int LAN1
#endif

#define BRIDGE_DEBUG 1                                                          

/* Macro to sepcify WAN interface when VLAN support is provided*/
//#define WAN_VLAN "eth0v4"

#define INET_LOCAL_VLANID 3
//#define INET_LOCAL_ISTAGGED ACCESS_MODE
#define INET_LOCAL_ISTAGGED 0

/* Macro for BRIDGE debug statements*/                                     
#ifdef BRIDGE_DEBUG                                                             
    #define printf(...)                                                         
#endif     

/* Macros for IPFW configuration support */ 
#define MAX_IPFW_RULE_ARGC 16
#define MAX_IPFW_RULE_ARGS_SIZE 24
#define MAX_IPFW_RULES 64
#define LAN_IFNAME "br0"
#define WAN_INTERFACE WAN 
#define WAN_INTERFACE_PPP "ppp0" 
#define DEFAULT_NAT_CONFIG_ID "123"
#define DEFAULT_REDIR_NAT_CONFIG_ID "122"
#define DEFAULT_REDIR_NAT_CONFIG_NUM 122
#define IPFW_SAME_NAT_CONFIG 1
#define IPFW_SAME_NAT_PROTO_EXTPORT_CONFIG 2

#define SIMPLE_NAT_RULE_ID "17001"
#define RED_NAT_RULE_ID "18001"
#define DENY_RULE1 "6001"
#define DENY_RULE2 "6002"
#define DENY_RULE3 "6003"
#define DENY_RULE4 "6004"

#define NAT_USER_MIN_ID 8000
#define NAT_USER_MAX_ID 10000

/* Macros used in vpnpassthrough */
#define PPTP_TCP1 "4001"
#define PPTP_TCP2 "4002"
#define PPTP_GRE "4003"
#define PPTP_UDP_IPSEC1 "4004"
#define PPTP_UDP_IPSEC2 "4005"
#define PPTP_UDP_IPSEC3 "4006"
#define PPTP_UDP_IPSEC4 "4007"
#define PPTP_UDP_L2TP1 "4008"
#define PPTP_UDP_L2TP2 "4009"
#define PPTP_ESP_IPSEC "4010"
#define PPTP_AH_IPSEC "4011"

#define FW_RULE_ID_NUM1 6001
#define FW_RULE_ID_NUM2 6002
#define FW_RULE_ID_NUM3 6003
#define FW_RULE_ID_NUM4 6004
#define FW_RULE_ID_NUM5 10202
#define FW_RULE_ID_NUM6 10301
#define FW_RULE_ID_NUM7 10302
#define FW_CHECK_RULE_NUM 11001

#define DHCP_RULE "1000"
#define SNTP_RULE1 "7001"
#define SNTP_RULE2 "7002"
#define ICMP_ME "13001"

#define HTTP_LAN1 "2001"
#define HTTP_LAN2 "2002"

#define HTTP_RULE1_ID "3001"
#define HTTP_RULE2_ID "3002"
#define HTTP_RULE3_ID "3003"
#define HTTP_RULE4_ID "3004"
#define HTTP_RULE5_ID "3005"
#define HTTP_RULE6_ID "3006"

/* DMZ firewall rule ids */
#define DMZ_NAT_CONFIG_WAN "124"
#define DMZ_NAT_CONFIG_LAN "125"
#define DMZ_NAT_CONFIG1 125
#define DMZ_NAT_CONFIG2 124

#define DMZ_NAT_FW_RULE "16001"
#define DMZ_ICMP "10201"
#define LANSUB_TO_WANSUB_LAN "10202"
#define WANSUB_TO_LANSUB_WAN "10301"
#define LANSUB_TO_WANSUB_WAN "10302"
#define FW_CHECKSTATE "11001"
#define FW_KEEPSTATE "12001"
#define INTERFACE_SIZE 10
#define FTP_DST_PORT "10101"
#define FTP_SRC_PORT "10102"
#define DMZ_FTP_WIF_ID "10001"
#define DMZ_FTP_WHOST_ID "10002"
#define DMZ_FTP_FTWH_ID "10003"
#define DMZ_FTP_WHFT_ID "10004"
#define DMZ_ICMP_NUM 10201
#define LANSUB_TO_WANSUB_WAN_NUM 10302

/* FTP firwall rule ids */
#define FTP_RULE1 "14001"
#define FTP_RULE2 "14002"
#define FTP_RULE3 "14003"
#define FTP_RULE4 "14004"

/* Rule to deny traffic from WAN to LAN */
#define DENY_FROM_WAN "15001"

/* Macros for different ports */
#define HTTP_WAN_PORT "8080"
#define HTTP_LAN_PORT "80"
#define PPTP_IPSEC_PORT1 "500"
#define PPTP_IPSEC_PORT2 "4500"
#define PPTP_L2TP_PORT "1701"
char ftp_nat_config_id[16];
char ftp_ipfw_rule1_id[16];
char ftp_ipfw_rule2_id[16];
int is_ftp_status;
/* Macros for dhcpd configuration support //TODO*/                                     
#define DHCPS_POOL_START_ADDR_IP "192.168.0.2"                                  
#define DHCPS_POOL_END_ADDR_IP "192.168.0.200"          
#define DHCPS_ADDR_MASK "255.255.255.0"                        
#define DHCPS_LEASE_TIME (24 * 60 * 60)                                         
#define DHCPS_MAX_LEASES 200                       
#define DHCPS_SERVER_PORT 67
//#define DHCPS_THREAD_PRIORITY 6
//#define DHCPS_STACK_SIZE 0x4000


/* Macros defined default for DNS */
#define CYGNUM_NS_DNS_GETADDRINFO_ADDRESSES 5
#define CYGOPT_NS_DNS_FIRST_FAMILY AF_INET6

/*Macros defined for httpd */
#define CYGNUM_HTTPD_SERVER_LAN_PORT 80
#ifdef  CYGPKG_COMPONENT_REMOTE_HTTP
#define CYGNUM_HTTPD_SERVER_WAN_PORT 8080
#define CYGNUM_HTTPD_THREAD_COUNT 2
#else // Only one HTTP server thread with port 80
#define CYGNUM_HTTPD_THREAD_COUNT 1
#endif
//#define CYGNUM_HTTPD_SERVER_AUTO_START 0
#define CYGNUM_HTTPD_SERVER_DELAY 0

/*Macros for PPPoE*/
#define CYGPKG_PPP_DEBUG_WARN_ONLY 1

/*Macros for DHCPC */
#define CYGPKG_NET_DHCP 1 			//Use full dhcp instead of BOOTP
#define CYGOPT_NET_DHCP_DHCP_THREAD 1		//Provide separate thread for renewal of lease
#define CYGOPT_NET_DHCP_DHCP_THREAD_PARAM 1 	//DHCPC renewal thread loops forever

/*Macros for eth0 and eth1 initialisation */
#define CYGHWR_NET_DRIVER_ETH0 1
#define CYGHWR_NET_DRIVER_ETH0_DHCP 1		//Use DHCP rather than BOOTP foe WAN


/* Macros for IPFW */
#define CYGNUM_NET_IPFW_BUFSIZE 2048

/* Macros for Bridge */
#define CYGPKG_NET_BRIDGE 1
#define CYGNUM_NET_BRIDGES 1

extern void start_nw_bridge_interface(void);
extern void start_nw_lan_interface(void);
extern void start_bridge_interface(void);
extern void start_dhcp_sever(char *);
extern void start_httpd_sever(char *);
extern void set_config_value(const char *, const struct bootp *);
#endif//__ECOSAP_NW_CONFIG_H
