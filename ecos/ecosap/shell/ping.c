//==========================================================================
//
//      ping.c
//
//      Simple test of PING (ICMP) and networking support
//
//==========================================================================
// ####BSDALTCOPYRIGHTBEGIN####                                             
// -------------------------------------------                              
// Portions of this software may have been derived from FreeBSD, OpenBSD,   
// or other sources, and if so are covered by the appropriate copyright     
// and license included herein.                                             
// -------------------------------------------                              
// ####BSDALTCOPYRIGHTEND####                                               
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    gthomas
// Contributors: gthomas, andrew.lunn@ascom.ch
// Date:         2000-01-10
// Purpose:      
// Description:  
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <stdlib.h>

#include <network.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pkgconf/system.h>
#include <pkgconf/net.h>
#include <shell.h>
#include <shell_err.h>
#include <commands.h>

#include "debug.h"

#ifndef CYGPKG_LIBC_STDIO
#define perror(s) diag_printf(#s ": %s\n", strerror(errno))
#endif

#define NUM_PINGS 50
#define MAX_PACKET 4096
#define MIN_PACKET   64
#define MAX_SEND   4000

#define PACKET_ADD  ((MAX_SEND - MIN_PACKET)/NUM_PINGS)
#define nPACKET_ADD  1 

#define UNIQUEID 0x1234

// Compute INET checksum
int
inet_cksum(u_short *addr, int len)
{
    register int nleft = len;
    register u_short *w = addr;
    register u_short answer;
    register u_int sum = 0;
    u_short odd_byte = 0;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while( nleft > 1 )  {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if( nleft == 1 ) {
        *(u_char *)(&odd_byte) = *(u_char *)w;
        sum += odd_byte;
    }

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0x0000ffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);                     /* add carry */
    answer = ~sum;                          /* truncate to 16 bits */
    return (answer);
}

static int
show_icmp(unsigned char *pkt, int len, 
          struct sockaddr_in *from, struct sockaddr_in *to)
{
    cyg_tick_count_t *tp, tv;
    struct ip *ip;
    struct icmp *icmp;

    tv = cyg_current_time();
    ip = (struct ip *)pkt;
    if ((len < sizeof(*ip)) || ip->ip_v != IPVERSION) {
        SHELL_ERROR("%s: Short packet or not IP! - Len: %d, Version: %d\n", 
                    inet_ntoa(from->sin_addr), len, ip->ip_v);
        return 0;
    }
    icmp = (struct icmp *)(pkt + sizeof(*ip));
    len -= (sizeof(*ip) + 8);
    tp = (cyg_tick_count_t *)&icmp->icmp_data;
    if (icmp->icmp_type != ICMP_ECHOREPLY) {
        SHELL_ERROR("%s: Invalid ICMP - type: %d\n", 
                    inet_ntoa(from->sin_addr), icmp->icmp_type);
        return 0;
    }
    if (icmp->icmp_id != UNIQUEID) {
        SHELL_ERROR("%s: ICMP received for wrong id - sent: %x, recvd: %x\n", 
                    inet_ntoa(from->sin_addr), UNIQUEID, icmp->icmp_id);
    }
    SHELL_PRINT("%4d bytes from %s: ", len, inet_ntoa(from->sin_addr));
    SHELL_PRINT("icmp_seq=%3d", icmp->icmp_seq);
    SHELL_PRINT(", time=%2dms\n", (int)(tv - *tp)*10);

    return (from->sin_addr.s_addr == to->sin_addr.s_addr);
}

static void
ping_host(int s, struct sockaddr_in *host)
{
	unsigned char *pkt1, *pkt2;
    struct icmp *icmp;
    int icmp_len = MIN_PACKET;
    int seq, ok_recv, bogus_recv;
    cyg_tick_count_t *tp;
    long *dp;
    struct sockaddr_in from;
    int i, len;
    socklen_t fromlen;

    ok_recv = 0;
    bogus_recv = 0;
	pkt1 = pkt2 = NULL;

	if ((pkt1 = malloc(MAX_PACKET)) == NULL ||
			(pkt2 = malloc(MAX_PACKET)) == NULL) {
		SHELL_ERROR("Error: Unable to allocate memory!");
		goto badexit;
	}

	icmp = (struct icmp *)pkt1;

    SHELL_PRINT("PING %s\n", inet_ntoa(host->sin_addr));

    for (seq = 0; seq < NUM_PINGS; seq++) {
        // Build ICMP packet
        icmp->icmp_type = ICMP_ECHO;
        icmp->icmp_code = 0;
        icmp->icmp_cksum = 0;
        icmp->icmp_seq = seq;
        icmp->icmp_id = 0x1234;

        // Set up ping data
        tp = (cyg_tick_count_t *)&icmp->icmp_data;
        *tp++ = cyg_current_time();
        dp = (long *)tp;
        for (i = sizeof(*tp); i < icmp_len; i += sizeof(*dp)) {
            *dp++ = i;
        }
        // Add checksum
        icmp->icmp_cksum = inet_cksum( (u_short *)icmp, icmp_len+8);
        // Send it off
        if (sendto(s, icmp, icmp_len+8, 0, (struct sockaddr *)host, sizeof(*host)) < 0) {
            perror("sendto");
            continue;
        }
		SHELL_PRINT("-> ");
        // Wait for a response
        fromlen = sizeof(from);
        len = recvfrom(s, pkt2, MAX_PACKET, 0, (struct sockaddr *)&from, &fromlen);
        if (len < 0) {
            perror("recvfrom");
            // icmp_len = MIN_PACKET - PACKET_ADD; // just in case - long routes
        } else {
            if (show_icmp(pkt2, len, &from, host)) {
                ok_recv++;
            } else {
                bogus_recv++;
            }
        }
    }

    SHELL_PRINT("Sent %d packets, received %d OK, %d bad\n", NUM_PINGS, ok_recv, bogus_recv);

badexit:
	if (pkt1)
		free(pkt1);
	if (pkt2)
		free(pkt2);
}

shell_cmd("ping",
     "ping network host",
     USAGESTR("ping <host>"),
     ping_cmd);

CMD_DECL(ping_cmd)
{
    struct protoent *p;
    struct timeval tv;
    struct sockaddr_in host;
    int s, ret;

	if (argc != 1) {
		SHELL_USAGE("Usage: ping <host>\n");
		return SHELL_INVALID_ARGUMENT;
	}

    // Set up host address
    host.sin_family = AF_INET;
    host.sin_len = sizeof(host);
    host.sin_port = 0;
    if (!inet_aton((char *)argv[0], &host.sin_addr)) {
		SHELL_ERROR("Invalid address!");
		return SHELL_INVALID_ARGUMENT;
	}

    if ((p = getprotobyname("icmp")) == (struct protoent *)0) {
        SHELL_ERROR("Error: getprotobyname\n");
        return SHELL_INVALID_ARGUMENT;
    }

    s = socket(AF_INET, SOCK_RAW, p->p_proto);
    if (s < 0) {
        SHELL_ERROR("Error: opening socket\n");
        return SHELL_INVALID_ARGUMENT;
    }

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (ret < 0) {
	close(s);
        return SHELL_INVALID_ARGUMENT;
    }

    ping_host(s, &host);

    close(s);

    return SHELL_OK;
}

