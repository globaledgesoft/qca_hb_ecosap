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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#include <set_lan_wan_util.h>
#include <ecosap_nw_http.h>
#include <pkgconf/system.h>
#include "debug.h"

/**function to lan print command usage message **/
void set_lan_print_usage(void)
{
	SHELL_USAGE("Usage:setlanconfig [IP Address]\n") ;
}


/**function to wan print command usage message **/
void set_wan_print_usage(void)
{
	SHELL_USAGE("<setwanconfig 0> -> To Assign DHCP Client\n") ;

#ifdef CYGPKG_COMPONENT_PPPoE
	SHELL_USAGE("<setwanconfig 1> -> To Assign PPPoE\n") ;
#endif
	SHELL_USAGE("<setwanconfig 2> -> To Assign Static\n") ;

#ifdef CYGPKG_COMPONENT_PPPoE
	SHELL_USAGE("Usage:setwanconfig 1 -U [PPPoE Username] -P [PPPoE Password] -M [MTU]\n") ;
#endif
	SHELL_USAGE("Usage:setwanconfig 2 -I [IP Address] -G [Gateway] -M [Mask] -D1 [DNS] -D2 [ADNS] -S [Server Address]\n") ;
}

/**function to route add print command usage message **/
void route_add_print_usage(void)
{
	SHELL_USAGE("Usage:route add -I [IP Address] -M [NetMask] -G [Gateway]\n") ;
}

/**function to dhcp print command usage message **/
void dhcp_print_usage(void)
{
	SHELL_USAGE("Usage:setdhcpsconfig -S [IP Address1] -E [IP Address2] -T [Time]\n") ;
}

/*function to print vpnpt usage*/
void vpnpt_print_usage(void)
{
	SHELL_USAGE("Usage:vpnpt [pptp/l2tp/ipsec] [enable/disable]\n") ;
}

#ifdef CYGPKG_COMPONENT_IPTV
void vlan_print_usage(void){
	SHELL_USAGE("Usage:setiptvconfig enable [VALUE b/w 4 - 4096] [access/trunk]\nUsage : setvlan disable\n");
}
#endif

int isvalidTime(unsigned int x){

	if (x < 400000 && x > 0)
		return 1;
	else 
		return 0;
}

