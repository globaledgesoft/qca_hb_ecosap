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

#include "ping.h"
#include <ecosap_nw_http.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/diag.h>
#include <cyg/io/devtab.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/var_intr.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/hal/hal_arch.h>

#include <ctype.h>
#include <stdlib.h>
#include <shell.h>
#include <shell_err.h>
#include <shell_thread.h>
#include <commands.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <pkgconf/system.h>
#include "ecosap_nw_config.h"
#include "debug.h"

#define CLI_STATIC_CONFIG_ARGC 13

#ifdef CYGPKG_COMPONENT_IPFW
static char ipfw_args[MAX_IPFW_RULE_ARGC][MAX_IPFW_RULE_ARGS_SIZE];
char nat_args[MAX_IPFW_RULE_ARGC][MAX_IPFW_RULE_ARGS_SIZE];
#endif

#ifdef NET_PKT_DEBUG
extern bool network_debug;
#endif

int isvalidTime(unsigned int);
void pexit(char *);
void get_thread_info(void);
void route_add_print_usage(void) ;
void set_lan_print_usage(void);
void set_wan_print_usage(void);
void dhcp_print_usage(void);
void vpnpt_print_usage(void);
void vlan_print_usage(void);
extern int brconfig_main(int argc, char *argv[]);
extern void watchdog_reboot(void);
#ifdef CYGPKG_COMPONENT_IPTV
CMD_DECL(setiptvconfig)
{
	unsigned int is_enable;
	unsigned int vlan_id;
	unsigned int is_tagged;
	uint8_t count =0;
	if(argc == 0)
	{
		vlan_print_usage();
		return SHELL_INVALID_ARGUMENT;
	}
	if(strcmp((const char *)argv[0], "enable")==0 && (argc == 3)){
		is_enable =1;
		count ++;
	}
	else if(strcmp((const char *)argv[0], "disable")==0 && (argc == 1)){
		is_enable =0;
		pthread_mutex_lock(&write_nw_mutex);
		ecosap_new_nw_config.vlan_config.vlan_flag = is_enable;	
		write_nw_flash();
		pthread_mutex_unlock(&write_nw_mutex);
		return SHELL_OK;
	}
	else{
		vlan_print_usage();
		return SHELL_INVALID_ARGUMENT;
	}
	if(argc == 3){
			if(isvalid_digit((char *)argv[1])){
				vlan_id = (unsigned int) atoi(argv[1]);
				if(vlan_id >=VLAN_VALUE_MIN && vlan_id <=VLAN_VALUE_MAX){
					count ++;
				}
				else{
					SHELL_PRINT("VLAN ID should between 4 to 4096\n");
					return SHELL_INVALID_ARGUMENT;
				}
				if(strcmp((const char *)argv[2], "access") == 0){
					is_tagged = 0;
					count ++;
				}
				else if(strcmp((const char *)argv[2], "trunk") == 0){
					is_tagged = 1;
					count ++;
				}
				else {
					SHELL_PRINT("Invalid tagged parameter\n");
					return SHELL_INVALID_ARGUMENT;
				}
			}
			else {
				SHELL_PRINT("Invalid VLAN ID\n");
				return SHELL_INVALID_ARGUMENT;
			}
		if(count == 3){
		
			pthread_mutex_lock(&write_nw_mutex);
			ecosap_new_nw_config.vlan_config.vlan_flag = is_enable;	
			ecosap_new_nw_config.vlan_config.is_tagged = is_tagged;	
			ecosap_new_nw_config.vlan_config.iptv_value = vlan_id;	
			write_nw_flash();
			pthread_mutex_unlock(&write_nw_mutex);
			SHELL_PRINT("Configuration success !\n");
			SHELL_PRINT("vlan flag : %u\n", ecosap_new_nw_config.vlan_config.vlan_flag);	
			SHELL_PRINT("is_tagged : %u\n", ecosap_new_nw_config.vlan_config.is_tagged);	
			SHELL_PRINT("iptv_value: %u\n", ecosap_new_nw_config.vlan_config.iptv_value);	
		}
	}
	return SHELL_OK;
}
#endif


#ifdef CYGPKG_COMPONENT_IPFW
CMD_DECL(vpn_passthrough)
{
	int is_enable;
	if(argc == 2){
		if(strcmp((const char *)argv[1], "enable")==0)
			is_enable =1;
		else if(strcmp((const char *)argv[1], "disable")==0)
			is_enable =0;
		else{
			vpnpt_print_usage();
			return SHELL_INVALID_ARGUMENT;
		}
		if(strcmp((const char *)argv[0], "pptp") == 0){
			pthread_mutex_lock(&write_nw_mutex);
			ecosap_new_nw_config.VPNPT_config_struct.is_pptp_enable = is_enable;
			pthread_mutex_unlock(&write_nw_mutex);
		}
		else if(strcmp((const char *)argv[0], "l2tp") == 0){
			pthread_mutex_lock(&write_nw_mutex);
			ecosap_new_nw_config.VPNPT_config_struct.is_l2tp_enable = is_enable;
			pthread_mutex_unlock(&write_nw_mutex);
		}
		else if(strcmp((const char *)argv[0], "ipsec") == 0){
			pthread_mutex_lock(&write_nw_mutex);
			ecosap_new_nw_config.VPNPT_config_struct.is_ipsec_enable = is_enable;
			pthread_mutex_unlock(&write_nw_mutex);
		}
		else{
			vpnpt_print_usage();
			return SHELL_INVALID_ARGUMENT;
		}
		write_nw_flash();
	}
	else {
        vpnpt_print_usage();
		return SHELL_INVALID_ARGUMENT;
    }
    return SHELL_OK;
}
#endif //ipfw_component


#ifdef CYGPKG_COMPONENT_BRIDGE
CMD_DECL(brconfig)
{
    brconfig_main(argc, argv);
    return SHELL_OK;
}
#endif

CMD_DECL(brinfo)
{
    int i, j;

    SHELL_PRINT("Bridge_info:\n");
    for (i = 0; i < 1; i++) {
        SHELL_PRINT("\nbridge name: %s\n", ecosap_config_struct.br_info_struct.br_name);
        SHELL_PRINT("number of bridge interfaces: %d\n", ecosap_config_struct.br_info_struct.br_intno);
        SHELL_PRINT("interfaces: \n");

        for (j = 0; j < 6; j++)
            if(strcmp((ecosap_config_struct.br_info_struct.br_interfaces[j]), "\0"))
                SHELL_PRINT(" %s\n",(ecosap_config_struct.br_info_struct.br_interfaces[j]));
    }
    return SHELL_OK;
}
CMD_DECL(show_config)
{
    show_nw_config();
    return SHELL_OK;
}

CMD_DECL(make_default)
{
    ecosap_nw_config_default();
    SHELL_PRINT("REBOOTING WITH DEFAULT VALUES\n");
    watchdog_reboot(); // Platform api for reboot
    return SHELL_OK;
}

CMD_DECL(wflash)
{
    write_nw_flash();
    return SHELL_OK;
}

