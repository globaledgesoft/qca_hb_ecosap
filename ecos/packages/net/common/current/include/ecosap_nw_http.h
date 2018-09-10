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

#ifndef __ECOSAP_NW_HTTP_H
#define __ECOSAP_NW_HTTP_H

#include <sys/types.h>
#include <stdio.h>
#include <cyg/dbg_print/dbg_print.h>
#include <net/if.h>
#include <pthread.h>

#include "ecosap_nw_config.h"
#define HTTP_PRINT(x, ...) do{ if(x == NULL) {diag_printf( __VA_ARGS__ );}  else {fprintf( x, __VA_ARGS__ ); fprintf( x, "<br>");}} while(0)

#define PPPOE_MTU_MIN 1340
#define PPPOE_MTU_MAX 1492

#define IPADDR_SIZE 16
#define SUBNET_ADDR_SIZE 32
#define DEFAULT_DMZ_IP "192.168.0.10"

/* VLAN id range to configure VLAN manually*/
#define VLAN_VALUE_MIN 4
#define VLAN_VALUE_MAX 4096

/* Mutex variables for read and write network struct */
pthread_mutex_t read_nw_mutex;
pthread_mutex_t write_nw_mutex;

/*IPFW and NAT related macros*/
#define FLASH_IPFW_INDEX_SIZE 5
#define IPFW_DELETE_ARGC 3
#define NAT_DELETE_ARGC 4

#define FTP_CONTROL_PORT 21
#define FTP_DATA_PORT 20

/*HTTPD related macros*/
#define HTTPD_BUFFER_SIZE 24
#define HTTPD_FORMLIST_SIZE 10

/*Same Port packet flow handling Macros*/

/** Structure for bridge configurataion*/
struct bridgeConf {
        u_int32_t inaddr_ip;
        u_int32_t inaddr_mas;
        u_int32_t inaddr_gw;
        u_int32_t inaddr_broad;
};

//struct bridgeConf bridgeConf;

/** structure which holds WAN Configuration values **/
typedef struct WAN_config_struct {
	char IPAddr[IPADDR_SIZE];
	char Mask[IPADDR_SIZE];
	char Gateway[IPADDR_SIZE];
	char DNS[IPADDR_SIZE];
        char ADNS[IPADDR_SIZE];
        char Broadcast[IPADDR_SIZE] ;
        char Server[IPADDR_SIZE] ;
} WAN_CONFIG_STRUCT;

/** structure which holds LAN Configuration values **/
typedef struct LAN_config_struct {
	char IPAddr[IPADDR_SIZE];
	char Mask[IPADDR_SIZE];
	char Broadcast[IPADDR_SIZE];
}LAN_CONFIG_STRUCT;
LAN_CONFIG_STRUCT LAN_config_addrs;

/** structure which holds PPPoE Configuration info **/
typedef struct PPPoE_config_struct {
        char Username[32];
        char Passwd[32];
        unsigned short MTU;
        char Service_Name[16];
        char Server_Name[16];
        unsigned long MAX_Idle_Time ;
} PPPoE_CONFIG_STRUCT;

/** structure which holds DHCP Server confgiuration values **/
typedef struct DHCP_config_struct {
	unsigned int dhcps_enable;
	char DHCP_From[IPADDR_SIZE];
	char DHCP_To[IPADDR_SIZE];
	unsigned long Lease_Time ;
}DHCPS_CONFIG_STRUCT;

/** structure to store firewall rules **/
typedef struct IPFW_config_struct {
       char ipfw_rules[MAX_IPFW_RULE_ARGC][MAX_IPFW_RULE_ARGS_SIZE];
       unsigned int ipfw_argc;
       unsigned int is_deleted;
}IPFW_CONFIG_STRUCT;

typedef struct vpnpt_config_struct {
         unsigned int is_pptp_enable;
         unsigned int is_l2tp_enable;
         unsigned int is_ipsec_enable;
}VPNPT_CONFIG_STRUCT;

#define MAX_BRINTERFACES 10
typedef struct br_info {
	char		br_name[IFNAMSIZ];
	u_int8_t 	br_intno;
	char		br_interfaces[MAX_BRINTERFACES][IFNAMSIZ];
}BR_INFO;

typedef struct VLAN_config {
	unsigned int vlan_flag;
	unsigned int is_tagged;
	unsigned int inet_value;
	unsigned int iptv_value;
}VLAN_CONFIG;

typedef struct DMZ_config {
	unsigned int status;
	char host[IPADDR_SIZE];
}DMZ_CONFIG;

#define CONFIG_WLAN_MAGIC_NUM 98765

struct WLAN_AP_Config {
    long int if_idx;
    char ssid[36];
    unsigned int channel;
    unsigned int bw;
    char wlan_mode[16];
    char security[16];
    char passphrase[65];
    int beacon_int;
    int wds;
};

struct WLAN_STA_Config {
    long int if_idx;
    char ssid[36];
    char security[16];
    char passphrase[64];
    int wds;
};

struct WLAN_AP_VLAN_IPTV_Config {
    char ssid[36];
    unsigned int channel;
    char security[16];
    char passphrase[65];
    int beacon_int;
};

