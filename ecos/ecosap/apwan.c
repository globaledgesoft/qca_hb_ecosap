/* Start and Stop of WAN API
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

#include	<stdio.h>
#include	"debug.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cyg/hal/drv_api.h>
#include <network.h>
#include <pkgconf/system.h>
#include <pkgconf/net.h>
#include <net/if_dl.h>
#include <net/if_var.h>
#include <net/if_types.h>

/* Header file having debug macros */
#include <cyg/dbg_print/dbg_print.h>

#define	S_BUFFER	100

extern void init_all_network_interfaces(void);

void get_wan_ip(void)
{
	struct ifreq ifr;
	const char * name = "eth0";
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	if(fd == -1){
		perror("sock");
		return -1;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strlcpy(ifr.ifr_name, name, IFNAMSIZ);

	ifr.ifr_flags |= (IFF_UP | IFF_RUNNING | IFF_BROADCAST );
	if(ioctl(fd, SIOCSIFFLAGS, &ifr)) {
		perror("SIOCSIFFLAGS");
		close(fd);
		return -1;
	}

	init_wan_interface();
	close (fd);

	return 0;
}

void apwan_start(void)
{
	ENTER();
	get_wan_ip();

} 

void apwan_stop(void)
{
	ENTER();
}