CMD_DECL(set_wan_config)
{
    int i ;

    /* PPPoE Temp variables*/
    char *ppp_username = NULL;
    char *ppp_password = NULL;
    unsigned short mtu  = 0;

    /* Static Temp variables*/
    char *static_ip = NULL;
    char *static_mask = NULL;
    char static_bcast[IPADDR_SIZE] = {0};
    char *static_gateway = NULL;
    char *static_dns = NULL;
    char *static_adns = NULL;
    char *static_server = NULL;

    if ( argc < 1 ) {
        SHELL_PRINT("Few arguments\n");
        set_wan_print_usage() ;
        return 0 ;
    }
    /** responding to help request **/
    if ( !strcmp((const char *)argv[0], "-h") ) {
	    set_wan_print_usage() ;
	    return SHELL_INVALID_ARGUMENT;
    }

    if ( !strcmp((const char *)argv[0], "0") ) { /**to configure DHCP **/
        if ( argc > 2 ) {
            SHELL_ERROR("Too many arguments\n") ;
	    return SHELL_INVALID_ARGUMENT;
        } else {
            pthread_mutex_lock(&write_nw_mutex);
            ecosap_new_nw_config.Connection_Type = DHCP;
            write_nw_flash();
            pthread_mutex_unlock(&write_nw_mutex);
        }
        return 0 ;
    } 
#ifdef CYGPKG_COMPONENT_PPPoE
    else if ( !strcmp((const char *)argv[0], "1") ) {/** to configure PPPoE **/
        if ( argc != 7 ) {
            SHELL_ERROR("Error:Wrong number of arguments\n") ;
            SHELL_USAGE("Usage:setwanconfig 1 -U [PPPoE Username] -P [PPPoE Password] -M [MTU]\n") ;
	    return SHELL_INVALID_ARGUMENT;
        } else {
            for ( i = 1 ; i < 6 ;  i = i + 2) {
                if ( !strcmp ((const char *)argv[i ], "-U"  ) ) {
                    ppp_username = (char *)argv[i + 1] ;
                } else if ( !strcmp ((const char *)argv[i], "-P"  ) ) {
                    ppp_password = (char *)argv[i + 1] ;
                }
                else if ( !strcmp ((const char *)argv[i], "-M"  ) ) {
                    if(isvalid_digit((char *)argv[i+1])){
                        mtu = (unsigned short)(atoi((const char *)argv[i + 1])) ;
                        if(is_valid_mtu(mtu) == 0){
                            SHELL_ERROR("Error:Allowed MTU range is %u-%u\n",PPPOE_MTU_MIN, PPPOE_MTU_MAX);
                            return SHELL_INVALID_ARGUMENT;
                        }
                    }
                    else{
			    SHELL_ERROR("Error:Invalid MTU\n");
			    return SHELL_INVALID_ARGUMENT;
                    }
                }
#if 0

                else if ( !strcmp ((const char *)argv[i], "-s"  ) ) {

                    strcpy(ecosap_config_struct.wan_conf.PPPoE_config_values.Service_Name, (char *)argv[i + 1]) ;

                } else if ( !strcmp ((const char *)argv[i], "-S"  ) ) {

                    strcpy(ecosap_config_struct.wan_conf.PPPoE_config_values.Server_Name, (char *)argv[i + 1]);

                } else if ( !strcmp ((const char *)argv[i], "-T"  ) ) {

                    ecosap_config_struct.wan_conf.PPPoE_config_values.MAX_Idle_Time = (unsigned long)atoi((const char *)argv[i + 1]) ;

                }
#endif
                else {
                    SHELL_ERROR("Unrecognized option\n") ;
                    return SHELL_INVALID_ARGUMENT;
                }
            }
            pthread_mutex_lock(&write_nw_mutex);
            SHELL_PRINT ("Enabling PPPoE\n") ;
            ecosap_new_nw_config.Connection_Type = PPPoE;
            strlcpy(ecosap_new_nw_config.wan_conf.PPPoE_config_values.Username,(char *)ppp_username, sizeof(ecosap_new_nw_config.wan_conf.PPPoE_config_values.Username)) ;
            strlcpy(ecosap_new_nw_config.wan_conf.PPPoE_config_values.Passwd,(char *)ppp_password, sizeof(ecosap_new_nw_config.wan_conf.PPPoE_config_values.Passwd)) ;
            ecosap_new_nw_config.wan_conf.PPPoE_config_values.MTU = mtu;

            SHELL_PRINT ("Configured values for PPPoE connection are :\n") ;
            SHELL_PRINT ("PPPoE Username %s \n", ecosap_new_nw_config.wan_conf.PPPoE_config_values.Username ) ;
            SHELL_PRINT ("PPPoE Password %s \n", ecosap_new_nw_config.wan_conf.PPPoE_config_values.Passwd ) ;
            SHELL_PRINT ("MTU            %u\n",  ecosap_new_nw_config.wan_conf.PPPoE_config_values.MTU ) ;
            
            write_nw_flash();
            pthread_mutex_unlock(&write_nw_mutex);
        }
    }
#endif
     else if ( !strcmp ((const char *)argv[0],"2") ) { /** to do WAN configuration **/
        if ( argc != CLI_STATIC_CONFIG_ARGC ) {
            SHELL_ERROR("Wrong number of arguments\n") ;
	    return SHELL_INVALID_ARGUMENT;
        } else {
            for ( i = 1 ; i < CLI_STATIC_CONFIG_ARGC ;  i = i + 2) {
                if ( !strcmp ((const char *)argv[i ], "-I"  ) ) {
                    if ( isvalidip((char *)argv[i + 1]) ) {
                        static_ip = (char *)argv[i + 1] ;
                    } else {
                        SHELL_PRINT ("IPAddress %s is invalid\n", argv[i + 1]) ;
                        return 0 ;
                    }

                } else if ( !strcmp ((const char *)argv[i], "-G"  ) ) {
                    if ( isvalidip((char *)argv[i + 1]) ) {
                        static_gateway = (char *)argv[i + 1] ;
                    } else {
                        SHELL_PRINT ("Gateway %s is invalid\n", argv[i + 1]) ;
                        return 0 ;
                    }

                } else if ( !strcmp ((const char *)argv[i], "-M"  ) ) {
                    if ( isvalid_mask((char *)argv[i + 1]) ) {
                        static_mask = (char *)argv[i + 1] ;
                    } else {
                        SHELL_PRINT ("Mask %s is invalid\n", argv[i + 1]) ;
                        return 0 ;
                    }
                } else if ( !strcmp ((const char *)argv[i], "-D1"  ) ) {
                    if ( isvalidip((char *)argv[i + 1]) ) {
                        static_dns = (char *)argv[i + 1] ;
                    } else {
                        SHELL_PRINT ("DNS1 %s is invalid\n", argv[i + 1]) ;
                        return 0 ;
                    }
                } else if ( !strcmp ((const char *)argv[i], "-D2"  ) ) {
                    if ( isvalidip((char *)argv[i + 1]) ) {
                        static_adns = (char *)argv[i + 1] ;
                    } else {
                        SHELL_PRINT ("DNS2 %s is invalid\n", argv[i + 1]) ;
                        return 0 ;
                    }
                } else if ( !strcmp ((const char *)argv[i], "-S"  ) ) {
                    if ( isvalidip((char *)argv[i + 1]) ) {
                        static_server = (char *)argv[i + 1] ;
                    } else {
                        SHELL_PRINT ("Server Address %s is invalid\n", argv[i + 1]) ;
                        return 0 ;
                    }

                } else {
                    SHELL_PRINT ( "Unrecognized option\n" ) ;
                    return 0 ;
                }
            }

			if ( (static_ip) && (inet_addr(static_ip) == ecosap_config_struct.bridgeConf.inaddr_ip) ) {
				SHELL_PRINT ("IP Address:%s is in Use!", static_ip ) ;
				return 0;
			}

			if ( (static_dns) && (inet_addr(static_dns) == ecosap_config_struct.bridgeConf.inaddr_ip) ) {
				SHELL_PRINT ("DNS IP:%s same as LAN IP! Give different IP\n", static_dns ) ;
				return 0;
			}

			if ( (static_adns) && (inet_addr(static_adns) == ecosap_config_struct.bridgeConf.inaddr_ip) ) {
				SHELL_PRINT ("ADNS IP:%s same as LAN IP! Give different IP\n", static_adns ) ;
				return 0;
			}
			
			if (gen_broadcast_addr(static_ip, static_mask, static_bcast) != 0 ) {
				NET_ERROR ("Unable to generate Broadcast\n") ;
				return 1;
			}

			struct in_addr lan_ip;
			lan_ip.s_addr = ecosap_config_struct.bridgeConf.inaddr_ip;
			if (issamesubnet(static_ip, inet_ntoa(lan_ip))) {
				SHELL_PRINT("IP Conflict! LAN using same subnet! Give different subnet\n");
				return 0;
			}

			pthread_mutex_lock(&write_nw_mutex);
			SHELL_PRINT ("Enabling Static Connection\n") ;
            ecosap_new_nw_config.Connection_Type = STATIC_IP;
            strlcpy(ecosap_new_nw_config.wan_conf.WAN_config_addrs.IPAddr, (char *)static_ip, sizeof(ecosap_new_nw_config.wan_conf.WAN_config_addrs.IPAddr)) ;
            strlcpy(ecosap_new_nw_config.wan_conf.WAN_config_addrs.Gateway, (char *)static_gateway, sizeof(ecosap_new_nw_config.wan_conf.WAN_config_addrs.Gateway)) ;
            strlcpy(ecosap_new_nw_config.wan_conf.WAN_config_addrs.Mask, (char *)static_mask, sizeof(ecosap_new_nw_config.wan_conf.WAN_config_addrs.Mask)) ;
            strlcpy(ecosap_new_nw_config.wan_conf.WAN_config_addrs.DNS, (char *)static_dns, sizeof(ecosap_new_nw_config.wan_conf.WAN_config_addrs.DNS)) ;
            strlcpy(ecosap_new_nw_config.wan_conf.WAN_config_addrs.ADNS, (char *)static_adns, sizeof(ecosap_new_nw_config.wan_conf.WAN_config_addrs.ADNS)) ;
            strlcpy(ecosap_new_nw_config.wan_conf.WAN_config_addrs.Broadcast, (char *)static_bcast, sizeof(ecosap_new_nw_config.wan_conf.WAN_config_addrs.Broadcast)) ;
            strlcpy(ecosap_new_nw_config.wan_conf.WAN_config_addrs.Server, (char *)static_server, sizeof(ecosap_new_nw_config.wan_conf.WAN_config_addrs.Server)) ;

            SHELL_PRINT ("Configured Values are:\n") ;
            SHELL_PRINT ("IPAddr %s\n", ecosap_new_nw_config.wan_conf.WAN_config_addrs.IPAddr ) ;
            SHELL_PRINT ("Gateway %s\n", ecosap_new_nw_config.wan_conf.WAN_config_addrs.Gateway) ;
            SHELL_PRINT ("mask %s\n", ecosap_new_nw_config.wan_conf.WAN_config_addrs.Mask ) ;
            SHELL_PRINT ("dns1 %s\n", ecosap_new_nw_config.wan_conf.WAN_config_addrs.DNS ) ;
            SHELL_PRINT ("dns2 %s\n", ecosap_new_nw_config.wan_conf.WAN_config_addrs.ADNS ) ;
            SHELL_PRINT ("broadcast %s\n", ecosap_new_nw_config.wan_conf.WAN_config_addrs.Broadcast ) ;
            SHELL_PRINT ("server %s\n", ecosap_new_nw_config.wan_conf.WAN_config_addrs.Server ) ;
            write_nw_flash();
            pthread_mutex_unlock(&write_nw_mutex);
        }
    } else {
        SHELL_PRINT ("\nInvalid option\n") ;
        set_wan_print_usage() ;
    }

    return SHELL_OK ;
}

