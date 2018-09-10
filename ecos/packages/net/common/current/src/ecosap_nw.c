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

#include <pkgconf/system.h>
#include <pkgconf/isoinfra.h>
#include <pkgconf/net.h>
#include <pkgconf/kernel.h>

#include <cyg/infra/cyg_trac.h>        /* tracing macros */
#include <cyg/infra/cyg_ass.h>         /* assertion macros */

#ifdef CYGPKG_COMPONENT_HTTPD
#include <cyg/httpd/httpd.h>
#include <pkgconf/httpd.h>
#endif

#include <cyg/kernel/kapi.h>
#ifdef CYGPKG_KERNEL_INSTRUMENT
#include <cyg/kernel/instrmnt.h>
#include <cyg/kernel/instrument_desc.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <cyg/hal/drv_api.h>

#define _KERNEL
#include <sys/param.h>
#undef _KERNEL
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _KERNEL

#include <sys/sysctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/route.h>
#include <net/if_dl.h>

#include <sys/protosw.h>
#include <netinet/in_pcb.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/ip_var.h>
#include <netinet/icmp_var.h>
#include <netinet/udp_var.h>
#include <netinet/tcp_var.h>
#ifdef CYGPKG_NET_INET6
#include <netinet/ip6.h>
#include <net/if_var.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_var.h>
#include <netinet/icmp6.h>
#endif

#include <sys/mbuf.h>
#include <cyg/io/eth/eth_drv_stats.h>
#include <netdb.h>
#include "ipfw2.h"
#include "ecosap_nw_http.h"
#include "ecosap_nw_config.h"

#define CONFIG_MAGIC_NUM 0x210818
extern void update_dns_ip(char *dns_ip);
extern int init_fs() ;

extern void watchdog_reboot(void);
extern void set_nw_lan_interface(void);

#define _string(s) #s
#define string(s) _string(s)

#define FLASH_VALIDATION 0

extern struct _s_x rule_actions[];
extern struct protoent protocols[];

#define ACTION_COUNT 29 
#define PROTO_COUNT 27 

/* Variables having VLAN interfaces names */
#ifdef CYGPKG_COMPONENT_IPTV
char VLAN1[16];
char VLAN2[16];
char VLAN3[16];
char VLAN4[16];
#endif

int is_flash_rule;

/*Writing net config to flash*/
void write_nw_flash(void){
	int n = cyg_flash_setconf((ECOSAP_CONFIG_STRUCT *) &ecosap_new_nw_config, sizeof(ECOSAP_CONFIG_STRUCT), 0);
	if(n < 0) {
		NET_ERROR("Default Set Failed...!\n");
	}
	else {
		NET_DEBUG("Written %d Bytes in Flash\n", n);
		NET_INFO("Flash Set Success...!\n");
	}
}
/* Function to check default configuration */
void ecosap_nw_default_start(void)
{
	int Config_Status = 0;

	Config_Status = Is_Config_Available();

	if (0 == Config_Status)
	{
		NET_INFO(" Setting Default Configuration in Flash...\n");
		ecosap_nw_config_default();
	}
	else
	{
		NET_INFO(" Reading Configuration from Flash...\n");
		ecosap_read_nw_config();
	}
	copy_nw_config_to_new_struct();
}

/* Function to check availabilty of default configuration */
int Is_Config_Available(void)
{
	int n = cyg_flash_getconf((ECOSAP_CONFIG_STRUCT *)&ecosap_config_struct, sizeof(ECOSAP_CONFIG_STRUCT),0);
	if(n < 0)
	{
		NET_ERROR("Couldn't able to fetch data from flash\n");
		return -1;
	}

	if (CONFIG_MAGIC_NUM == ecosap_config_struct.Magic_Number) {
		NET_INFO("Magic Number Matching...!\n");
		return 1;
        }
	else{
		NET_INFO("Magic Number NOT Matching... Flash Magic Value : [%u]", ecosap_config_struct.Magic_Number);
		return 0;
	}
}
void default_reboot(void)
{
	NET_CRITICAL("CONFIGURATION DATA IN FLASH IS NOT VALID\n");
	NET_CRITICAL("RECONFIGURING WITH DEFAULT CONFIGURATION ...!\n");
	cyg_thread_delay(500);
	ecosap_nw_config_default();
	//watchdog_reboot();
}
void is_flash_data_valid(void)
{

#if FLASH_VALIDATION
	int rule_argc, i, j, FLAG=0;
	char rule_argv[16][24];
	struct in_addr addr1, addr2, addr3 ;

	addr1.s_addr = ecosap_config_struct.bridgeConf.inaddr_ip ;
	addr2.s_addr = ecosap_config_struct.bridgeConf.inaddr_broad ;
	addr3.s_addr = ecosap_config_struct.bridgeConf.inaddr_mas ;

	if(isvalidip(ecosap_config_struct.wan_conf.WAN_config_addrs.IPAddr)!=1 		       ||
			isvalid_mask(ecosap_config_struct.wan_conf.WAN_config_addrs.Mask)!=1      || 
			isvalidip(ecosap_config_struct.wan_conf.WAN_config_addrs.Gateway)!=1   ||
			is_bcast_ip(ecosap_config_struct.wan_conf.WAN_config_addrs.Broadcast)!=1 ||
			isvalidip(ecosap_config_struct.wan_conf.WAN_config_addrs.DNS)!=1       ||
			isvalidip(ecosap_config_struct.wan_conf.WAN_config_addrs.ADNS)!=1      ||
			isvalidip(ecosap_config_struct.wan_conf.WAN_config_addrs.Server)!=1    ||
			isvalidip(ecosap_config_struct.dhcp_server_config.DHCP_From)!=1        ||
			isvalidip(ecosap_config_struct.dhcp_server_config.DHCP_To)!=1          ||	
			isvalidip(inet_ntoa(addr1))!=1                                         ||	
			is_bcast_ip(inet_ntoa(addr2))!=1                                         ||	
			isvalid_mask(inet_ntoa(addr3))!=1	    				       ||
			strlen(ecosap_config_struct.wan_conf.PPPoE_config_values.Username) >= 32 || 
			strlen(ecosap_config_struct.wan_conf.PPPoE_config_values.Passwd) >= 32   ||
			is_valid_mtu(ecosap_config_struct.wan_conf.PPPoE_config_values.MTU) != 1)
	    {
		default_reboot();
	}
	NET_DEBUG("Flash Rules:\n");
	for(i=0; i<5; i++){
		memset(rule_argv, '\0', sizeof(rule_argv)) ;
		rule_argc = ecosap_config_struct.ipfw_config[i].ipfw_argc;
		for(j=0; j<rule_argc; j++)
			strlcpy(rule_argv[j], ecosap_config_struct.ipfw_config[i].ipfw_rules[j], MAX_IPFW_RULE_ARGS_SIZE);
		for(j=0; j<rule_argc; j++)
			NET_DEBUG("%s ", rule_argv[j]);

		if(strcmp(rule_argv[1], "add") == 0){
			if(isvalid_ipfw_cmd(rule_argv) == 1){
				NET_DEBUG("ITS VALID FLASH RULE\n");
			}
			else{
				FLAG=1;
			}
		}
		else if(strcmp(rule_argv[1], "nat") == 0){
			if(rule_argc == 10){
				if(is_valid_nat_cmd(rule_argv) == 1){
					NET_DEBUG("ITS VALID FLASH NAT REDIRECT RULE\n");
				}
				else{
					FLAG=1;
				}


			}
			else if(rule_argc == 6){
				if(is_valid_nat_config(rule_argv) == 1){
					NET_DEBUG("ITS VALID FLASH NAT CONFIG RULE\n");
				}
				else{
					FLAG=1;
				}

			}
			else{
				FLAG=1;
				NET_DEBUG("Invalid Argument Count\n");
			}
		}
		NET_DEBUG("\n");
	}
	if(FLAG == 1){
		default_reboot();
	}
#endif
}

/** Function to set default network new configuration values **/
void copy_nw_config_to_new_struct(void)
{
	memset((ECOSAP_CONFIG_STRUCT *)&ecosap_new_nw_config, '\0', sizeof(ecosap_new_nw_config));
	memcpy((ECOSAP_CONFIG_STRUCT *)&ecosap_new_nw_config, (ECOSAP_CONFIG_STRUCT *)&ecosap_config_struct, sizeof(ecosap_config_struct));
}

