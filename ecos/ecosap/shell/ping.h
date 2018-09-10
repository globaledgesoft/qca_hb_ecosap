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

#include <stdio.h>
#include <stdlib.h>
#include <network.h>
#ifdef CYGPKG_NET_INET6
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#endif

#include <pkgconf/system.h>
#include <pkgconf/net.h>

#include <cyg/infra/testcase.h>

// Fill in the blanks if necessary
#ifndef TNR_OFF
# define TNR_OFF()
#endif
#ifndef TNR_ON
# define TNR_ON()
#endif
#ifndef TNR_INIT
# define TNR_INIT()
#endif
#ifndef TNR_PRINT_ACTIVITY
# define TNR_PRINT_ACTIVITY()
#endif

#define NUM_PINGS 16
#define MAX_PACKET 4096
#define MIN_PACKET   64
#define MAX_SEND   4000

#define PACKET_ADD  ((MAX_SEND - MIN_PACKET)/NUM_PINGS)
#define nPACKET_ADD  1 

unsigned char pkt1[MAX_PACKET], pkt2[MAX_PACKET];

#define UNIQUEID 0x1234
void ping_host(int s, struct sockaddr_in *host);