CMD_DECL(showLANstatus)
{
    struct in_addr addr1, addr2, addr3 ;

    addr1.s_addr = ecosap_config_struct.bridgeConf.inaddr_ip ;
    addr2.s_addr = ecosap_config_struct.bridgeConf.inaddr_broad ;
    addr3.s_addr = ecosap_config_struct.bridgeConf.inaddr_mas ;

    SHELL_PRINT("IP Address : %s \n", inet_ntoa(addr1)) ;
    SHELL_PRINT("Broadcast : %s \n", inet_ntoa(addr2)) ;
    SHELL_PRINT("Mask : %s \n", inet_ntoa(addr3)) ;
    return SHELL_OK;
}

CMD_DECL(set_lan_config)
{
	struct in_addr addr1, addr2, addr3 ;
	char class_type;
	if ( argc < 1 ) {
		set_lan_print_usage() ;
		return 0 ;
	}
	else {
		if (!isvalidip((char *)argv[0])) {
			SHELL_ERROR("Invalid IP Address\n");
			return -1;
		}
		strlcpy(LAN_config_addrs.IPAddr, (char *)argv[0], sizeof(LAN_config_addrs.IPAddr)) ;
	}

	if (issamesubnet(ecosap_config_struct.wan_conf.WAN_config_addrs.IPAddr, LAN_config_addrs.IPAddr)) {
		SHELL_PRINT("IP Conflict !!! WAN using same subnet !! Give different subnet\n");
		return 0;
	}
	class_type = findClass(LAN_config_addrs.IPAddr);
	if ( set_lanmask(class_type) == -1) {
		SHELL_PRINT ( "IP in invalid class type\n" ) ;
		return -1;
	}
	if (gen_broadcast_addr(LAN_config_addrs.IPAddr, LAN_config_addrs.Mask, LAN_config_addrs.Broadcast) != 0) {
		SHELL_PRINT("Unable to generate broadcast\n");
		return -1;
	}

        SHELL_PRINT ("Configured Values are:\n") ;
        SHELL_PRINT ("IPAddr %s\n", LAN_config_addrs.IPAddr ) ;
        SHELL_PRINT ("Broadcast %s\n", LAN_config_addrs.Broadcast ) ;
        SHELL_PRINT ("mask %s\n", LAN_config_addrs.Mask ) ;

	pthread_mutex_lock(&write_nw_mutex);
	inet_aton(LAN_config_addrs.IPAddr, &addr1);
	inet_aton(LAN_config_addrs.Broadcast, &addr2);
	inet_aton(LAN_config_addrs.Mask, &addr3);
	ecosap_new_nw_config.bridgeConf.inaddr_ip	= addr1.s_addr;
	ecosap_new_nw_config.bridgeConf.inaddr_broad 	= addr2.s_addr;
	ecosap_new_nw_config.bridgeConf.inaddr_mas 	= addr3.s_addr;
	change_dhcp_pool();
	write_nw_flash();
	pthread_mutex_unlock(&write_nw_mutex);
	return SHELL_OK;
}

