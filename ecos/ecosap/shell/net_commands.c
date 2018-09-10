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

#include "shell.h"
#include "misc.h"
#include "shell_err.h"
#include "shell_thread.h"
#include "commands.h"
#include "debug.h"

/** TODO: Move these commands to separate file */
#include <ecosap_nw_http.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pkgconf/system.h>
#include "ping.h"

/** TODO: Move these commands to separate file */


#ifdef CYGPKG_COMPONENT_IPTV
shell_cmd("setiptvconfig",
		"To set IPTV configuration",
		USAGESTR("Usage : setiptvconfig enable [VALUE b/w 4 - 4096] [access/trunk] Usage : setiptvconfig disable"),
		setiptvconfig);
#endif

shell_cmd("setwanconfig",
		"To set WAN configuration",
		USAGESTR("use -h for more info"),
		set_wan_config);

shell_cmd("wanstatus",
		"shows the WAN status",
		USAGESTR("wanstatus"),
		showWANstatus);

shell_cmd("setlanconfig",
		"To set IP to bridge",
		USAGESTR("setlanconfig [IP Address]"),
		set_lan_config);

shell_cmd("lanstatus",
		"shows the lan status",
		USAGESTR("lanstatus"),
		showLANstatus);

#ifdef CYGPKG_COMPONENT_DHCPS
shell_cmd("setdhcpsconfig",
		"To set DHCP Server configuration",
		USAGESTR("setdhcpsconfig -S [IP Address1] -E [IP Address2] -T [Time in seconds]"),
		dhcp_serv_config);

shell_cmd("setdhcps",
		"To Enable/Disable DHCP Server",
		USAGESTR("setdhcps [enable/disable]"),
		start_dhcpd);
#endif//DHCPS

shell_cmd("route",
		"show / manipulate the IP routing table",
		USAGESTR("route add -I [IP Address] -M [Mask] -G [Gateway]"),
		route_add);
/*
shell_cmd("ping",
		"send ICMP ECHO_REQUEST to network hosts",
		"ping [IP address]",
		ping_cmd);
*/
shell_cmd("showroute",
		"show / manipulate the IP routing table",
		USAGESTR("showroute"),
		network_tables);

#ifdef CYGPKG_COMPONENT_IPFW
shell_cmd("ipfw",
		"administration tool for IPv4 packet firewall",
		USAGESTR("ipfw add/nat [arg 1] [arg 2] ... [arg N]"),
		ipfw_func);
#endif//IPFW

shell_cmd("showconfig",
		"Displays the Network Configuration",
		USAGESTR("showconfig"),
		show_config);

shell_cmd("make_default",
		"Setting Default values to Network Configuration",
		USAGESTR("make_default"),
		make_default);

shell_cmd("wflash",
		"Setting current values to Network Configuration",
		USAGESTR("wflash"),
		wflash);

#ifdef CYGPKG_COMPONENT_IPFW
shell_cmd("vpnpt",
		"VPN Pass Through",
		USAGESTR("vpnpt [pptp/l2tp/ipsec] [enable/disable]"),
		vpn_passthrough);
#endif//IPFW

#ifdef CYGPKG_COMPONENT_BRIDGE
shell_cmd("brconfig",
	 "To set bridge configuration",
	 USAGESTR("brconfig"),
	 brconfig);

shell_cmd("brinfo",
	 "Shows bridge information",
	 USAGESTR("brinfo"),
	 brinfo);
#endif //BRIDGE

shell_cmd("ethreg",
	"ethreg tool",
	USAGESTR("ethreg"),
	ethreg);

CMD_DECL(ethreg){
	return ethreg_start(argc, argv);
}

shell_cmd("arp_dump",
	"Dump ARP packets",
	USAGESTR("arp_dump"),
	arp_dump);

#ifdef CYGPKG_NET_DMZ
shell_cmd("dmz",
	"DMZ Configuration",
	USAGESTR("dmz enable [IP Address] (or) dmz disable "),
	dmz_config);
#endif

#if defined(CYGPKG_COMPONENT_IGMPPROXY) && defined(CYGPKG_COMPONENT_IGMPSNOOP)
shell_cmd("setigmpconfig",
          "To control igmp proxy and snooping",
          "Usage : setigmpconfig enable/disable",
          setigmpconfig);
#endif /* CYGPKG_COMPONENT_IGMPPROXY && CYGPKG_COMPONENT_IGMPSNOOP */