/* Function to read default configuration */
void ecosap_read_nw_config(void)
{
	is_flash_data_valid();

#ifdef CYGPKG_COMPONENT_IPTV
	char VLANID1[8];
	char VLANID2[8];
	char VLANID3[8];

	if (ecosap_config_struct.vlan_config.vlan_flag) {
		strcpy(VLAN1, "eth0v");
		strcpy(VLAN2, "eth0v");
		strcpy(VLAN3, "eth1v");
		strcpy(VLAN4, "eth1v");

		sprintf(VLANID1, "%d", ecosap_config_struct.vlan_config.inet_value);
		sprintf(VLANID2, "%d", ecosap_config_struct.vlan_config.iptv_value);
		sprintf(VLANID3, "%d", INET_LOCAL_VLANID);

		strcat(VLAN1, VLANID1);
		strcat(VLAN2, VLANID2);
		strcat(VLAN3, VLANID2);
		strcat(VLAN4, VLANID3);
	}
#endif

	NET_DEBUG("WAN configuration\n") ;
    if(ecosap_config_struct.Connection_Type == PPPoE){
#ifdef CYGPKG_COMPONENT_PPPoE		
		NET_DEBUG("PPPoE\n") ;
		NET_DEBUG("Username	: %s \n", ecosap_config_struct.wan_conf.PPPoE_config_values.Username) ;
		NET_DEBUG("Password	: %s \n", ecosap_config_struct.wan_conf.PPPoE_config_values.Passwd) ;
		NET_DEBUG("MTU		: %u \n", ecosap_config_struct.wan_conf.PPPoE_config_values.MTU) ;
		NET_DEBUG("Service Name: %s \n", ecosap_config_struct.wan_conf.PPPoE_config_values.Service_Name) ;
		NET_DEBUG("Server Name : %s \n", ecosap_config_struct.wan_conf.PPPoE_config_values.Server_Name) ;
		NET_DEBUG("Max Idle Value:%lu \n", ecosap_config_struct.wan_conf.PPPoE_config_values.MAX_Idle_Time) ;
#endif //PPPoE
	}
	else{
		NET_DEBUG("Connection Type : %u \n", ecosap_config_struct.Connection_Type);
		NET_DEBUG("IP 	  	: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.IPAddr ) ;
		NET_DEBUG("Mask 		: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Mask ) ;
		NET_DEBUG("Gateway 	: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Gateway ) ;
		NET_DEBUG("Broadcast 	: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Broadcast ) ;
		NET_DEBUG("DNS server	: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.DNS ) ;
		NET_DEBUG("Alternate DNS server : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.ADNS ) ;
		NET_DEBUG("Server 	: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Server ) ;
	}
	struct in_addr addr1, addr2, addr3 ;

	addr1.s_addr = ecosap_config_struct.bridgeConf.inaddr_ip ;
	addr2.s_addr = ecosap_config_struct.bridgeConf.inaddr_broad ;
	addr3.s_addr = ecosap_config_struct.bridgeConf.inaddr_mas ;

	NET_DEBUG("LAN configuration\n") ;
	NET_DEBUG("IP 	 	: %s \n", inet_ntoa(addr1)) ;
	NET_DEBUG("Broadcast 	: %s \n", inet_ntoa(addr2)) ;
	NET_DEBUG("Mask 		: %s \n", inet_ntoa(addr3)) ;

#ifdef CYGPKG_COMPONENT_DHCPS
	NET_DEBUG("DHCP Server configuration\n") ;
	NET_DEBUG("DHCP server 	: %s\n", ecosap_new_nw_config.dhcp_server_config.dhcps_enable ? "Enabled": "Disabled") ;
	NET_DEBUG("From Address 	: %s\n", ecosap_config_struct.dhcp_server_config.DHCP_From);
	NET_DEBUG("To Address 	: %s\n", ecosap_config_struct.dhcp_server_config.DHCP_To);
	NET_DEBUG("Lease Time 	: %lu Sec\n", ecosap_config_struct.dhcp_server_config.Lease_Time) ;
#endif //DHCPS


#ifdef CYGPKG_COMPONENT_IPTV
	NET_DEBUG("VLAN configuration\n") ;
	NET_DEBUG("VLAN :%u\n", ecosap_config_struct.vlan_config.vlan_flag);
	NET_DEBUG("IPTV VLAN ID 	: %u\n", ecosap_config_struct.vlan_config.iptv_value);
	NET_DEBUG("Internet VLAN ID 	: %u\n", ecosap_config_struct.vlan_config.inet_value);
	NET_DEBUG("VLAN inetrfaces\n");
	NET_DEBUG("VLAN1:%s\n", VLAN1);
	NET_DEBUG("VLAN2:%s\n", VLAN2);
	NET_DEBUG("VLAN3:%s\n", VLAN3);
	NET_DEBUG("VLAN4:%s\n", VLAN4);
#endif
}
#define MAC_ARR_SIZE 24
void default_flash_rule(void)
{
	int i = 0;
	char http_rule1[][24]={"ipfw","add","140","allow","tcp","from","any","to","any","80"};//10
	char http_rule2[][24]={"ipfw","add","150","allow","tcp","from","any","80","to","any"};//10

	char dns_rule1[][24]={"ipfw","add","160","allow","udp","from","any","to","any","53"};//10
	char dns_rule2[][24]={"ipfw","add","170","allow","udp","from","any","53","to","any"};//10

	char sntp_rule1[][24]={"ipfw","add","180","allow","udp","from","any","to","any","123"};//10

	for ( i = 0 ; i < 5 ; i++){
		memset((ECOSAP_CONFIG_STRUCT *)&ecosap_config_struct.ipfw_config[i], '\0', sizeof(IPFW_CONFIG_STRUCT)) ;
		ecosap_config_struct.ipfw_config[i].ipfw_argc = 10;
		ecosap_config_struct.ipfw_config[i].is_deleted = 1;

		memset((ECOSAP_CONFIG_STRUCT *)&ecosap_new_nw_config.ipfw_config[i], '\0', sizeof(IPFW_CONFIG_STRUCT)) ;
		ecosap_new_nw_config.ipfw_config[i].ipfw_argc = 10;
		ecosap_new_nw_config.ipfw_config[i].is_deleted = 1;
	}

	memcpy(ecosap_config_struct.ipfw_config[0].ipfw_rules, http_rule1, sizeof(http_rule1)) ;
	memcpy(ecosap_config_struct.ipfw_config[1].ipfw_rules, http_rule2, sizeof(http_rule2)) ;
	memcpy(ecosap_config_struct.ipfw_config[2].ipfw_rules, dns_rule1, sizeof(dns_rule1)) ;
	memcpy(ecosap_config_struct.ipfw_config[3].ipfw_rules, dns_rule2, sizeof(dns_rule2)) ;
	memcpy(ecosap_config_struct.ipfw_config[4].ipfw_rules, sntp_rule1, sizeof(sntp_rule1)) ;

	memcpy(ecosap_new_nw_config.ipfw_config[0].ipfw_rules, http_rule1, sizeof(http_rule1)) ;
        memcpy(ecosap_new_nw_config.ipfw_config[1].ipfw_rules, http_rule2, sizeof(http_rule2)) ;
        memcpy(ecosap_new_nw_config.ipfw_config[2].ipfw_rules, dns_rule1, sizeof(dns_rule1)) ;
        memcpy(ecosap_new_nw_config.ipfw_config[3].ipfw_rules, dns_rule2, sizeof(dns_rule2)) ;
        memcpy(ecosap_new_nw_config.ipfw_config[4].ipfw_rules, sntp_rule1, sizeof(sntp_rule1)) ;
}


/** Function to set default network configuration values **/
void ecosap_nw_config_default(void)
{
	char mac_address[MAC_ARR_SIZE] =
			{ 0x00, 0x03, 0x7F, 0xFF, 0xFF, 0xFF,
			  0x00, 0x03, 0x7F, 0xFF, 0xFF, 0xFE,
			  0x8C, 0xFD, 0xF0, 0xAA, 0xBB, 0xCC }; //default mac addressses for eth0, eth1 and wifi0

	memset((ECOSAP_CONFIG_STRUCT *)&ecosap_config_struct, '\0', sizeof(ecosap_config_struct));
	memcpy(ecosap_config_struct.mac_addr, mac_address, MAC_ARR_SIZE);
	ecosap_config_struct.Magic_Number = CONFIG_MAGIC_NUM;


#if defined(CYGHWR_NET_DRIVER_ETH0_ADDRS_IP) 			//Check if configuration has changed from kernelconfig
	NET_INFO(" Default connection type: STATIC_IP\n");
	ecosap_config_struct.Connection_Type = STATIC_IP ;
#else
	NET_INFO("Default connection type: DHCP\n");
	ecosap_config_struct.Connection_Type = DHCP ;
#endif

	ecosap_config_struct.wan_status = 0 ;
	ecosap_config_struct.lan_status = 0 ;
	//ecosap_config_struct.index = 0 ;
	ecosap_config_struct.is_flashrule_full = 0 ;

	ecosap_config_struct.vlan_config.vlan_flag = 0;
	ecosap_config_struct.vlan_config.is_tagged = 0;
	ecosap_config_struct.vlan_config.iptv_value = VLAN_VALUE_MIN;
	ecosap_config_struct.vlan_config.inet_value = 1;	//Allow untagged packets to come on internet port

	ecosap_config_struct.VPNPT_config_struct.is_pptp_enable = 1;
	ecosap_config_struct.VPNPT_config_struct.is_l2tp_enable = 1;
	ecosap_config_struct.VPNPT_config_struct.is_ipsec_enable = 1;


#if defined(CYGHWR_NET_DRIVER_ETH0_ADDRS_IP) 			//Check if configuration has changed from kernelconfig
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.IPAddr, string(CYGHWR_NET_DRIVER_ETH0_ADDRS_IP), IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.Mask, string(CYGHWR_NET_DRIVER_ETH0_ADDRS_NETMASK), IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.Gateway, string(CYGHWR_NET_DRIVER_ETH0_ADDRS_GATEWAY), IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.DNS, "8.8.8.8", IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.ADNS, "8.8.4.4", IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.Broadcast, string(CYGHWR_NET_DRIVER_ETH0_ADDRS_BROADCAST), IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.Server, string(CYGHWR_NET_DRIVER_ETH0_ADDRS_SERVER), IPADDR_SIZE) ;
	NET_DEBUG("IP :%s, MASK:%s, GATEWAY:%s, BROAD:%s\n", ecosap_config_struct.wan_conf.WAN_config_addrs.IPAddr,ecosap_config_struct.wan_conf.WAN_config_addrs.Mask,ecosap_config_struct.wan_conf.WAN_config_addrs.Gateway,ecosap_config_struct.wan_conf.WAN_config_addrs.Broadcast);
#else
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.IPAddr, "172.16.7.50", IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.Mask, "255.255.255.0", IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.Gateway, "172.16.7.1", IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.DNS, "172.16.2.31", IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.ADNS, "8.8.4.4", IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.Broadcast, "172.16.7.255", IPADDR_SIZE) ;
	strlcpy(ecosap_config_struct.wan_conf.WAN_config_addrs.Server, "172.16.7.166", IPADDR_SIZE) ;
#endif //ETH0_ADDRS_IP

	strlcpy(ecosap_config_struct.wan_conf.PPPoE_config_values.Username, "username", sizeof(ecosap_config_struct.wan_conf.PPPoE_config_values.Username)) ;
	strlcpy(ecosap_config_struct.wan_conf.PPPoE_config_values.Passwd, "password", sizeof(ecosap_config_struct.wan_conf.PPPoE_config_values.Passwd)) ;
	ecosap_config_struct.wan_conf.PPPoE_config_values.MTU = PPPOE_MTU_MAX ;
	strlcpy(ecosap_config_struct.wan_conf.PPPoE_config_values.Service_Name, "Service_Name", sizeof(ecosap_config_struct.wan_conf.PPPoE_config_values.Service_Name)) ;
	strlcpy(ecosap_config_struct.wan_conf.PPPoE_config_values.Server_Name, "Server_Name", sizeof(ecosap_config_struct.wan_conf.PPPoE_config_values.Server_Name)) ;
	ecosap_config_struct.wan_conf.PPPoE_config_values.MAX_Idle_Time = (24 * 60 * 60) ;

	ecosap_config_struct.dhcp_server_config.dhcps_enable = 1; //default start dhcps 
	strlcpy(ecosap_config_struct.dhcp_server_config.DHCP_From, DHCPS_POOL_START_ADDR_IP, IPADDR_SIZE);
	strlcpy(ecosap_config_struct.dhcp_server_config.DHCP_To, DHCPS_POOL_END_ADDR_IP, IPADDR_SIZE);
	ecosap_config_struct.dhcp_server_config.Lease_Time = DHCPS_LEASE_TIME ;

	ecosap_config_struct.bridgeConf.inaddr_ip = htonl(inet_addr("192.168.0.1"));
	ecosap_config_struct.bridgeConf.inaddr_mas = htonl(inet_addr("255.255.255.0"));
	ecosap_config_struct.bridgeConf.inaddr_broad = htonl(inet_addr("192.168.0.255"));

	strlcpy(ecosap_config_struct.br_info_struct.br_name, "br0", sizeof(ecosap_config_struct.br_info_struct.br_name));
	ecosap_config_struct.br_info_struct.br_intno = 1;
	strlcpy(ecosap_config_struct.br_info_struct.br_interfaces[0], "eth1", ecosap_config_struct.br_info_struct.br_interfaces[0]);

	strlcpy(ecosap_config_struct.dmz_config.host, DEFAULT_DMZ_IP, IPADDR_SIZE);
	ecosap_config_struct.dmz_config.status = 0;
	ecosap_config_struct.igmp_config.status = 0;

	/* WLAN config default settings */
	memset((struct WLAN_Default_Config *)&ecosap_config_struct.WLAN_config,
			0, sizeof(ecosap_config_struct.WLAN_config));
	ecosap_config_struct.WLAN_config.magic_num = CONFIG_WLAN_MAGIC_NUM;
	strcpy(ecosap_config_struct.WLAN_config.mode, "ap");
	ecosap_config_struct.WLAN_config.ap_config.if_idx = 0;
	strcpy(ecosap_config_struct.WLAN_config.ap_config.ssid, "hello_tenda");
	ecosap_config_struct.WLAN_config.ap_config.channel = 6;
	ecosap_config_struct.WLAN_config.ap_config.bw = 40;
	strcpy(ecosap_config_struct.WLAN_config.ap_config.wlan_mode, "11NG");
	strcpy(ecosap_config_struct.WLAN_config.ap_config.security, "open");
	strcpy(ecosap_config_struct.WLAN_config.ap_config.passphrase, "1234567890");
	ecosap_config_struct.WLAN_config.ap_config.beacon_int = 100;
	ecosap_config_struct.WLAN_config.ap_config.wds = 0;

	ecosap_config_struct.WLAN_config.sta_config.if_idx = 1;
	strcpy(ecosap_config_struct.WLAN_config.sta_config.ssid, "hello_tenda");
	ecosap_config_struct.WLAN_config.sta_config.wds = 0;
	strcpy(ecosap_config_struct.WLAN_config.sta_config.security, "open");

    strcpy(ecosap_config_struct.WLAN_config.vlan_iptv_ap_config.ssid, "hello_tenda_iptv");
    ecosap_config_struct.WLAN_config.vlan_iptv_ap_config.channel = 6;
    strcpy(ecosap_config_struct.WLAN_config.vlan_iptv_ap_config.security, "open");
    strcpy(ecosap_config_struct.WLAN_config.vlan_iptv_ap_config.passphrase, "1234567890");
    ecosap_config_struct.WLAN_config.vlan_iptv_ap_config.beacon_int = 100;

	default_flash_rule();

	int n = cyg_flash_setconf((ECOSAP_CONFIG_STRUCT *) &ecosap_config_struct, sizeof(ECOSAP_CONFIG_STRUCT), 0);
	NET_INFO("Written %d Bytes in Flash firsttime\n", n);
}

/** Function to show network configuration addressess **/
void show_nw_config (  )
{
	int i,j;

	NET_INFO("WAN configuration\n") ;
	if(ecosap_config_struct.Connection_Type == PPPoE)
	{
#ifdef CYGPKG_COMPONENT_PPPoE
        NET_INFO("Connection Type: %s \n", "PPPoE") ;
        NET_INFO("Username	    : %s \n", ecosap_config_struct.wan_conf.PPPoE_config_values.Username) ;
        NET_INFO("Password	    : %s \n", ecosap_config_struct.wan_conf.PPPoE_config_values.Passwd) ;
        NET_INFO("MTU		    : %u \n", ecosap_config_struct.wan_conf.PPPoE_config_values.MTU) ;
        NET_INFO("IP 	  	    : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.IPAddr ) ;
        NET_INFO ("Mask 		    : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Mask ) ;
        NET_INFO ("Gateway 	    : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Gateway ) ;
        NET_INFO ("Broadcast 	    : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Broadcast ) ;
        NET_INFO ("DNS server	    : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.DNS ) ;
        NET_INFO ("Alternate DNS server : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.ADNS ) ;
        NET_INFO ("Server         : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Server ) ;
#endif //PPPoE
	}
	else{
		NET_INFO ("Connection Type : %s \n", (((ecosap_config_struct.Connection_Type)==2) ? "Static" : "DHCP"));
		NET_INFO ("Status	         : %s \n", (((ecosap_config_struct.wan_status) == 1) ? "UP" : "DOWN" ));
		NET_INFO ("IP 	  	 : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.IPAddr ) ;
		NET_INFO ("Mask 		 : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Mask ) ;
		NET_INFO ("Gateway 	 : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Gateway ) ;
		NET_INFO ("DNS server	 : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.DNS ) ;
		NET_INFO ("Alternate DNS server : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.ADNS ) ;
		NET_INFO ("Server          : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Server ) ;
	}
	struct in_addr addr1, addr2, addr3 ;

	addr1.s_addr = ecosap_config_struct.bridgeConf.inaddr_ip ;
	addr2.s_addr = ecosap_config_struct.bridgeConf.inaddr_broad ;
	addr3.s_addr = ecosap_config_struct.bridgeConf.inaddr_mas ;

	NET_INFO ("LAN configuration\n") ;
	NET_INFO("IP Address 	: %s \n", inet_ntoa(addr1)) ;
	NET_INFO("Broadcast 	: %s \n", inet_ntoa(addr2)) ;
	NET_INFO("Mask 		: %s \n", inet_ntoa(addr3)) ;

#ifdef CYGPKG_COMPONENT_DHCPS
	NET_INFO ("DHCP Server configuration\n") ;
	NET_INFO("DHCP Server	:	%s\n",(ecosap_config_struct.dhcp_server_config.dhcps_enable ? "Enabled":"Disabled"));
	NET_INFO("From Address 	: %s\n", ecosap_config_struct.dhcp_server_config.DHCP_From);
	NET_INFO("To Address 	: %s\n", ecosap_config_struct.dhcp_server_config.DHCP_To);
	NET_INFO("Lease Time 	: %lu Sec\n", ecosap_config_struct.dhcp_server_config.Lease_Time) ;
#endif //DHCPS

#ifdef CYGPKG_COMPONENT_IPTV
	NET_INFO ("VLAN configuration\n") ;
	NET_INFO("VLAN status			:	%s\n",(ecosap_config_struct.vlan_config.vlan_flag ? "Enable":"Disable"));
	if(ecosap_config_struct.vlan_config.vlan_flag == 1){
		NET_INFO("VLAN internet value	:	%u\n",ecosap_config_struct.vlan_config.inet_value);
		NET_INFO("VLAN iptv value		:	%u\n",ecosap_config_struct.vlan_config.iptv_value);
		NET_INFO("VLAN tag value		:	%u\n",ecosap_config_struct.vlan_config.is_tagged);
	}
#endif

#ifdef CYGPKG_COMPONENT_BRIDGE
	NET_INFO("Bridge Information\n");
	for (i = 0; i < 1; i++) {
		NET_INFO("Bridge Name : %s\n", ecosap_config_struct.br_info_struct.br_name);
		NET_INFO("No.of Br if : %d\n", ecosap_config_struct.br_info_struct.br_intno);
		NET_INFO("Interfaces  : \n");

		for (j = 0; j < 6; j++)
			if(strcmp((ecosap_config_struct.br_info_struct.br_interfaces[j]), "\0"))
				NET_INFO(" %s\n" ,(ecosap_config_struct.br_info_struct.br_interfaces[j]));
	}
#endif//BRIDGE

#ifdef CYGPKG_COMPONENT_IPFW
	NET_INFO ("Firewall & NAT Rules\n") ;
	for(i=0; i<5; i++){
		NET_INFO("%u  ", ecosap_config_struct.ipfw_config[i].is_deleted);
		for(j=0; j<16; j++){
			NET_INFO("%s ", ecosap_config_struct.ipfw_config[i].ipfw_rules[j]);
		}
		NET_INFO (" %u", ecosap_config_struct.ipfw_config[i].ipfw_argc);
		NET_INFO("\n") ;
	}

	NET_INFO ("VPN Pass through configuration\n") ;
	NET_INFO("VPN pass through for PPTP   : %s \n", ((ecosap_config_struct.VPNPT_config_struct.is_pptp_enable) ? "Enable" : "Disable"));
	NET_INFO("VPN pass through for L2TP   : %s \n", ((ecosap_config_struct.VPNPT_config_struct.is_l2tp_enable) ? "Enable" : "Disable"));
	NET_INFO("VPN pass through for IPSEC  : %s \n", ((ecosap_config_struct.VPNPT_config_struct.is_ipsec_enable) ? "Enable" : "Disable"));
#endif //IPFW

#ifdef CYGPKG_NET_DMZ
	NET_INFO ("DMZ Configuration\n") ;
	NET_INFO("DMZ Host	: %s\n", ecosap_new_nw_config.dmz_config.host);
	NET_INFO("DMZ Status	: %u\n", ecosap_new_nw_config.dmz_config.status);
#endif

#if defined(CYGPKG_COMPONENT_IGMPPROXY) && defined(CYGPKG_COMPONENT_IGMPSNOOP)
    NET_INFO("IGMP feature: %s\n",
             (ecosap_config_struct.igmp_config.status & IGMP_ENABLE) ?
             "Enabled" : "Disabled");
#endif
}
/** set delete flag for all ipfw rule in flash  **/
void delete_rule_from_flash(char *str){
	int k, flag=0;
	for(k = 0; k < FLASH_IPFW_INDEX_SIZE; k++){
		if(strcmp(ecosap_config_struct.ipfw_config[k].ipfw_rules[1], "add") == 0){
			if(strcmp(ecosap_config_struct.ipfw_config[k].ipfw_rules[2], str) == 0){
				if(ecosap_config_struct.ipfw_config[k].is_deleted == 0) {
				ecosap_config_struct.ipfw_config[k].is_deleted = 1;
				ecosap_new_nw_config.ipfw_config[k].is_deleted = 1;
				flag = 1;
				}
			}
		}
	}
	/*This variable is to check if rule about to be deleted is in flash or not*/
	is_flash_rule = flag;
	ecosap_config_struct.is_flashrule_full = (unsigned int)flag;
	ecosap_new_nw_config.is_flashrule_full = (unsigned int)flag;
}
/** set delete flag for all NAT in flash  **/
void delete_nat_from_flash(char *str){
	int k, flag=0;
	for(k = 0; k < FLASH_IPFW_INDEX_SIZE; k++){
		if(strcmp(ecosap_config_struct.ipfw_config[k].ipfw_rules[1], "nat") == 0){
			if(strcmp(ecosap_config_struct.ipfw_config[k].ipfw_rules[2], str) == 0){
				if(ecosap_config_struct.ipfw_config[k].is_deleted == 0) {
				ecosap_config_struct.ipfw_config[k].is_deleted = 1;
				ecosap_new_nw_config.ipfw_config[k].is_deleted = 1;
				flag = 1;
				}
			}
		}
	}
	is_flash_rule = flag;
	ecosap_config_struct.is_flashrule_full = (unsigned int)flag;
	ecosap_new_nw_config.is_flashrule_full = (unsigned int)flag;
}
/** return 1 if string contain only digits, else return 0 **/
int isvalid_digit(char *str)
{
	while (*str) {
		if (*str >= '0' && *str <= '9')
			++str;
		else
			return 0;
	}
	return 1;
}

/** Function to find out the Class **/
char findClass(char *str)
{
	char temp[4] ;/** To store the first octet **/
	int i = 0;
	int ip = 0 ;

	while (str[i] != '.') {
		temp[i] = str[i];
		i++;
	}

	ip = strtoul(temp, NULL, 10);
	if ((ip == ULONG_MAX) && (errno == ERANGE))
		return 'X';

	// Class A
	if (ip >=1 && ip <= 126) {
		return 'A';

		// Class B
	} else if (ip >= 128 && ip <= 191) {
		return 'B';

		// Class C
	} else if (ip >= 192 && ip <= 223) {
		return 'C';

		// Class D
	} else if (ip >= 224 && ip <= 239) {
		return 'D';

		// Class E
	} else {
		return 'E';
	}
}

/** Function to separate Network ID as well as Host ID  **/
void separate(char * ip_addr, char ipClass, NW_HOST_ADDR *var)
{
	/* Initializing network and host array to NULL*/
	int k ;

	for ( k = 0; k < 12; k++)
		var->network[k] = var->host[k] = '\0';

	/* for class A, only first octet is Network ID
	   and rest are Host ID */
	if (ipClass == 'A')
	{
		int i = 0, j = 0;
		while (ip_addr[j] != '.')
			var->network[i++] = ip_addr[j++];
		i = 0;
		j++;
		while (ip_addr[j] != '\0')
			var->host[i++] = ip_addr[j++];
	}

	/* for class B, first two octet are Network ID
	   and rest are Host ID */
	else if (ipClass == 'B')
	{
		int i = 0, j = 0, dotCount = 0;

		/* storing in network[] up to 2nd dot
		   dotCount keeps track of number of
		   dots or octets passed */
		while (dotCount < 2)
		{
			var->network[i++] = ip_addr[j++];
			if (ip_addr[j] == '.')
				dotCount++;
		}
		i = 0;
		j++;

		while (ip_addr[j] != '\0')
			var->host[i++] = ip_addr[j++];


	}

	/* for class C, first three octet are Network ID
	   and rest are Host ID*/
	else if (ipClass == 'C')
	{
		int i = 0, j = 0, dotCount = 0;

		/* storing in network[] up to 3rd dot
		   dotCount keeps track of number of
		   dots or octets passed */
		while (dotCount < 3)
		{
			var->network[i++] = ip_addr[j++];
			if (ip_addr[j] == '.')
				dotCount++;
		}

		i = 0;
		j++;

		while (ip_addr[j] != '\0')
			var->host[i++] = ip_addr[j++];


	} else {
		NET_INFO ("The IPv4 Addressess belong to class D or class E\n") ;
	}

}

/** function to remove dots in address**/
unsigned long remove_dots(char *str)
{

	int i ;

	for ( i = 0 ; i < strlen(str) ; i++ ) {
		if ( str[i] == '.' ) {
			str[i] = '0' ;
		}
	}
	return atoi(str) ;
}

/** Function to validate range of ipaddressess **/
int isvalidrange(char * ip_str1, char * ip_str2)
{
	unsigned long int host1 ; /* to store host1 address */
	unsigned long int host2 ; /* to store host2 address */
	NW_HOST_ADDR var1, var2 ;

	char ipClass1 = findClass(ip_str1);
	char ipClass2 = findClass(ip_str2);

	if ( ipClass1 == ipClass2) { /* Two IPv4 Addressess are of same class **/
		separate(ip_str1, ipClass1, &var1);
		separate(ip_str2, ipClass1, &var2);


		if (ipClass1 == 'C') {
			if ( (!strcmp (var1.network, var2.network)) && (atoi(var1.host) <= atoi(var2.host)) ) {
				return 1 ;/*valid range */
			} else {
				return 0 ; /* Not in valid range */
			}
		} else if ( ipClass1 == 'A' ) {
			host1 = remove_dots(var1.host) ;
			host2 = remove_dots(var2.host) ;

			if ( (!strcmp (var1.network, var2.network)) && (host1 <= host2 )) {
				return 1 ;
			} else {
				return 0 ;
			}
		} else if (  ipClass1 == 'B' ) {
			host1 = remove_dots(var1.host) ;
			host2 = remove_dots(var2.host) ;

			if ( (!strcmp (var1.network, var2.network)) && (host1 <= host2) ) {
				return 1 ;
			} else {
				return 0 ;
			}
		} else {
			return 0;
		}


	} else {

		return 0;
	}
	return 0 ;
}

int isvalid_mask(char *netmask)
{
	struct in_addr ip;
	unsigned int mask;
	if (inet_aton(netmask, &ip) == 0) {
		NET_ERROR("Invalid address\n");
		return 0;
	}
	mask = ip.s_addr;
	if (mask == 0 || mask == 0xFFFFFFFF) 
		return 0;
	if (mask & (~mask >> 1))
		return 0;
	else 
		return 1;
}

int gen_broadcast_addr(char *host_ip, char *netmask, char *bcast)
{
	struct in_addr host, mask, broadcast;
	if (inet_aton(host_ip, &host) != 0 &&
			inet_aton(netmask, &mask) != 0)
		broadcast.s_addr = host.s_addr | ~mask.s_addr;
	else {
		return 1;
	}
	strlcpy(bcast, (const char *)inet_ntoa(broadcast), IPADDR_SIZE);
	return 0;
}

int set_lanmask(char class) 
{ 
	switch(class) {
		case 'A' : 
			strlcpy(LAN_config_addrs.Mask, "255.0.0.0", IPADDR_SIZE);
			break;
		case 'B' : 
			strlcpy(LAN_config_addrs.Mask, "255.255.0.0", IPADDR_SIZE);
			break;
		case 'C' : 
			strlcpy(LAN_config_addrs.Mask, "255.255.255.0", IPADDR_SIZE);
			break;
		default :
			return -1;
	}
	return 0;
}

/** Function to Validate IPv4 addressess **/

bool isvalidip(const char * addr)
{
	char temp[8];
	int digit = 0;
	int addr_arr[8];
	int num = 0;
	int i ;/* For looping purpose */
	int dots = 0;

	for (i = 0; i < (int) strlen(addr); i++) {
		char c = addr[i];
		int cgood = 0;

		/* checking whether valid digit or not */
		if (c >= '0' && c <= '9' && digit < 4) {
			temp[digit++] = c;
			temp[digit] = 0;
			cgood++;
			continue;
		}
		/* return false if more than 4 digits */
		if (digit == 4) {
			return false;
		}
		if ((c == '.') && (addr[i + 1] != 0) ) {
			if ( !digit ) { /* if 1st character is '.' then return false */
				return false;
			}
			addr_arr[num++] = atoi(temp);
			digit = 0;
			cgood++;
			dots++ ;
			continue;
		}


		if (!cgood) {
			return false; // not a valid character
		}
	}
	addr_arr[num++] = atoi(temp); // we did not have a dot, we had a NULL.....

	if (num != 4) {
		return false; // we must have had 4 valid numbers....
	}
	if ( dots > 3 ) {
		return false ;
	}

	for ( i = 0; i < 4; i++) {
		if (addr_arr[i] < 0 || addr_arr[i] > 255) return 0; // octet values out-of-range
	}

	if ( !strcmp(addr, "0.0.0.0") || !strcmp(addr, "255.255.255.255") ) {
		return false ;
	}

	if ( (addr_arr[0] == 0) || (addr_arr[0] == 127) || (addr_arr[3] == 255) || (addr_arr[3] == 0)) {
		return false ;
	}

	return true; //Valid IPv4 addresses
}

bool isvalid_subnetip(const char * addr)
{
	char temp[8];
	int digit = 0;
	unsigned long int addr_arr[8];
	int num = 0;
	int i ;/* For looping purpose */
	int dots = 0;
	unsigned long int ret = 0;

	for (i = 0; i < (int) strlen(addr); i++) {
		char c = addr[i];
		int cgood = 0;

		/* checking whether valid digit or not */
		if (c >= '0' && c <= '9' && digit < 4) {
			temp[digit++] = c;
			temp[digit] = 0;
			cgood++;
			continue;
		}
		/* return false if more than 4 digits */
		if (digit == 4) {
			return false;
		}
		if ((c == '.') && (addr[i + 1] != 0) ) {
			if ( !digit ) { /* if 1st character is '.' then return false */
				return false;
			}
			ret = strtoul(temp, NULL, 10); 
			if ((ret == ULONG_MAX) && (errno == ERANGE)) 
				return false;
			addr_arr[num++] = ret; // we did not have a dot, we had a NULL.....
			digit = 0;
			cgood++;
			dots++ ;
			continue;
		}


		if (!cgood) {
			return false; // not a valid character
		}
	}
	ret = strtoul(temp, NULL, 10);
	if ((ret == ULONG_MAX) && (errno == ERANGE)) 
		return false;
	addr_arr[num++] = ret; // we did not have a dot, we had a NULL.....

	if (num != 4) {
		return false; // we must have had 4 valid numbers....
	}
	if ( dots > 3 ) {
		return false ;
	}

	for ( i = 0; i < 4; i++) {
		if (addr_arr[i] < 0 || addr_arr[i] > 255) return 0; // octet values out-of-range
	}

	if ( !strcmp(addr, "0.0.0.0") || !strcmp(addr, "255.255.255.255") ) {
		return false ;
	}

	if ( (addr_arr[0] == 0) || (addr_arr[3] == 255) ) {
		return false ;
	}

	return true; //Valid IPv4 addresses
}

/* Function to check if WAN and LAN IP are in same subnet */
bool issamesubnet(const char * wan_ip, const char * lan_ip) {
	int result = 0;
	int i = 0;
	char temp[32] = {0};
	int dot_count = 0;
	result = isvalidip(wan_ip);
	strlcpy(temp, lan_ip, sizeof(temp));

	if (result) {
		while(temp[i]){
			if(temp[i] == '.'){
				dot_count ++;
				if(dot_count == 3){
					temp[i] = '\0';
					break;
				}
			}
			i++;
		}
		if (strstr(wan_ip, temp)) {
			return 1;
		}

	} else {
		NET_INFO(" Invalid ip received ");
	}
		return 0;
}

/* Function to change subnet of lan ip*/
u_int32_t change_octal(u_int32_t ip){
	u_int32_t ori_lan_ip = ip;
	//ori_lan_ip = (ori_lan_ip & 0xFFFFFF00) + 1;
	u_int32_t shi_lan_ip1 = ((ori_lan_ip & 0x0000FF00) >> 8);
	if(shi_lan_ip1 == 0xFF || shi_lan_ip1 == 0xFE)
		shi_lan_ip1 = ((ori_lan_ip & 0x00FF0000) >> 16);
	else {
		shi_lan_ip1 += 1;
		shi_lan_ip1 = shi_lan_ip1 << 8;
		u_int32_t temp = ori_lan_ip & 0x000000FF;
		if(temp == 0xFF || temp == 0xFE){
			ori_lan_ip = (ori_lan_ip & 0xFFFF0000) + shi_lan_ip1;
		} else {
			ori_lan_ip = (ori_lan_ip & 0xFFFF00FF) + shi_lan_ip1;
		}
		return ori_lan_ip;
	}

	if(shi_lan_ip1 == 0xFF || shi_lan_ip1 == 0xFE){
		shi_lan_ip1 = ((ori_lan_ip & 0xFF000000) >> 24);
	} else {
		shi_lan_ip1 += 1;
		shi_lan_ip1 = shi_lan_ip1 << 16;
		u_int32_t temp = (ori_lan_ip & 0x0000FF00) >> 8;
		if(temp == 0xFF || temp == 0xFE){
			ori_lan_ip = ((ori_lan_ip & 0xFF0000FF) + shi_lan_ip1) ;
		} else {
			ori_lan_ip = (ori_lan_ip & 0xFF00FFFF) + shi_lan_ip1;
		}

		return ori_lan_ip;
	}

	if(shi_lan_ip1 == 0xFF || shi_lan_ip1 == 0xFE){
		return 0;
	} else {
		shi_lan_ip1 += 1;
		shi_lan_ip1 = shi_lan_ip1 << 24;
		u_int32_t temp = (ori_lan_ip & 0x00FF0000) >> 16;
		if(temp == 0xFF || temp == 0xFE){
			ori_lan_ip = ((ori_lan_ip & 0x0000FFFF) + shi_lan_ip1) ;
		} else {
			ori_lan_ip = (ori_lan_ip & 0xFF00FFFF) + shi_lan_ip1;
		}
		u_int32_t temp1 = (ori_lan_ip & 0x0000FF00) >> 8;

		if(temp1 == 0xFF || temp1 == 0xFE ){
			ori_lan_ip = ori_lan_ip & 0xFFFF00FF  ;
		}

		return ori_lan_ip;
	}

	return ori_lan_ip;
}

/* Function to change lan subnet and dhcp address pool */
void change_lan_subnet(void) 
{
	u_int32_t ori_lan_ip;

	pthread_mutex_lock(&write_nw_mutex);

	ecosap_new_nw_config.bridgeConf.inaddr_ip = change_octal(ecosap_new_nw_config.bridgeConf.inaddr_ip);

	ori_lan_ip = ecosap_new_nw_config.bridgeConf.inaddr_ip ;
	ecosap_new_nw_config.bridgeConf.inaddr_broad = (ori_lan_ip & 0xFFFFFF00) | (0x000000FF);

	change_dhcp_pool();
	pthread_mutex_unlock(&write_nw_mutex);
	NET_CRITICAL("IP Address Conflict Detected!!!!!!\n Changing Subnet of LAN !!! Login webpage with new LAN IP\n");
	NET_CRITICAL("Rebooting System !!!!!!!\n");
	write_nw_flash();
	watchdog_reboot();
}

void replace(char *ori_str, char *old_str, char *new_str)
{
	char *pos = strstr(ori_str, old_str);
	if (pos != NULL) {
		int newlen = strlen(ori_str) - strlen(old_str) + strlen(new_str);
		if(newlen < MAX_IPFW_RULE_ARGS_SIZE){
			char new_sentence[newlen + 1];
			memset(new_sentence, 0, newlen + 1);
			memcpy(new_sentence, ori_str, pos - ori_str);
			memcpy(new_sentence + (pos - ori_str), new_str, strlen(new_str));
			strlcpy(new_sentence + (pos - ori_str) + strlen(new_str), pos + strlen(old_str), sizeof(new_sentence));
			memset(ori_str, 0, newlen + 1);
			memcpy(ori_str, new_sentence, strlen(new_sentence));
		}
	}
}

int isvalid_addr (char *addr)
{
	char *token = NULL ;
	char buffer[MAX_IPFW_RULE_ARGS_SIZE] ;
	memset(buffer, '\0', MAX_IPFW_RULE_ARGS_SIZE) ;

	replace(addr, "%2F",  "/") ;
	strlcpy(buffer, addr, sizeof(buffer)) ;
	token = strtok(buffer, "/");
	if( token == NULL ) {
		return 0;
	}
	if ( !isvalid_subnetip(token) ) {
		return 0 ;
	}
	token = strtok(NULL, "/") ;
	if ( (token != NULL)  && isvalid_digit(token) ) {
		if ( atoi(token) < 32 ) {
			return 1 ;
		} else {
			return 0 ;
		}
	} else {
		return 0 ;
	}
	return 1 ;
}

#ifdef CYGPKG_COMPONENT_IPFW
int isvalid_cmd(char (*cmd)[MAX_IPFW_RULE_ARGS_SIZE], FILE *client)
{
	int i = 0 ;
	int k = 0 ;
	int FLAG = 0 ;
	int FLAG1 = 0 ;
	int FLAG2 = 0 ;


	if ( !strcmp(cmd[0], "ipfw") ) {

		if ( !strcmp(cmd[1], "add") ) {
			if ( !isvalid_digit(cmd[2]) ) {
				i = 2 ;
			} else {
				i = 3 ;
			}
			if(strcmp(cmd[i], "check-state")==0 ){
				if(strlen(cmd[i+1]) == 0){
					return 1;
				} else{
					return 0;
				}
			}
			for ( k = 0 ; k < ACTION_COUNT ; k++ ) {
				if ( !strcmp(cmd[i], rule_actions[k].s ) ) {
					FLAG = 1 ;
					break ;
				} else {
					continue ;
				}
			}


			if ( FLAG == 1 ) {
				i++ ;

				if ( !strcmp(cmd[i - 1], "nat" ) ) {
					if ( isvalid_digit(cmd[i]) ) {
						i++ ;
					} else {
						fprintf(client, "<br>ipfw: missing nat id") ;
						return 0 ;
					}
				}

				for ( k = 0 ; k < PROTO_COUNT ; k++ ) {
					if ( !strcmp(cmd[i], protocols[k].p_name ) ) {
						FLAG1 = 1 ;
						break ;
					} else {
						continue ;
					}
				}

				if ( FLAG1 == 1 ) {
					i++ ;

					if ( !strcmp(cmd[i], "from") ) {

						i++ ;
						if ( isvalidip(cmd[i]) || isvalid_addr(cmd[i]) 
									|| !strcmp(cmd[i], "any") ) {

							i++ ;

							if ( isvalid_digit(cmd[i]) == 1 ) {
								i++ ;
							}

							if ( !strcmp(cmd[i], "to") ) {
								i++ ;
								if ( isvalidip(cmd[i]) || isvalid_addr(cmd[i]) 
											|| !strcmp(cmd[i], "any") ) {
									return 1 ;
								} else {
									fprintf(client, "<br>ipfw: missing destination IP") ;
									return 0 ;
								}
							} else {
								fprintf(client, "<br>ipfw: missing to") ;
								return 0 ;
							}
						} else {
							fprintf(client, "<br>ipfw: missing source IP") ;
							return 0 ;
						}

					} else {

						fprintf(client, "<br>ipfw: missing from") ;
						return 0 ;
					}

				} else {
					fprintf(client, "<br>ipfw: missing protocol") ;
					return 0 ;
				}


			} else {
				fprintf(client, "<br>ipfw: missing action") ;
				return 0 ;
			}

		} else if (!strcmp(cmd[1], "nat")) {
			if ( isvalid_digit(cmd[2]) ) {
				if (!strcmp(cmd[3], "config")) {
					for ( k = 0 ; k < PROTO_COUNT ; k++ ) {
						if ( !strcmp(cmd[4], protocols[k].p_name ) ) {
							FLAG2 = 1 ;
							break ;
						} else {
							continue ;
						}
					}

					if (FLAG2 == 1) {
						if ( isvalidip(cmd[5]) || isvalid_addr(cmd[5]) ) {
							return 1 ;
						} else {
							fprintf(client, "<br>ipfw: Invalid IP") ;
						}

                    } else if (!strcmp(cmd[4], "if")) {
                        if (!strcmp(cmd[5], WAN_INTERFACE) ||!strcmp(cmd[5], WAN_INTERFACE_PPP)  ) {
                            return 1 ;
                        } else {
                            fprintf(client, "<br>ipfw: missing interface name") ;
                        }

					} else {
						fprintf(client, "<br>ipfw: missing protocol(ip/if) ") ;
					}

				} else {
					fprintf(client, "<br>ipfw: Bad command ") ;
				}

			} else if ( !strcmp(cmd[2], "show")) {
				if (!strcmp(cmd[3], "config")) {
					return 1 ;
				} else {
					fprintf(client, "<br>ipfw: Bad command") ;
				}
			} else {
				fprintf(client, "<br>ipfw: Bad command") ;
			}

		} else {
			fprintf(client, "<br>ipfw: Bad command ") ;
			return 0 ;
		}
	} else {
		fprintf(client, "<br>Invalid Command") ;
		return 0 ;
	}
	return 0 ;
}
#endif //IPFW


int ADD_ROUTE(u_int32_t ip, u_int32_t gw, u_int32_t nm, char *intf) {
	struct sockaddr_in addr;
	struct ecos_rtentry route;
	int s;


	s = socket(AF_INET, SOCK_DGRAM, 0);
	if ( s < 0) {
		perror("[ADD_ROUTE]Socket:");
		return -1;
	} else {
		memset(&route, 0, sizeof(route));
		memset(&addr, 0, sizeof(addr));

		addr.sin_family = AF_INET;
		addr.sin_len = sizeof(addr);

		addr.sin_addr.s_addr = ip & nm;
		memcpy(&route.rt_dst, &addr, sizeof(route.rt_dst));

		addr.sin_addr.s_addr = gw & nm;
		memcpy(&route.rt_gateway, &addr, sizeof(route.rt_gateway));

		addr.sin_addr.s_addr =  nm;
		memcpy(&route.rt_genmask, &addr, sizeof(route.rt_genmask));

		route.rt_dev = (char*)intf;
		route.rt_flags = RTF_UP;
		route.rt_metric = 0;
		if ((ioctl(s, SIOCADDRT, &route)) < 0) {
			close(s);
			return -1;
		}
	}
	close(s);
	return 0;
}


/* Function to start lan interface */
void start_nw_lan_interface(void)
{
	set_nw_lan_interface();
}

int isvalidip_port(char *str)
{
	char *token1 = strtok(str, ": ");

	if (NULL == token1) {
		NET_ERROR("strtok failed\n");
		return 0;
	}

	NET_DEBUG("ip : %s\n", token1);
	if(isvalidip(token1)){
		char *token2 = strtok(NULL, ":");
		if(token2 == NULL){
			NET_ERROR("NULL port in ip\n");
			return 0;
		}
		else{
			if(isvalid_ipfw_digit(token2))
			{
				return 1;
			}
			else{
				return 0;
			}
		}
	}
	else{
		NET_ERROR("Invalid IP\n");
		return 0;
	}

}

int isvalid_ip_mask (char *addr)
{
	char *token = NULL ;
	char buffer[32] ;

	memset(buffer, '\0', 32) ;
	strlcpy(buffer, addr, sizeof(buffer));
	token = strtok(buffer, "/");

	if (NULL == token)
		return 0;

	if ( !isvalid_subnetip(token) ) {
		return 0 ;
	}
	token = strtok(NULL, "/") ;
	if ( (token != NULL)  && isvalid_digit(token) ) {
		if ( atoi(token) < 32 ) {
			return 1 ;
		} else {
			return 0 ;
		}
	} else {
		return 0 ;
	}
	return 1 ;
}

int isvalid_ipfw_digit(char *str)
{
	while (*str) {
		if (*str >= '0' && *str <= '9')
			++str;
		else
			return 0;
	}
	return 1;
}

int isvalid_ipfw_cmd(char (*cmd)[24])
{
#if FLASH_VALIDATION
	int i = 1 ;
	int k = 0 ;
	int FLAG = 0 ;
	int FLAG1 = 0 ;


	if ( !strcmp(cmd[0], "ipfw") ) {
		if ( !strcmp(cmd[1], "add") ) {
			if ( !isvalid_ipfw_digit(cmd[2]) ) {
				i = 2 ;
			} else {
				i = 3 ;
			}
			if(strcmp(cmd[i], "check-state")==0 ){
				return 1;
			}

			for ( k = 0 ; k < ACTION_COUNT ; k++ ) {
				if ( !strcmp(cmd[i], rule_actions[k].s ) ) {
					FLAG = 1 ;
					break ;
				} else {
					continue ;
				}
			}


			if ( FLAG == 1 ) {
				i++ ;

				if ( !strcmp(cmd[i - 1], "nat" ) ) {
					if ( isvalid_ipfw_digit(cmd[i]) ) {
						i++ ;
					} else {
						NET_ERROR("ipfw: missing nat id\n") ;
						return 0 ;
					}
				}

				for ( k = 0 ; k < PROTO_COUNT ; k++ ) {
					if ( !strcmp(cmd[i], protocols[k].p_name ) ) {
						FLAG1 = 1 ;
						break ;
					} else {
						continue ;
					}
				}

				if ( FLAG1 == 1 ) {
					i++ ;

					if ( !strcmp(cmd[i], "from") ) {

						i++ ;
						if ( !strcmp(cmd[i], "any") || isvalidip(cmd[i]) || isvalid_ip_mask(cmd[i])) {

							i++ ;
							if(isvalid_ipfw_digit(cmd[i]) == 1){
								i++;
							}
							if ( !strcmp(cmd[i], "to") ) {
								i++ ;
								if ( !strcmp(cmd[i], "any") || isvalidip(cmd[i]) || isvalid_ip_mask(cmd[i])) {
									return 1 ;
								} else {
									NET_ERROR("ipfw: missing destination IP\n") ;
									return 0 ;
								}
							} else {
								NET_ERROR("ipfw: missing to\n") ;
								return 0 ;
							}
						} else {
							NET_ERROR("ipfw: missing source IP\n") ;
							return 0 ;
						}

					} else {

						NET_ERROR("ipfw: missing from\n") ;
						return 0 ;
					}

				} else {
					NET_ERROR("ipfw: missing protocol\n") ;
					return 0 ;
				}


			} else {
				NET_ERROR("ipfw: missing action\n") ;
				return 0 ;
			}

		} else {
			NET_ERROR("ipfw: Bad command\n") ;
			return 0 ;
		}
	} else {
		NET_ERROR("Invalid Command\n") ;
		return 0 ;
	}
#endif
	return 0 ;
}
int is_valid_nat_cmd(char (*cmd)[24])
{
	if(isvalid_ipfw_digit(cmd[2]))
	{
		if(strcmp(cmd[3], "config") == 0){
			if(strcmp(cmd[4], "if") == 0){
				if(strcmp(cmd[5], WAN_INTERFACE_PPP) == 0 || strcmp(cmd[5], WAN_INTERFACE) == 0){
				if(strcmp(cmd[6], "redirect_port") == 0){


					if(strcmp(cmd[7], "tcp")==0 || strcmp(cmd[7], "udp")==0){

						if(isvalidip_port(cmd[8])){

							if(isvalid_ipfw_digit(cmd[9])){

								return 1;
							}
							else{
								NET_ERROR("nat: external port error\n");
								return 0 ;
							}
						}
						else{
							NET_ERROR("nat : ip-port error\n");
							return 0 ;
						}
					}
					else{
						NET_ERROR("nat : TCP/UDP error\n");
						return 0 ;
					}
				}
				else{
					NET_ERROR("nat : redirect_port error\n");
					return 0 ;
				}
				}
				else {
					NET_ERROR("nat :  invalid interface error\n");
					return 0 ;
				}
			}
			else{
				NET_ERROR("nat : if error\n");
				return 0 ;
			}
		}
		else{
			NET_ERROR("nat : config error\n");
			return 0 ;
		}
	}
	else{
		NET_WARNING("Invalid Command\n") ;
		return 0 ;
	}
	return 0;
}

int is_valid_nat_config(char (*nat_config_arg)[MAX_IPFW_RULE_ARGS_SIZE])
{
	if(strcmp(nat_config_arg[0], "ipfw") == 0){
		if(strcmp(nat_config_arg[1], "nat") == 0){
			if(isvalid_ipfw_digit(nat_config_arg[2])){
				if(strcmp(nat_config_arg[3], "config") == 0){
					if(strcmp(nat_config_arg[4], "ip") == 0){
						if(isvalidip(nat_config_arg[5]) || isvalid_ip_mask(nat_config_arg[5])){
							return 1;
						}
						else{
							NET_ERROR("nat : ip-mask error");
							return 0;
						}
					}
					else if (strcmp(nat_config_arg[4], "if") == 0){
						if(strcmp(nat_config_arg[5], WAN_INTERFACE) == 0 || strcmp(nat_config_arg[5], WAN_INTERFACE_PPP) == 0 ){
							return 1;
						}
						else{
							NET_ERROR("nat : interface error");
							return 0;
						}
					}
					else
					{
						NET_ERROR("nat : ip/if error\n");
						return 0;
					}

				}
				else{
					NET_ERROR("nat : config error\n");
					return 0;
				}
			}
			else{
				NET_ERROR("nat : id error\n");
				return 0;
			}
		}
		else{
			NET_ERROR("nat : nat error\n");
			return 0;
		}
	}
	else{
		NET_CRITICAL("Invalid argument\n");
		return 0;
	}
}

/* Validating MTU size for pppoE */
int is_valid_mtu(unsigned short c){
    if(c >= PPPOE_MTU_MIN && c <= PPPOE_MTU_MAX){
        return 1;
    }
    return 0;
}

int do_flush_action(int ac, char (*flush_arg)[24], FILE *client){

	int i=0;
	strlcpy(flush_arg[0], "ipfw", MAX_IPFW_RULE_ARGS_SIZE);
	strlcpy(flush_arg[1], "flush", MAX_IPFW_RULE_ARGS_SIZE);
	if(ac == 3){
		if(strcmp(flush_arg[2], "all") == 0){
			strlcpy(flush_arg[2], "0", MAX_IPFW_RULE_ARGS_SIZE);
			if ( ecosap_nw_ipfw_rules_config(ac ,flush_arg, NULL) == -1 )
			{
				HTTP_PRINT(client, "IPFW Flush failed\n");
				return -1;
			}
			default_flash_rule();
			ecosap_config_struct.is_flashrule_full = 0;
			ecosap_new_nw_config.is_flashrule_full = 0;
		}
		else if(strcmp(flush_arg[2], "user") == 0){
			strlcpy(flush_arg[2], "1", MAX_IPFW_RULE_ARGS_SIZE);
			if ( ecosap_nw_ipfw_rules_config(ac ,flush_arg, NULL) == -1 )
			{
				HTTP_PRINT(client, "IPFW Flush failed\n");
				return -1;
			}
			for(i=0; i<5; i++){
				ecosap_config_struct.ipfw_config[i].is_deleted = 1;
				ecosap_new_nw_config.ipfw_config[i].is_deleted = 1;
			}
			ecosap_config_struct.is_flashrule_full = 0;
			ecosap_new_nw_config.is_flashrule_full = 0;
		}
		else if(strcmp(flush_arg[2], "default") == 0){
			strlcpy(flush_arg[2], "2", MAX_IPFW_RULE_ARGS_SIZE);
			if ( ecosap_nw_ipfw_rules_config(ac ,flush_arg, NULL) == -1 )
			{
				HTTP_PRINT(client, "IPFW Flush failed\n");
				return -1;
			}
		}
		else {
			HTTP_PRINT(client, "Invalid flush argument\n");
			return -1;
		}
		write_nw_flash();
	}
	else{
		HTTP_PRINT(client, "Invalid argument count in flush");
		return -1;
	}
	HTTP_PRINT(client, "Flush action success...\n");
	return 0;
}

void ipfw_help_msg(FILE *client){

	HTTP_PRINT(client, "Add IPFW Usage : \n");
	HTTP_PRINT(client, "ipfw add [ID] [ACTION] [PROTOCOL] from [SOURCE] to [DESTINATION] {via} {INTERFACE}\n");
	HTTP_PRINT(client, "Example : ipfw add 300 allow ip from 192.168.0.100 to 192.168.0.200 via eth0 \n\n");
	if(client != NULL) HTTP_PRINT(client,"<br>");

	HTTP_PRINT(client, "Delete IPFW Usage : \n");
	HTTP_PRINT(client, "ipfw delete [ID]\n");
	HTTP_PRINT(client, "Example : ipfw delete 300\n\n");
	if(client != NULL) HTTP_PRINT(client,"<br>");

	HTTP_PRINT(client, "NAT Rule Usage : \n");
	HTTP_PRINT(client, "ipfw nat [ID] config if [INTERFACE] redirect_port [tcp/udp] IP:INTERIAL_PORT EXTERNAL_POST\n");
	HTTP_PRINT(client, "Example : ipfw nat 300 config if ppp0 redirect_port tcp 192.168.0.100:22 22\n\n");
	if(client != NULL) HTTP_PRINT(client,"<br>");

	HTTP_PRINT(client, "Delete NAT Rule Usage : \n");
	HTTP_PRINT(client, "ipfw nat delete [ID]\n");
	HTTP_PRINT(client, "Example : ipfw nat delete 300\n\n");
	if(client != NULL) HTTP_PRINT(client,"<br>");

	HTTP_PRINT(client, "Flush Usage :\n");
	HTTP_PRINT(client, "ipfw flush [all/user/default]\n\n");
	if(client != NULL) HTTP_PRINT(client,"<br>");

	HTTP_PRINT(client, "Zero Usage :\n");
	HTTP_PRINT(client, "ipfw zero [ID1, ID2, ...]\n\n");
	if(client != NULL) HTTP_PRINT(client,"<br>");
}

int do_ipfw_zero(int ac, char (*ipfw_args)[24], FILE *client){
	int i=2;
	while(i < ac){
		if(isvalid_ipfw_digit(ipfw_args[i]) != 1){
			HTTP_PRINT(client, "Invalid ipfw zero arguments\n");
			return -1;
		}
		NET_DEBUG(" %s", ipfw_args[i]);
		++i;
	}
	if ( ecosap_nw_ipfw_rules_config(ac, ipfw_args, client) == -1 )
	{
		HTTP_PRINT (client, "IPFW zero configuration failed\n");
		return -1;
	}
	HTTP_PRINT (client, "IPFW configuration success\n");
	return 0;
}

/* Function to change dhcp address pool according to lan ip address */
void change_dhcp_pool(void)
{

	struct in_addr addr1, addr2;
	u_int32_t dhcp_from, dhcp_to;
	inet_aton(ecosap_new_nw_config.dhcp_server_config.DHCP_From, &addr1);
	inet_aton(ecosap_new_nw_config.dhcp_server_config.DHCP_To, &addr2);
	dhcp_from = addr1.s_addr;
	dhcp_to   = addr2.s_addr;

	dhcp_from = (dhcp_from & 0x000000FF)|(ecosap_new_nw_config.bridgeConf.inaddr_ip & 0xFFFFFF00);
	dhcp_to   = (dhcp_to & 0x000000FF)|(ecosap_new_nw_config.bridgeConf.inaddr_ip & 0xFFFFFF00);

	addr1.s_addr = dhcp_from;
	addr2.s_addr = dhcp_to;
	strlcpy(ecosap_new_nw_config.dhcp_server_config.DHCP_From, inet_ntoa(addr1), IPADDR_SIZE);
	strlcpy(ecosap_new_nw_config.dhcp_server_config.DHCP_To, inet_ntoa(addr2), IPADDR_SIZE);
}

/* Function to get last units from interface name */
int get_unit(char * interface_name) 
{

    int i = 0;
    int digits = 0;

    for (i = strlen(interface_name) - 1; ; i--)
    {
        if(isdigit(interface_name[i]))
            digits++;
        else
            break;
    }
	 return (atoi((interface_name + strlen(interface_name)) - digits));
}

/* Funtion to check ip is broadcast or not */

int is_bcast_ip(char *ip_str)
{
	struct in_addr addr;
	u_int32_t ip_addr;
	if (inet_aton(ip_str, (struct in_addr *)&addr) == 0) {
		NET_ERROR("Invalid address\n");
		return 0;
	}
	ip_addr = addr.s_addr;
	if(((ip_addr & 0xFF)  == 0xFF) && ((ip_addr & 0xFF000000)  != 0)){
		return 1;
	}
	return 0;
}

void cli_http_rule_deletion(void)
{
		if (is_flash_rule == 1) {
				ecosap_config_struct.is_flashrule_full = 0;
				ecosap_new_nw_config.is_flashrule_full = 0;
				write_nw_flash();
		}
}

int is_flash_ipfw_full(void)
{
	int i;
	for(i = 0; i < FLASH_IPFW_INDEX_SIZE; i++){
		if(ecosap_config_struct.ipfw_config[i].is_deleted == 1)
			return 0;
	}
	return 1;
}

void del_duplicate_nat(char *id)
{
	int i;
	for(i = 0; i < FLASH_IPFW_INDEX_SIZE; i++) {
		if(!strcmp(ecosap_new_nw_config.ipfw_config[i].ipfw_rules[2], id) &&
				!ecosap_new_nw_config.ipfw_config[i].is_deleted) {
			ecosap_config_struct.ipfw_config[i].is_deleted = 1;
			ecosap_new_nw_config.ipfw_config[i].is_deleted = 1;
		}
	}
}

void write_ipfw_rules_in_flash(int argc, char (*ipfw_args)[MAX_IPFW_RULE_ARGS_SIZE], FILE *client)
{
	int i, k;
	pthread_mutex_lock(&write_nw_mutex);
	if(!strcmp(ipfw_args[1], "nat") && ipfw_args[2]) {
		del_duplicate_nat(ipfw_args[2]);
	}
	ecosap_config_struct.is_flashrule_full = (unsigned int)is_flash_ipfw_full();
	ecosap_new_nw_config.is_flashrule_full = ecosap_config_struct.is_flashrule_full;
	if(ecosap_new_nw_config.is_flashrule_full == 1) {
		HTTP_PRINT(client, "EXCEEDED FLASH LIMIT FOR IPFW\n");
	}
	else if(ecosap_new_nw_config.is_flashrule_full == 0) {
		for(k = 0; k < FLASH_IPFW_INDEX_SIZE; k++) {
			if(ecosap_new_nw_config.ipfw_config[k].is_deleted == 1){ 
				memset(ecosap_config_struct.ipfw_config[k].ipfw_rules, '\0', sizeof(ecosap_config_struct.ipfw_config[k].ipfw_rules)) ;
				memcpy(ecosap_config_struct.ipfw_config[k].ipfw_rules, ipfw_args, MAX_IPFW_RULE_ARGC*MAX_IPFW_RULE_ARGS_SIZE) ;
				memset(ecosap_new_nw_config.ipfw_config[k].ipfw_rules, '\0', sizeof(ecosap_config_struct.ipfw_config[k].ipfw_rules)) ;
				memcpy(ecosap_new_nw_config.ipfw_config[k].ipfw_rules, ipfw_args, MAX_IPFW_RULE_ARGC*MAX_IPFW_RULE_ARGS_SIZE) ;
				for(i = argc; i < MAX_IPFW_RULE_ARGC; i++){
					memset(ecosap_config_struct.ipfw_config[k].ipfw_rules[i], '\0', MAX_IPFW_RULE_ARGS_SIZE);
					memset(ecosap_new_nw_config.ipfw_config[k].ipfw_rules[i], '\0', MAX_IPFW_RULE_ARGS_SIZE);
				}
				ecosap_config_struct.ipfw_config[k].ipfw_argc = argc;	
				ecosap_new_nw_config.ipfw_config[k].ipfw_argc = argc;	
				ecosap_config_struct.ipfw_config[k].is_deleted = 0;
				ecosap_new_nw_config.ipfw_config[k].is_deleted = 0;
				write_nw_flash();
				break;
			}
		}
	}
	pthread_mutex_unlock(&write_nw_mutex);
}