CMD_DECL(showWANstatus)
{

    if ( ecosap_config_struct.wan_status == 1 ) {
        SHELL_PRINT("WAN Connection : UP\n") ;
	if(ecosap_config_struct.Connection_Type == STATIC_IP)
		SHELL_PRINT("Connection Type : %s\n", "Static") ;
	else
		SHELL_PRINT("Connection Type : %s\n", (ecosap_config_struct.Connection_Type) ? "PPPoE" : "DHCP") ;
        SHELL_PRINT("IP Address 	: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.IPAddr) ;
        SHELL_PRINT("Mask 	   	: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Mask) ;
        SHELL_PRINT("Gateway Address : %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Gateway) ;
        SHELL_PRINT("DNS 		: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.DNS) ;
        SHELL_PRINT("ADNS 		: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.ADNS) ;
        SHELL_PRINT("Broadcast 	: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Broadcast) ;
        SHELL_PRINT("Server 		: %s \n", ecosap_config_struct.wan_conf.WAN_config_addrs.Server) ;
        if(ecosap_config_struct.Connection_Type == PPPoE)
            SHELL_PRINT("MTU            : %u \n", ecosap_config_struct.wan_conf.PPPoE_config_values.MTU);
    } else {
        SHELL_PRINT("WAN Connection : DOWN\n") ;
    }
    return SHELL_OK;
}

#ifdef CYGPKG_COMPONENT_DHCPS
CMD_DECL(dhcp_serv_config)
{
    int i ; /* For looping purpose */
    char *ip1 = NULL, *ip2 = NULL;
    unsigned int ltime = 0;
    if ( argc < 2 ) {
        SHELL_PRINT ("Few arguments\n") ;
        dhcp_print_usage() ;
        return 0 ;
    }

    if ( !strcmp ((const char *)argv[0], "-h") ) {
        dhcp_print_usage() ;
        return 0 ;
    }

    if ( argc < 6 ) {
        SHELL_USAGE("Few arguments\n") ;
	dhcp_print_usage();
    } else if ( argc > 6 ) {
        SHELL_USAGE("Too many arguments\n") ;
	dhcp_print_usage();
    } else {

        for ( i = 0 ; i < 6 ;  i = i + 2) {
            if ( !strcmp ((const char *)argv[i ], "-S"  ) ) {
                if ( isvalidip((char *)argv[i + 1]) ) {
                    ip1 = (char *)argv[i + 1] ;
                } else {
                    SHELL_PRINT ("IPAddress1 %s is invalid\n", argv[i + 1]) ;
                    return 0 ;
                }

            } else if ( !strcmp ((const char *)argv[i], "-E"  ) ) {
                if ( isvalidip((char *)argv[i + 1]) ) {
                    ip2 = (char *)argv[i + 1] ;
                } else {
                    SHELL_PRINT ("IPAddress2 %s is invalid\n", argv[i + 1]) ;
                    return 0 ;
                }

            } else if ( !strcmp ((const char *)argv[i], "-T"  ) ) {
                if ( isvalidTime(atoi((const char *)argv[i + 1])) ) {
                    ltime = (unsigned long)atoi((const char *)argv[i + 1]);
                } else {
                    SHELL_PRINT ("Time %d is invalid\n", (atoi((const char *)argv[i + 1]))) ;
                    return 0 ;
                }

            }  else {
                SHELL_PRINT ( "Unrecognized option\n" ) ;
                return 0 ;
            }
        }
        if( ip1 && ip2 && (isvalidrange(ip1, ip2) == 1 )) {
            pthread_mutex_lock(&write_nw_mutex);
            strlcpy(ecosap_new_nw_config.dhcp_server_config.DHCP_From, (char *) ip1, sizeof(ecosap_new_nw_config.dhcp_server_config.DHCP_From)) ;
            strlcpy(ecosap_new_nw_config.dhcp_server_config.DHCP_To, (char *) ip2, sizeof(ecosap_new_nw_config.dhcp_server_config.DHCP_To)) ;
            ecosap_new_nw_config.dhcp_server_config.Lease_Time = ltime;
            SHELL_PRINT ("Configured Values are:\n") ;
            SHELL_PRINT ("Start IP %s\n", ecosap_new_nw_config.dhcp_server_config.DHCP_From ) ;
            SHELL_PRINT ("End IP %s\n", ecosap_new_nw_config.dhcp_server_config.DHCP_To ) ;
            SHELL_PRINT ("Time %lu (in seconds)\n", ecosap_new_nw_config.dhcp_server_config.Lease_Time) ;
            write_nw_flash();
            pthread_mutex_unlock(&write_nw_mutex);
        }
        else {
            SHELL_PRINT("IP Range Not Valid\n");
            return SHELL_INVALID_ARGUMENT;
        }
    }
    return SHELL_OK;
}


CMD_DECL(start_dhcpd) {
	if (argc == 1) {
		if (strcmp((const char *)argv[0], "enable") == 0)
			ecosap_new_nw_config.dhcp_server_config.dhcps_enable = 1;
		else if(strcmp((const char *)argv[0], "disable") == 0)
			ecosap_new_nw_config.dhcp_server_config.dhcps_enable = 0;
		else {
			SHELL_ERROR("Error:Invalid arguments\n");
			return SHELL_INVALID_ARGUMENT;
		}
		write_nw_flash();
	}
	else {
		SHELL_PRINT("setdhcps enable/disable\n");
	}
	return SHELL_OK;
}
#endif

CMD_DECL(route_add)
{
    char *IPAddr = NULL ;
    char *Mask = NULL;
    char *Gateway = NULL;
    int i ;

    if ( argc < 2 ) {
        route_add_print_usage() ;
        return SHELL_INVALID_ARGUMENT ;
    }

    /** responding to help request **/
    if ( (argc == 2 ) ) {
        route_add_print_usage() ;
        return SHELL_INVALID_ARGUMENT;
    }

    if ( argc < 7 ) {
        SHELL_PRINT ("Few arguments\n") ;
        SHELL_PRINT ("Usage :route add -I [IP Address] -M [Mask] -G [Gateway] \n") ;
        return SHELL_INVALID_ARGUMENT;
    } else if ( argc > 7 ) {
        SHELL_PRINT ("Too many arguments\n") ;
        SHELL_PRINT ("Usage :route add -I [IP Address] -M [Mask] -G [Gateway] \n") ;
        return SHELL_INVALID_ARGUMENT;
    } else {
        if(strcmp((const char *)argv[0],"add") != 0){
            SHELL_PRINT ("first arg should be add\n") ;
            return SHELL_INVALID_ARGUMENT;
        }
        for ( i = 1 ; i < 7 ;  i = i + 2) {
            if ( !strcmp ((const char *)argv[i ], "-I"  ) ) {
                if ( isvalidip((char *)argv[i + 1]) ) {
                    IPAddr = (char *)argv[i + 1];
                } else {
                    SHELL_PRINT ("IPAddress %s is invalid\n", argv[i + 1]) ;
                    return SHELL_INVALID_ARGUMENT;
                }

            } else if ( !strcmp ((const char *)argv[i], "-M"  ) ) {
                if ( isvalidip((char *)argv[i + 1]) ) {
                    Mask = (char *)argv[i + 1];
                } else {
                    SHELL_PRINT ("Mask %s is invalid\n", argv[i + 1]) ;
                    return SHELL_INVALID_ARGUMENT;
                }

            } else if ( !strcmp ((const char *)argv[i], "-G"  ) ) {
                if ( isvalidip((char *)argv[i + 1]) ) {
                    Gateway = (char *)argv[i + 1];
                } else {
                    SHELL_PRINT ("Gateway %s is invalid\n", argv[i + 1]) ;
                    return SHELL_INVALID_ARGUMENT;
                }

            } else {
                SHELL_PRINT ( "Unrecognized option\n" ) ;
                return SHELL_INVALID_ARGUMENT;
            }
        }
        SHELL_PRINT ("Configured Values are:\n") ;
        SHELL_PRINT ("IP Address : %s\n", IPAddr ) ;
        SHELL_PRINT ("Gateway 	 : %s\n", Gateway ) ;
        SHELL_PRINT ("Mask 	 : %s\n", Mask ) ;
    }
    SHELL_PRINT("\n");
    return SHELL_OK;
}
/*
   CMD_DECL(ping_cmd)
   {

   if(argc == 0 || argc >2)
   {
   SHELL_PRINT("Invalid ping command\n");
   return SHELL_INVALID_ARGUMENT;
   }

   if(argc == 1) {
   if(isvalidip((char *)argv[0])){
   struct protoent *p;
   struct timeval tv;
   struct sockaddr_in host;
   int s;

//Change for DNS
struct hostent *hent;

if ((p = getprotobyname("icmp")) == (struct protoent *)0) {
pexit("getprotobyname");
return -1;
}
//s = socket(AF_INET, SOCK_RAW, p->p_proto);
s = socket(AF_INET, SOCK_RAW, 1);
if (s < 0) {
pexit("socket");
return -1;
}
tv.tv_sec = 1;
tv.tv_usec = 0;
setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
// Set up host address
host.sin_family = AF_INET;
host.sin_len = sizeof(host);
host.sin_port = 0;
// Now try a bogus host

host.sin_addr.s_addr = inet_addr(argv[0]);
ping_host(s, &host);

}
else{
SHELL_PRINT("Usage: ping [ipaddress] (OR) Invalid IP address\n");
return SHELL_INVALID_ARGUMENT;
}
}

return SHELL_OK;
}
*/
CMD_DECL(network_tables)
{
    show_network_tables(diag_printf);
    return SHELL_OK;
}

#ifdef CYGPKG_COMPONENT_IPFW
#if 0
CMD_DECL(ipfw_func1)
{
	int i=0, k=0;
	memset((char **)nat_args, '\0', sizeof(nat_args));

	strlcpy(ipfw_args[0],"ipfw", MAX_IPFW_RULE_ARGS_SIZE);
	int r = 0;

	if(argc < 1){
		SHELL_ERROR("Few arguments\n");
		return SHELL_INVALID_ARGUMENT;
	}

	if(argc == 1){
		if(strcmp((const char *)argv[0] , "list")==0 || strcmp((const char *)argv[0] , "show")==0 ){
			SHELL_PRINT("\n Valid IPFW Commands\n");
			ecosap_nw_ipfw_rules_show(NULL);
		}
		else if(strcmp((const char *)argv[0] , "--help")==0 || strcmp((const char *)argv[0] , "-h")==0 ){
			ipfw_help_msg(NULL);
		}
		else if(strcmp((const char *)argv[0] , "zero")==0){
			strlcpy(ipfw_args[1], (const char *)argv[0], MAX_IPFW_RULE_ARGS_SIZE);
			if(do_ipfw_zero(argc+1 , ipfw_args, NULL)== -1){
				SHELL_PRINT("ipfw zero command failed\n");
			}
		}
		else{
			SHELL_PRINT("Invalid arguments\n");
		}
		return SHELL_OK;
	}
	if(strcmp((const char *)argv[0] , "zero")==0){
		for(i=0; i<argc; i++)
		{
			strlcpy(ipfw_args[i+1], (const char *)argv[i], MAX_IPFW_RULE_ARGS_SIZE);
		}
		if(do_ipfw_zero(argc+1 , ipfw_args, NULL)== -1){
			SHELL_PRINT("ipfw zero command failed\n");
		}
		return SHELL_OK;
	}
	if(argc == 2){
		if(strcmp((const char *)argv[0] , "delete")==0 ){
			strlcpy(ipfw_args[1], "delete",MAX_IPFW_RULE_ARGS_SIZE);
			if(isvalid_ipfw_digit(argv[1])){
				strlcpy(ipfw_args[2], (const char *)argv[1], MAX_IPFW_RULE_ARGS_SIZE);
				if ( ecosap_nw_ipfw_rules_config(argc+1 , ipfw_args, NULL) == -1 )
				{
					SHELL_PRINT ("IPFW rule delete failed\n");
					return -1;
				}
				delete_rule_from_flash(argv[1]);
				ecosap_config_struct.is_flashrule_full = 0;
				write_nw_flash();
				return SHELL_OK;
			}

		}
		else if(strcmp((const char *)argv[0] , "flush")==0 ){
			for(i=0; i<argc; i++)
			{
				strlcpy(ipfw_args[i+1], (const char *)argv[i], MAX_IPFW_RULE_ARGS_SIZE);
			}
			if(do_flush_action(argc+1 , ipfw_args, NULL) ==-1){
				SHELL_PRINT("Invalid Flush rule\n");
				return -1;
			}
			return SHELL_OK;
		}
		else{
			SHELL_PRINT("Invalid arguments 2\n");
			return SHELL_INVALID_ARGUMENT;
		}
	}
	SHELL_PRINT("\n");

	if(strcmp((const char *)argv[0], "add") == 0){
		for(i=0; i<argc; i++)
		{
			strlcpy(ipfw_args[i+1], (const char *)argv[i], MAX_IPFW_RULE_ARGS_SIZE);	
		}
		SHELL_PRINT("String : ");
		for(i=0; i<argc+1; i++)
		{
			SHELL_PRINT("%s ", ipfw_args[i]);	
		}
		if(isvalid_ipfw_cmd(ipfw_args) == 1) {
			//TODO : call ipfw_main()
			if ( ecosap_nw_ipfw_rules_config(argc+1 , ipfw_args, NULL) == -1 )
			{
				SHELL_PRINT ("IPFW rule configuration failed\n");
				return SHELL_INVALID_ARGUMENT;	
			}
			pthread_mutex_lock(&write_nw_mutex);
			if(ecosap_config_struct.is_flashrule_full == 0) {
				for(k=0; k<5; k++){
					if(ecosap_config_struct.ipfw_config[k].is_deleted == 1){
						memset(ecosap_config_struct.ipfw_config[k].ipfw_rules, '\0', sizeof(IPFW_CONFIG_STRUCT)) ;
						memcpy(ecosap_config_struct.ipfw_config[k].ipfw_rules, ipfw_args, sizeof(ipfw_args)) ;
						memset(ecosap_new_nw_config.ipfw_config[k].ipfw_rules, '\0', sizeof(IPFW_CONFIG_STRUCT)) ;
						memcpy(ecosap_new_nw_config.ipfw_config[k].ipfw_rules, ipfw_args, sizeof(ipfw_args)) ;

						for(i=argc+1; i<16; i++){
							memset(ecosap_config_struct.ipfw_config[k].ipfw_rules[i], '\0', 24);
							memset(ecosap_new_nw_config.ipfw_config[k].ipfw_rules[i], '\0', 24);
						}
						ecosap_config_struct.ipfw_config[k].ipfw_argc = argc+1;
						ecosap_new_nw_config.ipfw_config[k].ipfw_argc = argc+1;
						if(k == 4){
							ecosap_config_struct.is_flashrule_full = 1;
							ecosap_new_nw_config.is_flashrule_full = 1;
						}
						ecosap_config_struct.ipfw_config[k].is_deleted = 0;
						ecosap_new_nw_config.ipfw_config[k].is_deleted = 0;
						write_nw_flash();
						break;
					}
				}
			}
			if(ecosap_config_struct.is_flashrule_full == 1){
				SHELL_PRINT("EXCEEDED FLASH LIMIT FOR IPFW\n");
			}
			pthread_mutex_unlock(&write_nw_mutex);
		}	
	}
	else if(strcmp((const char *)argv[0], "nat") == 0){

		strlcpy(ipfw_args[1], "nat", MAX_IPFW_RULE_ARGS_SIZE);
		if(argc == 5){
			strlcpy(nat_args[0], "ipfw", MAX_IPFW_RULE_ARGS_SIZE);
			strlcpy(ipfw_args[0], "ipfw", MAX_IPFW_RULE_ARGS_SIZE);
			for(i=1; i<6; i++){
				strlcpy(ipfw_args[i], (const char *)argv[i-1], MAX_IPFW_RULE_ARGS_SIZE);
				strlcpy(nat_args[i], (const char *)argv[i-1], MAX_IPFW_RULE_ARGS_SIZE);
			}
			/*NAT command validation*/
			if(is_valid_nat_config(ipfw_args) == 1) {
				if ( ecosap_nw_nat_config(0, 6, nat_args, NULL) == -1 )
				{
					SHELL_PRINT ( " NAT config configuration failed\n");
					return SHELL_INVALID_ARGUMENT;
				}
				SHELL_PRINT("Valid NAT Redirect Commands\n");
				pthread_mutex_lock(&write_nw_mutex);
				if(ecosap_config_struct.is_flashrule_full == 0) {
					for(k=0; k<5; k++){
						if(ecosap_config_struct.ipfw_config[k].is_deleted == 1){ 
							memset(ecosap_config_struct.ipfw_config[k].ipfw_rules, '\0', sizeof(IPFW_CONFIG_STRUCT)) ;
							memcpy(ecosap_config_struct.ipfw_config[k].ipfw_rules, nat_args, sizeof(nat_args)) ;
							memset(ecosap_new_nw_config.ipfw_config[k].ipfw_rules, '\0', sizeof(IPFW_CONFIG_STRUCT)) ;
							memcpy(ecosap_new_nw_config.ipfw_config[k].ipfw_rules, nat_args, sizeof(nat_args)) ;
							for(i=6; i<16; i++){
								memset(ecosap_config_struct.ipfw_config[k].ipfw_rules[i], '\0', 24);
								memset(ecosap_new_nw_config.ipfw_config[k].ipfw_rules[i], '\0', 24);
							}
							ecosap_config_struct.ipfw_config[k].ipfw_argc = 6;	
							ecosap_new_nw_config.ipfw_config[k].ipfw_argc = 6;	
							if(k == 4){
								ecosap_config_struct.is_flashrule_full = 1;
								ecosap_new_nw_config.is_flashrule_full = 1;
							}
							ecosap_config_struct.ipfw_config[k].is_deleted = 0;
							ecosap_new_nw_config.ipfw_config[k].is_deleted = 0;
							write_nw_flash();
							break;
						}
					}
				}
				if(ecosap_config_struct.is_flashrule_full == 1){
					SHELL_PRINT("EXCEEDED FLASH LIMIT FOR IPFW\n");
				}
				pthread_mutex_unlock(&write_nw_mutex);
				return SHELL_OK;

			}
			else {
				SHELL_PRINT("Invalid digits\n");
				return SHELL_INVALID_ARGUMENT;
			}
		}

		if(argc == 3){
			if(strcmp(argv[1], "delete")==0 ){
				strlcpy(ipfw_args[2], "delete", MAX_IPFW_RULE_ARGS_SIZE);
				if(isvalid_ipfw_digit(argv[2])){
					strlcpy(ipfw_args[3], (const char *)argv[2], MAX_IPFW_RULE_ARGS_SIZE);

					if ( ecosap_nw_ipfw_rules_config(argc+1 , ipfw_args, NULL) == -1 )
					{
						SHELL_PRINT ("IPFW rule delete failed\n");
						return -1;
					}
					delete_nat_from_flash(argv[2]);
					ecosap_config_struct.is_flashrule_full = 0;
					ecosap_new_nw_config.is_flashrule_full = 0;
					write_nw_flash();
					return SHELL_OK;

				}else {
					SHELL_PRINT("Invalid nat : id\n");
					return SHELL_INVALID_ARGUMENT;
				}

			}else {
				SHELL_PRINT("Invalid nat : delete command\n");
				return SHELL_INVALID_ARGUMENT;
			}
			SHELL_PRINT("show rule : ");
			for(i=0; i<argc+1; i++)
			{
				SHELL_PRINT("%s ", ipfw_args[i]);
			}
			SHELL_PRINT("\n");
			ecosap_nw_ipfw_rules_show(NULL);

			return SHELL_OK;
		}

		if(argc < 9){
			SHELL_PRINT("Few arguments in NAT\n");
			return SHELL_INVALID_ARGUMENT;
		}
		if(argc > 9){
			SHELL_PRINT("More arguments in NAT\n");
			return SHELL_INVALID_ARGUMENT;
		}

		strlcpy(nat_args[0], "ipfw", MAX_IPFW_RULE_ARGS_SIZE);
		strlcpy(ipfw_args[0], "ipfw", MAX_IPFW_RULE_ARGS_SIZE);
		for(i=1; i<10; i++){
			strlcpy(ipfw_args[i], (const char *)argv[i-1], MAX_IPFW_RULE_ARGS_SIZE);
			strlcpy(nat_args[i], (const char *)argv[i-1], MAX_IPFW_RULE_ARGS_SIZE);
		}
		/*NAT command validation*/
		if(is_valid_nat_cmd(ipfw_args) == 1) {
			SHELL_PRINT("\n Valid NAT Redirect Commands\n");
			for(i=0; i<10; i++)
			{
				SHELL_PRINT("%s ", nat_args[i]);
			}
			SHELL_PRINT("\n");
			if ( ecosap_nw_nat_config(0, 10, nat_args, NULL) == -1 )
			{
				SHELL_PRINT ( " NAT redirect configuration failed\n");
				return SHELL_OK;
			}

			pthread_mutex_lock(&write_nw_mutex);
			if(ecosap_config_struct.is_flashrule_full == 0) {
				for(k=0; k<5; k++){
					if(ecosap_config_struct.ipfw_config[k].is_deleted == 1){ 
						memset(ecosap_config_struct.ipfw_config[k].ipfw_rules, '\0', sizeof(IPFW_CONFIG_STRUCT)) ;
						memcpy(ecosap_config_struct.ipfw_config[k].ipfw_rules, nat_args, sizeof(nat_args)) ;
						memset(ecosap_new_nw_config.ipfw_config[k].ipfw_rules, '\0', sizeof(IPFW_CONFIG_STRUCT)) ;
						memcpy(ecosap_new_nw_config.ipfw_config[k].ipfw_rules, nat_args, sizeof(nat_args)) ;
						for(i=10; i<16; i++){
							memset(ecosap_config_struct.ipfw_config[k].ipfw_rules[i], '\0', 24);
							memset(ecosap_new_nw_config.ipfw_config[k].ipfw_rules[i], '\0', 24);
						}
						ecosap_config_struct.ipfw_config[k].ipfw_argc = 10;	
						ecosap_new_nw_config.ipfw_config[k].ipfw_argc = 10;	
						if(k == 4){
							ecosap_config_struct.is_flashrule_full = 1;
							ecosap_new_nw_config.is_flashrule_full = 1;
						}
						ecosap_config_struct.ipfw_config[k].is_deleted = 0;
						ecosap_new_nw_config.ipfw_config[k].is_deleted = 0;
						write_nw_flash();
						break;
					}
				}
			}
			if(ecosap_config_struct.is_flashrule_full == 1){
				SHELL_PRINT("EXCEEDED FLASH LIMIT FOR IPFW\n");
			}
			pthread_mutex_unlock(&write_nw_mutex);
		}	
	}
	else {
		SHELL_PRINT("Invalid Commands\n");
		return SHELL_INVALID_ARGUMENT;
	}
	SHELL_PRINT("END ecosap_nw_ipfw_rules_config\n");
	return SHELL_OK;
}
#endif 
#endif //ipfw component

/* Command to enable ARP packet prints */
unsigned char arp_debug;
CMD_DECL(arp_dump)                                                              
{
	arp_debug = 1;
    return SHELL_OK;                                                            
}
      
#ifdef CYGPKG_NET_DMZ
CMD_DECL(dmz_config)
{
	char dmz_host[IPADDR_SIZE];
	int from_range = 0;
	int to_range = 0;
	char from[IPADDR_SIZE] = {0};
	char to[IPADDR_SIZE] = {0};
	strlcpy(from, ecosap_config_struct.dhcp_server_config.DHCP_From, IPADDR_SIZE);
	strlcpy(to, ecosap_config_struct.dhcp_server_config.DHCP_To, IPADDR_SIZE);
	if(argc == 1){
		if(strncmp("disable", (const char *)argv[0], strlen((const char *)argv[0])) == 0){
			if(ecosap_new_nw_config.dmz_config.status == 1){
				pthread_mutex_lock(&write_nw_mutex);
				ecosap_new_nw_config.dmz_config.status = 0;
				write_nw_flash();
				pthread_mutex_unlock(&write_nw_mutex);
				if(disable_dmz_ipfw_rule()!= 0 )
				{
					dmz_error();
					return -1;
				}
			}
			else{
				SHELL_PRINT("DMZ is already disable\n");
			}
		}
		else {
			SHELL_USAGE("Usage:dmz enable [IP Address]\n\tdmz disable\n");
		}
	}
	else if(argc == 2){
		if(strncmp("enable", (const char *)argv[0], strlen((const char *)argv[0])) == 0){
			if(isvalidip((char *)argv[1])){
				strlcpy(dmz_host, (char *)argv[1], IPADDR_SIZE);
				from_range = isvalidrange(from, dmz_host);
				to_range   = isvalidrange(dmz_host, to);	
			}
			else{
				SHELL_PRINT("Invalid IP Address\n");
				return SHELL_INVALID_ARGUMENT;
			}
			if(from_range && to_range){
					pthread_mutex_lock(&write_nw_mutex);
					strlcpy(ecosap_new_nw_config.dmz_config.host, dmz_host, IPADDR_SIZE);
					ecosap_new_nw_config.dmz_config.status = 1;
					write_nw_flash();
					pthread_mutex_unlock(&write_nw_mutex);
					if(enable_dmz_ipfw_rule()!= 0 )
                			{
                        			dmz_error();
						return -1;
                			}
			}
			else {
				SHELL_ERROR("ERROR:DMZ Host not in same LAN/WLAN DHCP pool\n");
			}
		}
		else {
			SHELL_USAGE("Usage:dmz enable [IP Address]\n\tdmz disable\n");
		}
	}
	else {
		SHELL_USAGE("Usage:dmz enable [IP Address]\n\tdmz disable\n");
	}

	return SHELL_OK;
}

#endif

#if defined(CYGPKG_COMPONENT_IGMPPROXY) && defined(CYGPKG_COMPONENT_IGMPSNOOP)

CMD_DECL(setigmpconfig)
{
    if (argc != 1)
        goto err_invalid_args;

    if (strcmp((const char *)argv[0], "enable") == 0) {
        if (ecosap_config_struct.igmp_config.status & IGMP_ENABLE) {
            NET_INFO("IGMP feature is already enabled\n");
        } else {
            ecosap_new_nw_config.igmp_config.status = IGMP_ENABLE;
            write_nw_flash();
            NET_INFO("Reboot system to enable IGMP feature\n");
        }
    } else if (strcmp((const char *)argv[0], "disable") == 0) {
        if (ecosap_config_struct.igmp_config.status & IGMP_ENABLE) {
            ecosap_new_nw_config.igmp_config.status = IGMP_DISABLE;
            write_nw_flash();
            NET_INFO("Reboot system to disable IGMP feature\n");
        } else {
            NET_INFO("IGMP feature is already disabled\n");
        }
    } else {
        goto err_invalid_args;
    }

    return SHELL_OK;

err_invalid_args:
    NET_ERROR("Error:Usage setigmpconfig enable/disable\n");
    return SHELL_INVALID_ARGUMENT;
}
#endif /* CYGPKG_COMPONENT_IGMPPROXY && CYGPKG_COMPONENT_IGMPSNOOP */
#ifdef CYGPKG_COMPONENT_IPFW

int isvalid_fromto_ips(char *from, char *to) 
{
	if( !( ( isvalidip(from) || 
		(strcmp((const char *)from, "any") == 0) ||
		isvalid_ip_mask(from) ) &&
	       ( isvalidip(to) || 
		(strcmp((const char *)to, "any") == 0) ||
		isvalid_ip_mask(to) ) ) ) {
		SHELL_ERROR("Error:Invalid IP Address\n");
		return 0;
	}
	return 1;
}

CMD_DECL(ipfw_func)
{
	int i=0, is_valid_nat = 0;
	char temp[SUBNET_ADDR_SIZE] = {0};
	unsigned int ipfw_argc = argc + 1;
	
	strlcpy(ipfw_args[0],"ipfw", MAX_IPFW_RULE_ARGS_SIZE);

	if(ipfw_argc < MIN_CLI_HTTP_IPFW_ARGC || ipfw_argc > MAX_CLI_HTTP_IPFW_ARGC) {
		SHELL_ERROR("Error:Wrong arguments\n");
		return SHELL_INVALID_ARGUMENT;
	}

	for(i=1; i < ipfw_argc; i++) {
		strlcpy(ipfw_args[i], (const char *)argv[i-1], MAX_IPFW_RULE_ARGS_SIZE);
	}

	if(strcmp((const char *)ipfw_args[1], "add") == 0) {
		if( ( ipfw_argc > 8 ) && ( strcmp((const char *)ipfw_args[3], "nat") != 0 ) ) {
			if (!isvalid_fromto_ips(ipfw_args[6], ipfw_args[8]))
				return SHELL_INVALID_ARGUMENT;
		}
		else if ( strcmp((const char *)ipfw_args[3], "nat") == 0 ) {
			if( !isvalid_digit(ipfw_args[2]) || 
				!isvalid_digit(ipfw_args[4]) ) {
				SHELL_PRINT("Invalid IPFW/NAT ID\n");
                                return SHELL_INVALID_ARGUMENT;
			}
			if (!isvalid_fromto_ips(ipfw_args[7], ipfw_args[9]))
				return SHELL_INVALID_ARGUMENT;
		}
		#if 0
		SHELL_PRINT("\nBefore ipfw_main printing Rule:\n");
		for(i = 0; i < ipfw_argc; i++) {
			SHELL_PRINT("%s ", ipfw_args[i]);
		}
		SHELL_PRINT("\n");
		#endif
		if(ecosap_nw_ipfw_rules_config(ipfw_argc , ipfw_args, NULL) == -1) {
			SHELL_ERROR("Error:IPFW rule configuration failed\n");
			return SHELL_INVALID_ARGUMENT;	
		}
		else if(ipfw_argc > 8) {
			write_ipfw_rules_in_flash(ipfw_argc, ipfw_args, NULL);
			return SHELL_OK;
		}
		else 
			return SHELL_OK;
	}
	else if(strcmp((const char *)ipfw_args[1], "zero") == 0) {
		if(do_ipfw_zero(ipfw_argc , ipfw_args, NULL)== -1) {
			SHELL_ERROR("Error:ipfw zero command failed\n");
			return SHELL_INVALID_ARGUMENT;
		}
		return SHELL_OK;
	}
	else if( !strcmp((const char *)ipfw_args[1], "delete") 
				&& isvalid_digit((char *)ipfw_args[2])) {
		if ( ecosap_nw_ipfw_rules_config(ipfw_argc , ipfw_args, NULL) == -1 ) {
			return SHELL_INVALID_ARGUMENT;
		}
		delete_rule_from_flash((char *)argv[1]);
		cli_http_rule_deletion();
		SHELL_PRINT("Successfully rule deleted\n");
		return SHELL_OK;
	}
	else if(strcmp((const char *)ipfw_args[1] , "flush")==0 ) {
		if(do_flush_action(ipfw_argc , ipfw_args, NULL) == -1) {
			SHELL_ERROR("Error:Invalid Flush rule\n");
			return SHELL_INVALID_ARGUMENT;
		}
		return SHELL_OK;
	}
	else if((strcmp((const char *)ipfw_args[1], "show") == 0) || 
			(strcmp((const char *)ipfw_args[1], "list") == 0)) {
		ecosap_nw_ipfw_rules_show(NULL);
		return SHELL_OK;
	}
	else if(strcmp((const char *)ipfw_args[1], "nat") == 0){
		if(ipfw_argc > 6){
			if(strcmp((const char *)ipfw_args[4], "redirect_port")==0 ) { 
				strlcpy(temp, ipfw_args[6], MAX_IPFW_RULE_ARGS_SIZE);
				if( isvalidip_port(temp) ) {
					is_valid_nat = 1;
				}
				else { 
					is_valid_nat = 0;
				}
			}
			else if((strcmp((const char *)ipfw_args[4], "redirect_addr") == 0) || 
					(strcmp((const char *)ipfw_args[6], "redirect_addr") == 0)) {
				if((isvalidip(ipfw_args[5]) && isvalidip(ipfw_args[6])) ||
					(isvalidip(ipfw_args[7]) && isvalidip(ipfw_args[8]))) {
					is_valid_nat = 1;
				}
				else {
					is_valid_nat = 0;
				}
			}
			if(is_valid_nat == 0) {
				SHELL_ERROR("Invalid NAT Redirect command\n");
				return SHELL_INVALID_ARGUMENT;
			}
		}
		if((ipfw_argc >= 6) && (ecosap_nw_nat_config(0, ipfw_argc, ipfw_args, NULL) == -1)) {
			SHELL_ERROR ( "Error:NAT config configuration failed\n");
			return SHELL_INVALID_ARGUMENT;
		}else if(ipfw_argc >= 6) {
			write_ipfw_rules_in_flash(ipfw_argc, ipfw_args, NULL);
			return SHELL_OK;
		}
		if( (ipfw_argc == NAT_DELETE_ARGC) && !strcmp((const char *)ipfw_args[2], "delete") 
							&& isvalid_digit((char *)ipfw_args[3])) {
			if ( ecosap_nw_ipfw_rules_config(argc+1 , ipfw_args, NULL) == -1 ) {
				return -1;
			}
			delete_nat_from_flash(ipfw_args[3]);
			cli_http_rule_deletion();
			SHELL_PRINT("Successfully rule deleted\n");
			return SHELL_OK;
		}
	}
	SHELL_PRINT("This command not accepted from CLI\n");
	return SHELL_OK;
}
#endif

#ifdef NET_PKT_DEBUG
shell_cmd("netdebug",
                "To set NET Debug",
                USAGESTR("netdebug [enable/disable]"),
                netdebug);

CMD_DECL(netdebug)
{
	if(argc == 1) {
		if(!strcmp(argv[0], "enable")) {
			SHELL_PRINT("Net Debug enabled\n");
			network_debug = 1;
		} else if(!strcmp(argv[0], "disable")) {
			SHELL_PRINT("Net Debug disabled\n");
			network_debug = 0;
		} else {
			SHELL_ERROR("Invalid argument\n");
		}
	} else {
		SHELL_ERROR("Invalid argument\n");
	}
	return SHELL_OK;
}
#endif
