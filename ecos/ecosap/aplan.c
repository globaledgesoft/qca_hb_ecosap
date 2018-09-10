/* aplan.c

   Start and Stop of LAN API
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

#include <network.h>
#include <stdio.h>
#include <netinet/in.h>
#include "debug.h"

#include "ecosap_nw_config.h"
#include "ecosap_nw_http.h"


extern void start_bridge_interface (void);
extern int apwlan_init(void);

void aplan_start(void)
{
	struct in_addr ip_addr;        
	char *arg[3];

	ENTER();

#ifdef BRIDGE_SUPPORT
	start_bridge_interface();

	/* Add the default WLAN VAP interface to bridge */
	/* ToDo: Need to move this to proper place.
	   May be after all the network initialization ? */
#if defined(CYGPKG_COMPONENT_WLAN_CONFIG_TOOLS) && defined(CONFIG_WLAN_WLANCONFIG) && \
	defined(CYGPKG_COMPONENT_WLAN_WIRELESS_TOOLS) && defined(CONFIG_WLAN_IWCONFIG)
	apwlan_init();
#endif
#else
	/* Initialize LAN interfaces*/
	 start_nw_lan_interface();
#endif

    	ip_addr.s_addr = ecosap_config_struct.bridgeConf.inaddr_ip;
        //arg[0] = BRIDGE;
        arg[0] = Server_Int;
        arg[1] = inet_ntoa(ip_addr);
        ifconfig_cmd(2, arg);
} 

void aplan_stop(void) 
{
	ENTER();
}

#ifdef CYGPKG_COMPONENT_HTTPD
void aphttpd_start(void)
{
	ENTER();
	start_httpd_server();
}
#endif

void apdhcpd_start(void)
{
	ENTER();
	start_dhcp_server();
}        