struct WLAN_Default_Config {
    unsigned int magic_num;
    char mode[16]; /* ap/rootap/repeater */
    struct WLAN_AP_Config ap_config;
    struct WLAN_AP_VLAN_IPTV_Config vlan_iptv_ap_config;
    struct WLAN_STA_Config sta_config;
};

#define IGMP_DISABLE (0)
#define IGMP_ENABLE  (1)

typedef struct igmp_config {
    unsigned int status;
}IGMP_CONFIG;

/** TODO: Reverify */
typedef struct ecosap_config_struct {

	char mac_addr[24];	 // XXX TODO DONOT CHANGE THE mac_addr member FROM HERE it should be at the start.
	char __conf_pad__[8] ;    // alignment issue causing an issue while printing network configuration
	unsigned int Magic_Number;
	unsigned int Connection_Type;
	unsigned int lan_status;
	unsigned int wan_status;
	unsigned int is_flashrule_full;
	VLAN_CONFIG vlan_config;
	VPNPT_CONFIG_STRUCT VPNPT_config_struct;

	struct WAN_CONF{
	WAN_CONFIG_STRUCT WAN_config_addrs ;
        PPPoE_CONFIG_STRUCT PPPoE_config_values ;
	} wan_conf;

	struct bridgeConf bridgeConf;
	DHCPS_CONFIG_STRUCT dhcp_server_config ;
	BR_INFO br_info_struct;
	IPFW_CONFIG_STRUCT ipfw_config[FLASH_IPFW_INDEX_SIZE] ;
	DMZ_CONFIG dmz_config;
	IGMP_CONFIG igmp_config;
	char reserved[24];	 // Reserved for future use
	struct WLAN_Default_Config WLAN_config;
} ECOSAP_CONFIG_STRUCT ;

ECOSAP_CONFIG_STRUCT ecosap_config_struct, ecosap_new_nw_config ;


typedef struct nw_host {
    char network[16] ;
    char host[16] ;
} NW_HOST_ADDR ;

/** To set connection Type **/
enum conn_type { DHCP, PPPoE, STATIC_IP } ;

int ADD_ROUTE(u_int32_t , u_int32_t , u_int32_t , char *);


/** Functions to handle network configuration **/
void show_nw_config(void) ;
void ecosap_nw_config_default(void);

void ecosap_nw_default_start(void);
int Is_Config_Available(void);
void ecosap_read_nw_config (void);
bool isvalidip(const char * ) ;/** function to validate IPv4 address **/
int isvalid_digit(char * ) ;
int isvalid_range(char *, char * ) ;
char findClass(char *) ;
int isvalid_mask(char *) ;
void separate(char *, char, NW_HOST_ADDR *);
int isvalidrange(char * , char *);
unsigned long remove_dots(char *) ;
int is_valid_mtu(unsigned short);
int is_bcast_ip(char *ip_str);
void change_dhcp_pool( void );
int do_flush_action(int ac, char (*flush_arg)[MAX_IPFW_RULE_ARGS_SIZE], FILE *client);
void ipfw_help_msg(FILE *client);
int do_ipfw_zero(int ac, char (*ipfw_args)[MAX_IPFW_RULE_ARGS_SIZE], FILE *client);
void delete_rule_from_flash(char *str);
void write_nw_flash(void);
void delete_nat_from_flash(char *str);
void write_ipfw_rules_in_flash(int argc, char (*ipfw_args)[MAX_IPFW_RULE_ARGS_SIZE], FILE *client);
void dmz_error(void);
int enable_dmz_ipfw_rule(void);
int disable_dmz_ipfw_rule(void);
bool issamesubnet(const char * wan_ip, const char * lan_ip);
void copy_nw_config_to_new_struct(void);
int isvalid_ipfw_digit(char *str);
int ecosap_nw_nat_config (int from_flash, int argc, char av[][MAX_IPFW_RULE_ARGS_SIZE], FILE *client);
int ecosap_nw_ipfw_rules_show (FILE *client );
int ecosap_nw_ipfw_rules_config (int argc, char av[][MAX_IPFW_RULE_ARGS_SIZE], FILE *client);
bool issamesubnet(const char *, const char * );
void change_lan_subnet(void);
bool isvalid_subnetip(const char * addr);
int calculate_mask_bits(char *net_mask );
int gen_broadcast_addr(char *host_ip, char *netmask, char *bcast);
int set_lanmask(char class);
size_t strlcpy(char *dst, const char *src, size_t size);
int isvalid_ip_mask (char *addr);
int isvalidip_port(char *str);

typedef struct Nat_Config {
     char *id ;
     char *ext_port ;
     char *int_port ;
     char *internal_ip ;
     char *protocol ;
   //  char *interface ;
} NAT_CONFIG_STRUCT ;

typedef struct Redirect_Address_Config {
     char *id;
     char *from_ip;
     char *to_ip;
     char *interface ;
} NAT_READDR_STRUCT ;
/** TODO: Move all these definitions to ".c" file */

int isvalid_cmd(char (*)[MAX_IPFW_RULE_ARGS_SIZE], FILE *client);


#define NAT_LOOP_COUNT 2

#define  MIN_CLI_HTTP_IPFW_ARGC 2
#define  MAX_CLI_HTTP_IPFW_ARGC 14
#endif //__ECOSAP_NW_HTTP_H

