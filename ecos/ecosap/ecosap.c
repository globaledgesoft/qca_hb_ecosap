/*
    eCos AP main entry point to bring up the AP.
*/

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

#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/* Header file having debug macros */
#include <cyg/dbg_print/dbg_print.h>

#include "debug.h"
#include "ecosap.h"
#include "ecosap_nw_http.h"
#include "ecosap_nw_config.h"

#ifdef CYGPKG_COMPONENT_IGMPSNOOP
#include "igmp-snoop.h"
#endif

#ifdef CYGPKG_COMPONENT_IGMPPROXY
#include "igmp-proxy.h"
#endif

#define SHELL_DEBUG
#define VLAN 1

void ecosap_start(void);
void ecosap_stop(void);

extern void interface_down(const char *);
extern cyg_handle_t dhcp_mgt_thread_h;
volatile unsigned char eth0_flag;
extern int  eth0_up;

#ifdef CYGPKG_COMPONENT_PPPoE
extern int is_ppp_up;
#endif

extern int static_connection_up;
#ifdef CYGPKG_COMPONENT_DNS_CACHE
extern void dns_cache(void);
#endif

int main(void)
{
	cyg_thread_delay(100);
	ecosap_start();

	/** @TODO: Make this event driven to handle AP events. */
 	for (;;) {
		cyg_thread_delay(200);

		if (eth0_flag == 2) {
			NET_INFO("WAN ethernet link down...\n");
			if(((ecosap_config_struct.Connection_Type != PPPoE)
#ifdef CYGPKG_COMPONENT_PPPoE
			&& (!is_ppp_up)
#endif
			) || ((ecosap_config_struct.Connection_Type == PPPoE)
#ifdef CYGPKG_COMPONENT_PPPoE
			 && (is_ppp_up == -1)
#endif
			 )) {
				interface_down(WAN);
				eth0_flag = 0;
				eth0_up = 0;
			}

		} else if(eth0_flag == 1) {
			if(((ecosap_config_struct.Connection_Type != PPPoE)
#ifdef CYGPKG_COMPONENT_PPPoE
			&& (!is_ppp_up )
#endif
			) || ((ecosap_config_struct.Connection_Type == PPPoE)
#ifdef CYGPKG_COMPONENT_PPPoE
			&& (is_ppp_up == -1)
#endif
			 )) {
				NET_INFO("WAN ethernet link UP...\n");
				apwan_start();
				eth0_flag = 0;
			}
		}
	}
	reboot();
}

void reboot(void)
{
	/* need to handle system clean up than reboot */
	NET_INFO("Rebooting...");
	ecosap_stop();
}

void ecosap_start(void)
{
	ENTER();

    NET_INFO("Starting AP!\n");

#ifdef SHELL_DEBUG
	apcli_start();
#endif

	apconfig_start();

#if CYGPKG_COMPONENT_IGMPSNOOP
    if (ecosap_config_struct.igmp_config.status & IGMP_ENABLE) {
        igmp_snoop_init();
    }
#endif

#ifdef CYGPKG_COMPONENT_IPTV
	if(ecosap_config_struct.vlan_config.vlan_flag) {
		apvlan_start();
		NET_INFO("Enabled VLAN on LAN interface.......\n");
	}
#endif

	aplan_start();

#ifdef CYGPKG_COMPONENT_IPTV
    if(ecosap_config_struct.vlan_config.vlan_flag)
	    apvlan_wlan_start();
#endif

#ifdef CYGPKG_COMPONENT_IPTV
	if(ecosap_config_struct.vlan_config.vlan_flag) {
		//athr_vlan_enable_all();
		//vlanset(ecosap_config_struct.vlan_config.iptv_value);
		if(nw_set_port_mode()) {				//This function configures the vlan ID for IPTV and Internet ports
			NET_INFO("VLAN port configuration failed...\n");
		}
	}
#endif

#ifdef CYGPKG_COMPONENT_DHCPS
	if (ecosap_config_struct.dhcp_server_config.dhcps_enable)
		apdhcpd_start();
#endif

#ifdef CYGPKG_COMPONENT_HTTPD
	aphttpd_start();
#endif

#ifdef CYGPKG_COMPONENT_IPFW
	apipfw_start();
#endif

	apwan_start();

	show_nw_config();

#ifdef CYGPKG_COMPONENT_SNTP
	cyg_sntp_start();
#endif

#ifdef CYGPKG_COMPONENT_IGMPPROXY
    if (ecosap_config_struct.igmp_config.status & IGMP_ENABLE) {
        igmp_proxy_start();
    }
#endif

#ifdef CYGPKG_COMPONENT_DNS_CACHE
    dns_cache();
#endif
}

void ecosap_stop(void)
{
	EXIT();

    apwan_stop();

#ifdef CYGPKG_COMPONENT_IPFW
//	apipfw_stop();
#endif

#ifdef CYGPKG_COMPONENT_HTTPD
//	aphttpd_stop();
#endif

#ifdef CYGPKG_COMPONENT_DHCPS
//	apdhcpd_stop();
#endif

	aplan_stop();

	apconfig_stop();

#ifdef SHELL_DEBUG
	apcli_stop();
#endif

#ifdef CYGPKG_COMPONENT_IGMPPROXY
    if (ecosap_config_struct.igmp_config.status & IGMP_ENABLE) {
        igmp_proxy_stop();
    }
#endif

#if CYGPKG_COMPONENT_IGMPSNOOP
    if (ecosap_config_struct.igmp_config.status & IGMP_ENABLE) {
        igmp_snoop_deinit();
    }
#endif
}
