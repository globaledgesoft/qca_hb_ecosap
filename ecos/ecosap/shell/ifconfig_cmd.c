/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static const char rcsid[] =
  "$FreeBSD: src/sbin/ifconfig/ifconfig.c,v 1.51.2.13 2001/08/20 18:38:41 brooks Exp $";
#endif /* not lint */

#define __ECOS
#define NO_IPX
#define __unused
#undef NS
#undef USE_IEEE80211
#undef USE_VLANS
#undef USE_IF_MEDIA
#undef _KERNEL
#define __FreeBSD_cc_version 300001


#define SET 1

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>

/* IP */
#define __FreeBSD__
#include <netinet/in.h>
#undef __FreeBSD__
#include <netinet/in_var.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef INET6
#include <netinet6/nd6.h>	/* Define ND6_INFINITE_LIFETIME */
#endif


#ifndef NO_IPX
/* IPX */
#define	IPXIP
#define IPTUNNEL
#include <netipx/ipx.h>
#include <netipx/ipx_if.h>
#endif


/* XNS */
#ifdef NS
#define	NSIP
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

/* OSI */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h> /*GES */

#include <stdio.h>
#include <stdlib.h>
#include <shell.h>
#include <shell_err.h>
#include <shell_thread.h>
#include <thread_cleanup.h>
#include <commands.h>
#include "ifconfig_internal.h"
#include <pkgconf/system.h>

#ifdef __EOCS
#include <network.h>
#include <sys/sysctl.h>
#include <cyg/infra/testcase.h>
#endif
#include <shell.h>
#include <shell_err.h>
#include <shell_thread.h>
#include "debug.h"
#include <cyg/dbg_print/dbg_print.h>

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);   //base is the input number system
extern void print_statistics(char *);

/* wrapper for KAME-special getnameinfo() */
#ifndef NI_WITHSCOPEID
#define	NI_WITHSCOPEID	0
#endif

#ifndef MIN
#define MIN(a, b)                ((a) < (b) ? a : b)
#endif /* MIN */

#define MAXHOSTNAMELEN       256  

extern size_t strlcpy(char *, const char *, size_t);
extern void init_getopt(void);

struct	ifreq		ifr, ridreq;
struct	ifaliasreq	addreq;
#ifdef INET6
struct	in6_ifreq	in6_ridreq;
struct	in6_aliasreq	in6_addreq = 
  { { 0 }, 
    { 0 }, 
    { 0 }, 
    { 0 }, 
    0, 
    { 0, 0, ND6_INFINITE_LIFETIME, ND6_INFINITE_LIFETIME } };
#endif
struct	sockaddr_in	netmask;

char	name[32];
int	flags;
int	metric;
int	mtu;
int	setaddr;
int	setipdst;
int	setmask;
int	doalias;
int	clearaddr;
int	newaddr = 1;
#ifdef INET6
static	int ip6lifetime;
#endif

struct	afswtch;

int supmedia = 0;
int listcloners = 0;

#ifdef INET6
char	addr_buf[MAXHOSTNAMELEN *2 + 1];	/*for getnameinfo()*/
#endif

int	ifconfig __P((int argc, char *const *argv, const struct afswtch *afp));
int		notealias __P((const char *, int, int, const struct afswtch *afp));
void	list_cloners __P((void));
static void	printb __P((const char *s, unsigned value, const char *bits));
void	rt_xaddrs __P((caddr_t, caddr_t, struct rt_addrinfo *));
void	status __P((const struct afswtch *afp, int addrcount,
		    struct sockaddr_dl *sdl, struct if_msghdr *ifm,
		    struct ifa_msghdr *ifam));
static void	usage __P((void));

#ifdef INET6
void	in6_fillscopeid __P((struct sockaddr_in6 *sin6));
int	prefix __P((void *, int));
static	char *sec2str __P((time_t));
int	explicit_prefix = 0;
#endif

typedef	int c_func __P((const char *cmd, int arg, int s, const struct afswtch *afp));
typedef	int c_func2 __P((const char *arg, const char *arg2, int s, const struct afswtch *afp));
c_func	setatphase, setatrange;
c_func	setifaddr, setifbroadaddr, setifdstaddr, setifnetmask;
#ifdef INET6
c_func	setifprefixlen;
c_func	setip6flags;
c_func  setip6pltime;
c_func  setip6vltime;
c_func2	setip6lifetime;
#endif
c_func	setifipdst;
c_func	setifflags, setifmetric, setifmtu, setiflladdr;

#define	NEXTARG		0xffffff
#define	NEXTARG2	0xfffffe

const
struct	cmd {
	const	char *c_name;
	int	c_parameter;		/* NEXTARG means next argv */
	int		(*c_func) __P((const char *, int, int, const struct afswtch *afp));
	int 	(*c_func2) __P((const char *, const char *, int, const struct afswtch *afp));
} cmds[] = {
	{ "up",		IFF_UP,		setifflags } ,
	{ "down",	-IFF_UP,	setifflags },
	{ "arp",	-IFF_NOARP,	setifflags },
	{ "-arp",	IFF_NOARP,	setifflags },
	{ "debug",	IFF_DEBUG,	setifflags },
	{ "-debug",	-IFF_DEBUG,	setifflags },
	{ "add",	IFF_UP,		notealias },
	{ "alias",	IFF_UP,		notealias },
	{ "-alias",	-IFF_UP,	notealias },
	{ "delete",	-IFF_UP,	notealias },
	{ "remove",	-IFF_UP,	notealias },
#ifdef notdef
#define	EN_SWABIPS	0x1000
	{ "swabips",	EN_SWABIPS,	setifflags },
	{ "-swabips",	-EN_SWABIPS,	setifflags },
#endif
	{ "netmask",	NEXTARG,	setifnetmask },
#ifdef INET6
	{ "prefixlen",	NEXTARG,	setifprefixlen },
	{ "anycast",	IN6_IFF_ANYCAST, setip6flags },
	{ "tentative",	IN6_IFF_TENTATIVE, setip6flags },
	{ "-tentative",	-IN6_IFF_TENTATIVE, setip6flags },
	{ "deprecated",	IN6_IFF_DEPRECATED, setip6flags },
	{ "-deprecated", -IN6_IFF_DEPRECATED, setip6flags },
	{ "autoconf",	IN6_IFF_AUTOCONF, setip6flags },
	{ "-autoconf",	-IN6_IFF_AUTOCONF, setip6flags },
	{ "pltime",     NEXTARG,        setip6pltime },
	{ "vltime",     NEXTARG,        setip6vltime },
#endif
	{ "metric",	NEXTARG,	setifmetric },
	{ "broadcast",	NEXTARG,	setifbroadaddr },
	{ "ipdst",	NEXTARG,	setifipdst },
	{ "link0",	IFF_LINK0,	setifflags },
	{ "-link0",	-IFF_LINK0,	setifflags },
	{ "link1",	IFF_LINK1,	setifflags },
	{ "-link1",	-IFF_LINK1,	setifflags },
	{ "link2",	IFF_LINK2,	setifflags },
	{ "-link2",	-IFF_LINK2,	setifflags },
#ifdef USE_IF_MEDIA
	{ "media",	NEXTARG,	setmedia },
	{ "mediaopt",	NEXTARG,	setmediaopt },
	{ "-mediaopt",	NEXTARG,	unsetmediaopt },
#endif
#ifdef USE_VLANS
	{ "vlan",	NEXTARG,	setvlantag },
	{ "vlandev",	NEXTARG,	setvlandev },
	{ "-vlandev",	NEXTARG,	unsetvlandev },
#endif
#ifdef USE_IEEE80211
	{ "ssid",	NEXTARG,	set80211ssid },
	{ "nwid",	NEXTARG,	set80211ssid },
	{ "stationname", NEXTARG,	set80211stationname },
	{ "station",	NEXTARG,	set80211stationname },	/* BSD/OS */
	{ "channel",	NEXTARG,	set80211channel },
	{ "authmode",	NEXTARG,	set80211authmode },
	{ "powersavemode", NEXTARG,	set80211powersavemode },
	{ "powersave",	1,		set80211powersave },
	{ "-powersave",	0,		set80211powersave },
	{ "powersavesleep", NEXTARG,	set80211powersavesleep },
	{ "wepmode",	NEXTARG,	set80211wepmode },
	{ "wep",	1,		set80211wep },
	{ "-wep",	0,		set80211wep },
	{ "weptxkey",	NEXTARG,	set80211weptxkey },
	{ "wepkey",	NEXTARG,	set80211wepkey },
	{ "nwkey",	NEXTARG,	set80211nwkey },	/* NetBSD */
	{ "-nwkey",	0,		set80211wep },		/* NetBSD */
#endif
	{ "normal",	-IFF_LINK0,	setifflags },
	{ "compress",	IFF_LINK0,	setifflags },
	{ "noicmp",	IFF_LINK1,	setifflags },
	{ "mtu",	NEXTARG,	setifmtu },
	{ "lladdr",	NEXTARG,	setiflladdr },
	{ 0,		0,		setifaddr },
	{ 0,		0,		setifdstaddr },
};

/*
 * XNS support liberally adapted from code written at the University of
 * Maryland principally by James O'Toole and Chris Torek.
 */
typedef	int af_status __P((int, struct rt_addrinfo *));
typedef	int af_getaddr __P((const char *, int));
typedef void af_getprefix __P((const char *, int));

af_status	in_status, at_status, ether_status;
af_getaddr	in_getaddr, at_getaddr, ether_getaddr;

#ifndef NO_IPX
af_status	ipx_status;
af_getaddr	ipx_getaddr;
#endif

#ifdef INET6
af_status	in6_status;
af_getaddr	in6_getaddr;
af_getprefix	in6_getprefix;
#endif /*INET6*/
#ifdef NS
af_status	xns_status;
af_getaddr	xns_getaddr;
#endif

/* Known address families */
const
struct	afswtch {
	const char *af_name;
	short af_af;
	af_status *af_status;
	af_getaddr *af_getaddr;
	af_getprefix *af_getprefix;
	u_long af_difaddr;
	u_long af_aifaddr;
	caddr_t af_ridreq;
	caddr_t af_addreq;
} afs[] = {
#define C(x) ((caddr_t) &x)
	{ "inet", AF_INET, in_status, in_getaddr, NULL,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq) },
#ifdef INET6
	{ "inet6", AF_INET6, in6_status, in6_getaddr, in6_getprefix,
	     SIOCDIFADDR_IN6, SIOCAIFADDR_IN6,
	     C(in6_ridreq), C(in6_addreq) },
#endif /*INET6*/
#ifndef NO_IPX
	{ "ipx", AF_IPX, ipx_status, ipx_getaddr, NULL,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq) },
#endif
#ifdef NS
	{ "ns", AF_NS, xns_status, xns_getaddr, NULL,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq) },
#endif
	{ "ether", AF_LINK, ether_status, ether_getaddr, NULL,
	     0, SIOCSIFLLADDR, NULL, C(ridreq) },
	{ 0,	0,	    0,		0 }
};

/*
 * Expand the compacted form of addresses as returned via the
 * configuration read via sysctl().
 */

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

void
rt_xaddrs(cp, cplim, rtinfo)
	caddr_t cp, cplim;
	struct rt_addrinfo *rtinfo;
{
	struct sockaddr *sa;
	int i;

	memset(rtinfo->rti_info, 0, sizeof(rtinfo->rti_info));
	for (i = 0; (i < RTAX_MAX) && (cp < cplim); i++) {
		if ((rtinfo->rti_addrs & (1 << i)) == 0)
			continue;
		rtinfo->rti_info[i] = sa = (struct sockaddr *)cp;
		ADVANCE(cp, sa);
	}
}

#ifndef CYGPKG_COMPONENT_BRIDGE
/* TODO: Verify if this is required or the one in the bridge */
static char *
_ether_aton(const char* s, struct ether_addr* e)
{
    int i;
    long l;
    char *pp;

    while (isspace(*s))
        s++;

    /* expect 6 hex octets separated by ':' or space/NUL if last octet */
    for (i = 0; i < 6; i++) {
        l = strtol(s, &pp, 16);
        if (pp == s || l > 0xFF || l < 0)
            return (NULL);
        if (!(*pp == ':' || (i == 5 && (isspace(*pp) || *pp == '\0'))))
            return (NULL);
        e->octet[i] = (u_char)l;
        s = pp + 1;
    }

    /* return character after the octets ala strtol(3) */
    return (pp);
}

struct ether_addr *
ether_aton(const char* s)
{
    static struct ether_addr n;

    return (_ether_aton(s, &n) ? &n : NULL);
}

#endif

static void
usage()
{
#ifndef INET6
	PLAT_ERROR("%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
	"usage: ifconfig interface address_family [address [dest_address]]",
	"                [parameters]",
	"       ifconfig -C",
	"       ifconfig interface create",
	"       ifconfig -a [-d] [-m] [-u] [address_family]",
	"       ifconfig -l [-d] [-u] [address_family]",
	"       ifconfig [-d] [-m] [-u]");
#else
	PLAT_ERROR("%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
	"usage: ifconfig [-L] interface address_family [address [dest_address]]",
	"                [parameters]",
	"       ifconfig -C",
	"       ifconfig interface create",
	"       ifconfig -a [-L] [-d] [-m] [-u] [address_family]",
	"       ifconfig -l [-d] [-u] [address_family]",
	"       ifconfig [-L] [-d] [-m] [-u]");
#endif
}

int
ifconfig_main(int argc, char* argv[])
{
	int c;
	int all, namesonly, downonly, uponly;
	int foundit = 0, need_nl = 0;
	const struct afswtch *afp = 0;
	int addrcount;
	struct	if_msghdr *ifm, *nextifm;
	struct	ifa_msghdr *ifam;
	struct	sockaddr_dl *sdl;
	char	*buf, *lim, *next;


	size_t needed;
	int mib[6];
	memset(&ifr, 0, sizeof(ifr));
	memset(&ridreq, 0, sizeof(ridreq));
	memset(&addreq, 0, sizeof(addreq));
#ifdef INET6
	explicit_prefix = 0;
	memset(&in6_ridreq, 0, sizeof(in6_ridreq));
#endif
	memset(&netmask, 0, sizeof(netmask));
	memset(name, 0, sizeof(name));
	flags = 0;
	metric = 0;
	mtu = 0; 
	setaddr = 0;
	setipdst = 0;
	setmask = 0;
	doalias = 0;
	clearaddr = 0;
	newaddr = 1;
#ifdef INET6
	int ip6lifetime = 0;
#endif
	supmedia = 0;
	listcloners = 0;
#ifdef INET6
	memset(addr_buf, 0, sizeof(addr_buf));
#endif


	/* Parse leading line options */
	all = downonly = uponly = namesonly = 0;
    init_getopt();
	while ((c = getopt(argc, argv, "adlmuC"
#ifdef INET6
					"L"
#endif
			)) != -1) {
		switch (c) {
		case 'a':	/* scan all interfaces */
			all++;
			break;
		case 'd':	/* restrict scan to "down" interfaces */
			downonly++;
			break;
		case 'l':	/* scan interface names only */
			namesonly++;
			break;
		case 'm':	/* show media choices in status */
			supmedia = 1;
			break;
		case 'u':	/* restrict scan to "up" interfaces */
			uponly++;
			break;
		case 'C':
			listcloners = 1;
			break;
#ifdef INET6
		case 'L':
			ip6lifetime++;	/* print IPv6 address lifetime */
			break;
#endif
		default:
			usage();
			return 0;

			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (listcloners) {
		/* -C must be solitary */
		if (all || supmedia || uponly || downonly || namesonly ||
		    argc > 0)
			usage();
		
		list_cloners();
		return 0;
	}

	/* -l cannot be used with -a or -m */
	if (namesonly && (all || supmedia)) {
		usage();
		return 0;

	}

	/* nonsense.. */
	if (uponly && downonly) {
		usage();
		return 0;
	}

	/* no arguments is equivalent to '-a' */
	if (!namesonly && argc < 1)
		all = 1;

	/* -a and -l allow an address family arg to limit the output */
	if (all || namesonly) {
		if (argc > 1) {
			usage();
			return 0;
		}

		if (argc == 1) {
			for (afp = afs; afp->af_name; afp++)
				if (strcmp(afp->af_name, *argv) == 0) {
					argc--, argv++;
					break;
				}
			if (afp->af_name == NULL) {
				usage();
				return 0;
			}
			/* leave with afp non-zero */
		}
	} else {
		strncpy(name, *argv, sizeof(name));
		argc--, argv++;
	}

	/* Check for address family */
	if (argc > 0) {
		for (afp = afs; afp->af_name; afp++)
			if (strcmp(afp->af_name, *argv) == 0) {
				argc--, argv++;
				break;
			}
		if (afp->af_name == NULL)
			afp = NULL;	/* not a family, NULL */
	}

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = 0;	/* address family */
	mib[4] = NET_RT_IFLIST;
	mib[5] = 0;

	/* if particular family specified, only ask about it */
	if (afp)
		mib[3] = afp->af_af;

	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
		PLAT_ERROR("iflist-sysctl-estimate\n");
		return -1;
	}
	if ((buf = malloc(needed)) == NULL) {
		PLAT_ERROR("malloc\n");
		return -1;
	}
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
                PLAT_ERROR("actual retrieval of interface table\n");
                return -1;
    }

	lim = buf + needed;

	next = buf;
	while (next < lim) {

		ifm = (struct if_msghdr *)next;
		
		if (ifm->ifm_type == RTM_IFINFO) {
			sdl = (struct sockaddr_dl *)(ifm + 1);
			flags = ifm->ifm_flags;
		} else {
			PLAT_ERROR("out of sync parsing NET_RT_IFLIST\n");
			PLAT_ERROR("expected %d, got %d\n", RTM_IFINFO,
				ifm->ifm_type);
			PLAT_ERROR("msglen = %d\n", ifm->ifm_msglen);
			PLAT_ERROR("buf:%p, next:%p, lim:%p\n", buf, next,
				lim);
			return 1;
		}

		next += ifm->ifm_msglen;
		ifam = NULL;
		addrcount = 0;
		while (next < lim) {

			nextifm = (struct if_msghdr *)next;

			if (nextifm->ifm_type != RTM_NEWADDR)
				break;

			if (ifam == NULL)
				ifam = (struct ifa_msghdr *)nextifm;

			addrcount++;
			next += nextifm->ifm_msglen;
		}

		if (all || namesonly) {
			if (uponly)
				if ((flags & IFF_UP) == 0)
					continue; /* not up */
			if (downonly)
				if (flags & IFF_UP)
					continue; /* not down */
			strncpy(name, sdl->sdl_data, sdl->sdl_nlen);
			name[sdl->sdl_nlen] = '\0';
			if (namesonly) {
				if (afp == NULL ||
					afp->af_status != ether_status ||
					sdl->sdl_type == IFT_ETHER) {
					if (need_nl)
						putchar(' ');
					fputs(name, stdout);
					need_nl++;
				}
				continue;
			}
		} else {
			if (strlen(name) != sdl->sdl_nlen)
				continue; /* not same len */
			if (strncmp(name, sdl->sdl_data, sdl->sdl_nlen) != 0)
				continue; /* not same name */
		}

		if (argc > 0)
			ifconfig(argc, argv, afp);
		else
			status(afp, addrcount, sdl, ifm, ifam);

		if (all == 0 && namesonly == 0) {
			foundit++; /* flag it as 'done' */
			break;
		}
	}
	free(buf);

	if (namesonly && need_nl > 0)
		putchar('\n');

	if (all == 0 && namesonly == 0 && foundit == 0) {
               PLAT_ERROR("interface %s does not exist\n", name);
               return -1;
    }
 
	return 0;
}


int
ifconfig(argc, argv, afp)
	int argc;
	char *const *argv;
	const struct afswtch *afp;
{
	int s;

	if (afp == NULL)
		afp = &afs[0];
	ifr.ifr_addr.sa_family = afp->af_af == AF_LINK ? AF_INET : afp->af_af;
	strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);

	if ((s = socket(ifr.ifr_addr.sa_family, SOCK_DGRAM, 0)) < 0) {
	    perror("socket");
	    return -1;
	}

	while (argc > 0) {
		register const struct cmd *p;

		for (p = cmds; p->c_name; p++)
			if (strcmp(*argv, p->c_name) == 0)
				break;
		if (p->c_name == 0 && setaddr)
			p++;	/* got src, do dst */
		if (p->c_func || p->c_func2) {
			if (p->c_parameter == NEXTARG) {
				if (argv[1] == NULL) {
					PLAT_ERROR("'%s' requires argument", p->c_name);
					goto bad;
				}
		
				if (p->c_func && (*p->c_func)(argv[1], 0, s, afp) != 0) {
					goto bad;
				}
				argc--, argv++;
			} else if (p->c_parameter == NEXTARG2) {
				if (argc < 3) {
                    PLAT_ERROR("'%s' requires 2 arguments", p->c_name);
                    goto bad;

				}
				if ((*p->c_func2)(argv[1], argv[2], s, afp) != 0) {
					goto bad;
				}
				argc -= 2, argv += 2;
			} else
				if (p->c_func && (*p->c_func)(*argv, p->c_parameter, s, afp) != 0) {
					goto bad;
				}
		}
		argc--, argv++;
	}
#ifdef INET6
	if (ifr.ifr_addr.sa_family == AF_INET6 && explicit_prefix == 0) {
		/* Aggregatable address architecture defines all prefixes
		   are 64. So, it is convenient to set prefixlen to 64 if
		   it is not specified. */
		setifprefixlen("64", 0, s, afp);
		/* in6_getprefix("64", MASK) if MASK is available here... */
	}
#endif
#ifndef NO_IPX
	if (setipdst && ifr.ifr_addr.sa_family == AF_IPX) {
		struct ipxip_req rq;
		int size = sizeof(rq);

		rq.rq_ipx = addreq.ifra_addr;
		rq.rq_ip = addreq.ifra_dstaddr;

		if (setsockopt(s, 0, SO_IPXIP_ROUTE, &rq, size) < 0) {
			perror("Encapsulation Routing");
			goto bad;			
		}

	}
#endif
#ifdef NS
	if (setipdst && ifr.ifr_addr.sa_family == AF_NS) {
		struct nsip_req rq;
		int size = sizeof(rq);

		rq.rq_ns = addreq.ifra_addr;
		rq.rq_ip = addreq.ifra_dstaddr;

		if (setsockopt(s, 0, SO_NSIP_ROUTE, &rq, size) < 0) {
			perror("Encapsulation Routing");
			goto bad;			
		}
	}
#endif
	if (clearaddr) {
		if (afp->af_ridreq == NULL || afp->af_difaddr == 0) {
			PLAT_ERROR("interface %s cannot change %s addresses!\n",
			      name, afp->af_name);
			clearaddr = (int)NULL;
		}
	}
	if (clearaddr) {
		int ret;
		strncpy(afp->af_ridreq, name, sizeof ifr.ifr_name);
		if ((ret = ioctl(s, afp->af_difaddr, afp->af_ridreq)) < 0) {
			if (errno == EADDRNOTAVAIL && (doalias >= 0)) {
				/* means no previous address for interface */
			} else {
				perror("ioctl (SIOCDIFADDR)");
				goto bad;			
			}
		}
	}
	if (newaddr) {
		if (afp->af_addreq == NULL || afp->af_aifaddr == 0) {
			PLAT_ERROR("interface %s cannot change %s addresses!\n",
			      name, afp->af_name);
			newaddr = 0;
		}
	}
	if (newaddr && (setaddr || setmask)) {
		strncpy(afp->af_addreq, name, sizeof ifr.ifr_name);
		if (ioctl(s, afp->af_aifaddr, afp->af_addreq) < 0) {
			perror("ioctl (SIOCAIFADDR)");
			goto bad;			
		}
	}
bad:

	close(s);
	return(0);
}
#define RIDADDR 0
#define ADDR	1
#define MASK	2
#define DSTADDR	3

/*ARGSUSED*/
int
setifaddr(addr, param, s, afp)
	const char *addr;
	int param;
	int s;
	const struct afswtch *afp;
{
	if (*afp->af_getaddr == NULL) {
		return 0;
	}
	/*
	 * Delay the ioctl to set the interface addr until flags are all set.
	 * The address interpretation may depend on the flags,
	 * and the flags may change when the address is set.
	 */
	setaddr++;
	if (doalias == 0 && afp->af_af != AF_LINK)
		clearaddr = 1;
	return (*afp->af_getaddr)(addr, (doalias >= 0 ? ADDR : RIDADDR));
}


int
setifnetmask(addr, dummy, s, afp)
	const char *addr;
	int dummy __unused;
	int s;
	const struct afswtch *afp;
{
	if (*afp->af_getaddr == NULL) {
        return -1;

	}
	setmask++;
	return (*afp->af_getaddr)(addr, MASK);
}

#ifdef INET6
int
setifprefixlen(addr, dummy, s, afp)
        const char *addr;
	int dummy __unused;
	int s;
	const struct afswtch *afp;
{
	if (*afp->af_getprefix)
		(*afp->af_getprefix)(addr, MASK);
	explicit_prefix = 1;
	return 0;
}

int
setip6flags(dummyaddr, flag, dummysoc, afp)
	const char *dummyaddr __unused;
	int flag;
	int dummysoc __unused;
	const struct afswtch *afp;
{
	if (afp->af_af != AF_INET6) {
        PLAT_ERROR("address flags can be set only for inet6 addresses");
        return -1;
	}

	if (flag < 0)
		in6_addreq.ifra_flags &= ~(-flag);
	else
		in6_addreq.ifra_flags |= flag;
	return 0;
}

int
setip6pltime(seconds, dummy, s, afp)
    	const char *seconds;
	int dummy __unused;
	int s;
	const struct afswtch *afp;
{
	setip6lifetime("pltime", seconds, s, afp);
	return 0;
}

int
setip6vltime(seconds, dummy, s, afp)
    	const char *seconds;
	int dummy __unused;
	int s;
	const struct afswtch *afp;
{
	setip6lifetime("vltime", seconds, s, afp);
	return 0;
}

int
setip6lifetime(cmd, val, s, afp)
	const char *cmd;
	const char *val;
	int s;
	const struct afswtch *afp;
{
	time_t newval, t;
	char *ep;

	t = time(NULL);
	newval = (time_t)strtoul(val, &ep, 0);
	if (val == ep) {
		PLAT_ERROR("invalid %s", cmd);
		return -1;
	}
	if (afp->af_af != AF_INET6) {
        PLAT_ERROR("%s not allowed for the AF", cmd);
        return -1;
	}
	if (strcmp(cmd, "vltime") == 0) {
		in6_addreq.ifra_lifetime.ia6t_expire = t + newval;
		in6_addreq.ifra_lifetime.ia6t_vltime = newval;
	} else if (strcmp(cmd, "pltime") == 0) {
		in6_addreq.ifra_lifetime.ia6t_preferred = t + newval;
		in6_addreq.ifra_lifetime.ia6t_pltime = newval;
	}
	return 0;
}
#endif


int
setifbroadaddr(addr, dummy, s, afp)
	const char *addr;
	int dummy __unused;
	int s;
	const struct afswtch *afp;
{
	if (*afp->af_getaddr == NULL) {
		return 0;
	}
	return (*afp->af_getaddr)(addr, DSTADDR);
}

int
setifipdst(addr, dummy, s, afp)
	const char *addr;
	int dummy __unused;
	int s;
	const struct afswtch *afp;
{
	in_getaddr(addr, DSTADDR);
	setipdst++;
	clearaddr = 0;
	newaddr = 0;
	return 0;
}
#define rqtosa(x) (&(((struct ifreq *)(afp->x))->ifr_addr))

int
notealias(addr, param, s, afp)
	const char *addr;
	int param;
	int s;
	const struct afswtch *afp;
{
	if (setaddr && doalias == 0 && param < 0)
		bcopy((caddr_t)rqtosa(af_addreq),
		      (caddr_t)rqtosa(af_ridreq),
		      rqtosa(af_addreq)->sa_len);
	doalias = param;
	if (param < 0) {
		clearaddr = 1;
		newaddr = 0;
	} else
		clearaddr = 0;
	return 0;
}

/*ARGSUSED*/
int
setifdstaddr(addr, param, s, afp)
	const char *addr;
	int param __unused;
	int s;
	const struct afswtch *afp;
{
	if (*afp->af_getaddr == NULL) {
		return -1;
	}
	return (*afp->af_getaddr)(addr, DSTADDR) ;
}

/*
 * Note: doing an SIOCIGIFFLAGS scribbles on the union portion
 * of the ifreq structure, which may confuse other parts of ifconfig.
 * Make a private copy so we can avoid that.
 */
int
setifflags(vname, value, s, afp)
	const char *vname;
	int value;
	int s;
	const struct afswtch *afp;
{
	struct ifreq		my_ifr;

	bcopy((char *)&ifr, (char *)&my_ifr, sizeof(struct ifreq));

 	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&my_ifr) < 0) {
 		perror("ioctl (SIOCGIFFLAGS)");
		return -1;
 	}
	strncpy(my_ifr.ifr_name, name, sizeof (my_ifr.ifr_name));
 	flags = my_ifr.ifr_flags;

	if (value < 0) {
		value = -value;
		flags &= ~value;
	} else
		flags |= value;
	my_ifr.ifr_flags = flags;
	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&my_ifr) < 0) {
		perror(vname);
		return -1;
	}
	return 0;
}

int
setifmetric(val, dummy, s, afp)
	const char *val;
	int dummy __unused;
	int s;
	const struct afswtch *afp;
{
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	ifr.ifr_metric = atoi(val);
	if (ioctl(s, SIOCSIFMETRIC, (caddr_t)&ifr) < 0) {
		perror("ioctl (set metric)");
		return -1;
	}	

	return 0;
}

int
setifmtu(val, dummy, s, afp)
	const char *val;
	int dummy __unused;
	int s;
	const struct afswtch *afp;
{
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	ifr.ifr_mtu = atoi(val);
	if (ioctl(s, SIOCSIFMTU, (caddr_t)&ifr) < 0) {
		perror("ioctl (set mtu)");
		return -1;
	}
	return 0;
}

int
setiflladdr(val, dummy, s, afp)
	const char *val;
	int dummy __unused;
	int s;
	const struct afswtch *afp;
{
	struct ether_addr	*ea;

	ea = ether_aton(val);
	if (ea == NULL) {
		PLAT_ERROR("malformed link-level address\n");
		return -1;
	}
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	ifr.ifr_addr.sa_len = ETHER_ADDR_LEN;
	ifr.ifr_addr.sa_family = AF_LINK;
	bcopy(ea, ifr.ifr_addr.sa_data, ETHER_ADDR_LEN);
	if (ioctl(s, SIOCSIFLLADDR, (caddr_t)&ifr) < 0) {
		perror("ioctl (set lladdr)");
		return -1;
	}

	return 0;
}

struct     net_device_stats {
    cyg_uint64 rx_packets;
    cyg_uint64 tx_packets;
};


#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6SMART\7RUNNING" \
"\10NOARP\11PROMISC\12ALLMULTI\13OACTIVE\14SIMPLEX\15LINK0\16LINK1\17LINK2" \
"\20MULTICAST"

/*
 * Print the status of the interface.  If an address family was
 * specified, show it and it only; otherwise, show them all.
 */
void
status(afp, addrcount, sdl, ifm, ifam)
	const struct afswtch *afp;
	int addrcount;
	struct	sockaddr_dl *sdl;
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;
{
	const struct afswtch *p = NULL;
	struct	rt_addrinfo info;
	int allfamilies, s;
	struct ifstat ifs;

	if (afp == NULL) {
		allfamilies = 1;
		afp = &afs[0];
	} else
		allfamilies = 0;

	ifr.ifr_addr.sa_family = afp->af_af == AF_LINK ? AF_INET : afp->af_af;
	strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);

	if ((s = socket(ifr.ifr_addr.sa_family, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return;
	}

	/*
	 * XXX is it we are doing a SIOCGIFMETRIC etc for one family.
	 * is it possible that the metric and mtu can be different for
	 * each family?  If so, we have a format problem, because the
	 * metric and mtu is printed on the global the flags line.
	 */
	if (ioctl(s, SIOCGIFMETRIC, (caddr_t)&ifr) < 0) {
		perror("ioctl (SIOCGIFMETRIC)");
	}
	else
		metric = ifr.ifr_metric;

	if (ioctl(s, SIOCGIFMTU, (caddr_t)&ifr) < 0)
		perror("ioctl (SIOCGIFMTU)");
	else
		mtu = ifr.ifr_mtu;

	printf("%-8s ", name);
	printb("flags", flags, IFFBITS);
	if (metric)
		printf(" metric %d", metric);
	if (mtu)
		printf(" mtu %d", mtu);
	putchar('\n');
    

	/* check if virtual interfaces*/
	if (strncmp(name, "lo", 2) && strncmp (name, "br", 2) && strncmp (name, "ppp", 3)) {
		struct net_device_stats stats_db;
		ifr.ifr_ifru.ifru_data = (caddr_t)&stats_db;
		if (ioctl(s, SIOCGIFSTATS, (caddr_t)&ifr) < 0) {
			perror("SIOCGIFSTATS fail");        
		} else {        
			printf("         RX packet:%llu, TX packet:%llu\n",stats_db.rx_packets,stats_db.tx_packets);    
		}
	}

	/* Print statistics of virtual interfaces(bridge/pppoe)*/
	if (!strncmp(name, "br", 2) || !strncmp(name, "ppp", 3)) {
		print_statistics(name);
	}

	while (addrcount > 0) {
		
		info.rti_addrs = ifam->ifam_addrs;

		/* Expand the compacted addresses */
		rt_xaddrs((char *)(ifam + 1), ifam->ifam_msglen + (char *)ifam,
			  &info);

		if (!allfamilies) {
			if (afp->af_af == info.rti_info[RTAX_IFA]->sa_family) {
				p = afp;
				if ((*p->af_status)(s, &info) != 0) {
					goto bad;
				}
			}
		} else for (p = afs; p->af_name; p++) {
			if (p->af_af == info.rti_info[RTAX_IFA]->sa_family) {
				if ((*p->af_status)(s, &info) != 0) {
					goto bad;
				}
			}
		}
		addrcount--;
		ifam = (struct ifa_msghdr *)((char *)ifam + ifam->ifam_msglen);
	}
	if (allfamilies || afp->af_status == ether_status)
		ether_status(s, (struct rt_addrinfo *)sdl);
#ifdef USE_IF_MEDIA
	if (allfamilies || afp->af_status == media_status)
		media_status(s, NULL);
#endif
#ifdef USE_VLANS
	if (allfamilies || afp->af_status == vlan_status)
		vlan_status(s, NULL);
#endif
#ifdef USE_IEEE80211
	if (allfamilies || afp->af_status == ieee80211_status)
		ieee80211_status(s, NULL);
#endif
	strncpy(ifs.ifs_name, name, sizeof ifs.ifs_name);
	if (ioctl(s, SIOCGIFSTATUS, &ifs) == 0) 
		printf("%s", ifs.ascii);

	if (!allfamilies && !p && 
#ifdef USE_IF_MEDIA
		afp->af_status != media_status &&
#endif
	    afp->af_status != ether_status
#ifdef USE_VLANS
	    && afp->af_status != vlan_status
#endif
		)
	PLAT_ERROR("%s has no %s interface address!\n", name, afp->af_name);

        putchar('\n');
bad:
	close(s);
	return;
}


int
in_status(s, info)
	int s __unused;
	struct rt_addrinfo * info;
{
	struct sockaddr_in *sin, null_sin;
	
	memset(&null_sin, 0, sizeof(null_sin));

	sin = (struct sockaddr_in *)info->rti_info[RTAX_IFA];
	printf("         inet %s ", inet_ntoa(sin->sin_addr));

	if (flags & IFF_POINTOPOINT) {
		/* note RTAX_BRD overlap with IFF_BROADCAST */
		sin = (struct sockaddr_in *)info->rti_info[RTAX_BRD];
		if (!sin)
			sin = &null_sin;
		printf("P-T-P %s ", inet_ntoa(sin->sin_addr));
	}

	sin = (struct sockaddr_in *)info->rti_info[RTAX_NETMASK];
	if (!sin)
		sin = &null_sin;
	printf("netmask 0x%lx ", (unsigned long)ntohl(sin->sin_addr.s_addr));

	if (flags & IFF_BROADCAST) {
		/* note RTAX_BRD overlap with IFF_POINTOPOINT */
		sin = (struct sockaddr_in *)info->rti_info[RTAX_BRD];
		if (sin && sin->sin_addr.s_addr != 0)
			printf("broadcast %s", inet_ntoa(sin->sin_addr));
	}
	putchar('\n');
	return 0;
}

#ifdef INET6
void
in6_fillscopeid(sin6)
	struct sockaddr_in6 *sin6;
{
#if defined(__KAME__) && defined(KAME_SCOPEID)
	if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
		sin6->sin6_scope_id =
			ntohs(*(u_int16_t *)&sin6->sin6_addr.s6_addr[2]);
		sin6->sin6_addr.s6_addr[2] = sin6->sin6_addr.s6_addr[3] = 0;
	}
#endif
}

int
in6_status(int s, struct rt_addrinfo *info)
{
	struct sockaddr_in6 *sin, null_sin;
	struct in6_ifreq ifr6;
	int s6;
	u_int32_t flags6;
	struct in6_addrlifetime lifetime;
	time_t t = time(NULL);
	int error;
	u_int32_t scopeid;

	memset(&null_sin, 0, sizeof(null_sin));

	sin = (struct sockaddr_in6 *)info->rti_info[RTAX_IFA];
	strncpy(ifr6.ifr_name, ifr.ifr_name, sizeof(ifr.ifr_name));
	if ((s6 = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		perror("ifconfig: socket");
		return -1;
	}
	ifr6.ifr_addr = *sin;
	if (ioctl(s6, SIOCGIFAFLAG_IN6, &ifr6) < 0) {
		perror("ifconfig: ioctl(SIOCGIFAFLAG_IN6)");
		close(s6);
		return -1;
	}
	flags6 = ifr6.ifr_ifru.ifru_flags6;
	memset(&lifetime, 0, sizeof(lifetime));
	ifr6.ifr_addr = *sin;
	if (ioctl(s6, SIOCGIFALIFETIME_IN6, &ifr6) < 0) {
		perror("ifconfig: ioctl(SIOCGIFALIFETIME_IN6)");
		close(s6);
		return -1;
	}
	lifetime = ifr6.ifr_ifru.ifru_lifetime;
	close(s6);

	/* XXX: embedded link local addr check */
	if (IN6_IS_ADDR_LINKLOCAL(&sin->sin6_addr) &&
	    *(u_short *)&sin->sin6_addr.s6_addr[2] != 0) {
		u_short index;

		index = *(u_short *)&sin->sin6_addr.s6_addr[2];
		*(u_short *)&sin->sin6_addr.s6_addr[2] = 0;
		if (sin->sin6_scope_id == 0)
			sin->sin6_scope_id = ntohs(index);
	}
	scopeid = sin->sin6_scope_id;

	error = getnameinfo((struct sockaddr *)sin, sin->sin6_len, addr_buf,
			    sizeof(addr_buf), NULL, 0,
			    NI_NUMERICHOST|NI_WITHSCOPEID);
	if (error != 0)
		inet_ntop(AF_INET6, &sin->sin6_addr, addr_buf,
			  sizeof(addr_buf));
	printf("\tinet6 %s ", addr_buf);

	if (flags & IFF_POINTOPOINT) {
		/* note RTAX_BRD overlap with IFF_BROADCAST */
		sin = (struct sockaddr_in6 *)info->rti_info[RTAX_BRD];
		/*
		 * some of the interfaces do not have valid destination
		 * address.
		 */
		if (sin && sin->sin6_family == AF_INET6) {
			int error;

			/* XXX: embedded link local addr check */
			if (IN6_IS_ADDR_LINKLOCAL(&sin->sin6_addr) &&
			    *(u_short *)&sin->sin6_addr.s6_addr[2] != 0) {
				u_short index;

				index = *(u_short *)&sin->sin6_addr.s6_addr[2];
				*(u_short *)&sin->sin6_addr.s6_addr[2] = 0;
				if (sin->sin6_scope_id == 0)
					sin->sin6_scope_id = ntohs(index);
			}

			error = getnameinfo((struct sockaddr *)sin,
					    sin->sin6_len, addr_buf,
					    sizeof(addr_buf), NULL, 0,
					    NI_NUMERICHOST|NI_WITHSCOPEID);
			if (error != 0)
				inet_ntop(AF_INET6, &sin->sin6_addr, addr_buf,
					  sizeof(addr_buf));
			printf("--> %s ", addr_buf);
		}
	}

	sin = (struct sockaddr_in6 *)info->rti_info[RTAX_NETMASK];
	if (!sin)
		sin = &null_sin;
	printf("prefixlen %d ", prefix(&sin->sin6_addr,
		sizeof(struct in6_addr)));

	if ((flags6 & IN6_IFF_ANYCAST) != 0)
		printf("anycast ");
	if ((flags6 & IN6_IFF_TENTATIVE) != 0)
		printf("tentative ");
	if ((flags6 & IN6_IFF_DUPLICATED) != 0)
		printf("duplicated ");
	if ((flags6 & IN6_IFF_DETACHED) != 0)
		printf("detached ");
	if ((flags6 & IN6_IFF_DEPRECATED) != 0)
		printf("deprecated ");
	if ((flags6 & IN6_IFF_AUTOCONF) != 0)
		printf("autoconf ");
	if ((flags6 & IN6_IFF_TEMPORARY) != 0)
		printf("temporary ");

        if (scopeid)
		printf("scopeid 0x%x ", scopeid);

	if (ip6lifetime && (lifetime.ia6t_preferred || lifetime.ia6t_expire)) {
		printf("pltime ");
		if (lifetime.ia6t_preferred) {
			printf("%s ", lifetime.ia6t_preferred < t
				? "0" : sec2str(lifetime.ia6t_preferred - t));
		} else
			printf("infty ");

		printf("vltime ");
		if (lifetime.ia6t_expire) {
			printf("%s ", lifetime.ia6t_expire < t
				? "0" : sec2str(lifetime.ia6t_expire - t));
		} else
			printf("infty ");
	}

	putchar('\n');

	return 0;
}
#endif /*INET6*/

#ifndef NO_IPX
void
ipx_status(s, info)
	int s __unused;
	struct rt_addrinfo * info;
{
	struct sockaddr_ipx *sipx, null_sipx;

	memset(&null_sipx, 0, sizeof(null_sipx));

	sipx = (struct sockaddr_ipx *)info->rti_info[RTAX_IFA];
	printf("\tipx %s ", ipx_ntoa(sipx->sipx_addr));

	if (flags & IFF_POINTOPOINT) {
		sipx = (struct sockaddr_ipx *)info->rti_info[RTAX_BRD];
		if (!sipx)
			sipx = &null_sipx;
		printf("--> %s ", ipx_ntoa(sipx->sipx_addr));
	}
	putchar('\n');
}
#endif



#ifdef NS
void
xns_status(s, info)
	int s __unused;
	struct rt_addrinfo * info;
{
	struct sockaddr_ns *sns, null_sns;

	memset(&null_sns, 0, sizeof(null_sns));

	sns = (struct sockaddr_ns *)info->rti_info[RTAX_IFA];
	printf("\tns %s ", ns_ntoa(sns->sns_addr));

	if (flags & IFF_POINTOPOINT) {
		sns = (struct sockaddr_ns *)info->rti_info[RTAX_BRD];
		if (!sns)
			sns = &null_sns;
		printf("--> %s ", ns_ntoa(sns->sns_addr));
	}

	putchar('\n');
	close(s);
}
#endif


int
ether_status(s, info)
	int s __unused;
	struct rt_addrinfo *info;
{
	char *cp;
	int n;
	struct sockaddr_dl *sdl = (struct sockaddr_dl *)info;

	cp = (char *)LLADDR(sdl);
	if ((n = sdl->sdl_alen) > 0) {
		if (sdl->sdl_type == IFT_ETHER)
			printf ("         ether ");
		else
			printf ("         lladdr ");
             	while (--n >= 0)
			printf("%02x%c",*cp++ & 0xff, n>0? ':' : ' ');
		putchar('\n');
	}
	return 0;
}


#define SIN(x) ((struct sockaddr_in *) &(x))
struct sockaddr_in *sintab[] = {
SIN(ridreq.ifr_addr), SIN(addreq.ifra_addr),
SIN(addreq.ifra_mask), SIN(addreq.ifra_broadaddr)};

int
in_getaddr(s, which)
	const char *s;
	int which;
{
	register struct sockaddr_in *sin = sintab[which];
	struct hostent *hp;

	sin->sin_len = sizeof(*sin);
	if (which != MASK)
		sin->sin_family = AF_INET;

	if (which == ADDR) {
		char *p = NULL;

		if((p = strrchr(s, '/')) != NULL) {
			/* address is `name/masklen' */
			int masklen;
			int ret;
			struct sockaddr_in *min = sintab[MASK];
			*p = '\0';
			ret = sscanf(p+1, "%u", &masklen);
			if(ret != 1 || (masklen < 0 || masklen > 32)) {
				*p = '/';
				PLAT_ERROR("%s: bad value\n", s);
				return -1;
			}
			min->sin_len = sizeof(*min);
			min->sin_addr.s_addr = htonl(~((1LL << (32 - masklen)) - 1) & 
				              0xffffffff);
		}
	}

	if (inet_aton(s, &sin->sin_addr))
		return 0;
	if ((hp = gethostbyname(s)) != 0)
		bcopy(hp->h_addr, (char *)&sin->sin_addr, 
		    MIN(hp->h_length, sizeof(sin->sin_addr)));
	else {
		PLAT_ERROR("%s: bad value\n", s);
		return -1;

	}
	return 0;
}

#ifdef INET6
#define	SIN6(x) ((struct sockaddr_in6 *) &(x))
struct	sockaddr_in6 *sin6tab[] = {
SIN6(in6_ridreq.ifr_addr), SIN6(in6_addreq.ifra_addr),
SIN6(in6_addreq.ifra_prefixmask), SIN6(in6_addreq.ifra_dstaddr)};

int
in6_getaddr(s, which)
	const char *s;
	int which;
{
	register struct sockaddr_in6 *sin = sin6tab[which];
	struct addrinfo hints, *res;
	int error = -1;

	newaddr &= 1;

	sin->sin6_len = sizeof(*sin);
	if (which != MASK)
		sin->sin6_family = AF_INET6;

	if (which == ADDR) {
		char *p = NULL;
		if((p = strrchr(s, '/')) != NULL) {
			*p = '\0';
			in6_getprefix(p + 1, MASK);
			explicit_prefix = 1;
		}
	}

	if (sin->sin6_family == AF_INET6) {
		bzero(&hints, sizeof(struct addrinfo));
		hints.ai_family = AF_INET6;
		error = getaddrinfo(s, NULL, &hints, &res);
	}
	if (error != 0) {
		if (inet_pton(AF_INET6, s, &sin->sin6_addr) != 1) {
			PLAT_ERROR("%s: bad value", s);
			return -1;
//			errx(1, "%s: bad value", s);
		}
	} else
		bcopy(res->ai_addr, sin, res->ai_addrlen);

	return 0;
}

void
in6_getprefix(plen, which)
	const char *plen;
	int which;
{
	register struct sockaddr_in6 *sin = sin6tab[which];
	register u_char *cp;
	int len = atoi(plen);

	if ((len < 0) || (len > 128)) {
		PLAT_ERROR("%s: bad value", plen);
		return;
//		errx(1, "%s: bad value", plen);
	}
	sin->sin6_len = sizeof(*sin);
	if (which != MASK)
		sin->sin6_family = AF_INET6;
	if ((len == 0) || (len == 128)) {
		memset(&sin->sin6_addr, 0xff, sizeof(struct in6_addr));
		return;
	}
	memset((void *)&sin->sin6_addr, 0x00, sizeof(sin->sin6_addr));
	for (cp = (u_char *)&sin->sin6_addr; len > 7; len -= 8)
		*cp++ = 0xff;
	*cp = 0xff << (8 - len);
}
#endif

/*
 * Print a value a la the %b format of the kernel's printf
 */
static void
printb(s, v, bits)
	const char *s;
	register unsigned v;
	register const char *bits;
{
	register int i, any = 0;
	register char c;

	if (bits && *bits == 8) {
		printf("%s=%o", s, v);
	} else if (bits != NULL) {
		printf("%s=%x", s, v);
	} else {
		return;
	}
	bits++;
	if (bits) {
		putchar('<');
		while ((i = *bits++) != '\0') {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

#ifndef NO_IPX
#define SIPX(x) ((struct sockaddr_ipx *) &(x))
struct sockaddr_ipx *sipxtab[] = {
SIPX(ridreq.ifr_addr), SIPX(addreq.ifra_addr),
SIPX(addreq.ifra_mask), SIPX(addreq.ifra_broadaddr)};

void
ipx_getaddr(addr, which)
	const char *addr;
	int which;
{
	struct sockaddr_ipx *sipx = sipxtab[which];

	sipx->sipx_family = AF_IPX;
	sipx->sipx_len = sizeof(*sipx);
	sipx->sipx_addr = ipx_addr(addr);
	if (which == MASK)
		printf("Attempt to set IPX netmask will be ineffectual\n");
}
#endif


int
ether_getaddr(addr, which)
	const char *addr;
	int which;
{
	struct ether_addr *ea;
	struct sockaddr *sea = &ridreq.ifr_addr;

	ea = ether_aton(addr);
	if (ea == NULL) {
		PLAT_ERROR("malformed ether address\n");
		return -1;
	}
	if (which == MASK) {
		PLAT_ERROR("Ethernet does not use netmasks\n");
		return -1;
	}
	sea->sa_family = AF_LINK;
	sea->sa_len = ETHER_ADDR_LEN;
	bcopy(ea, sea->sa_data, ETHER_ADDR_LEN);
	return 0;
}


#ifdef NS
#define SNS(x) ((struct sockaddr_ns *) &(x))
struct sockaddr_ns *snstab[] = {
SNS(ridreq.ifr_addr), SNS(addreq.ifra_addr),
SNS(addreq.ifra_mask), SNS(addreq.ifra_broadaddr)};

void
xns_getaddr(addr, which)
	const char *addr;
	int which;
{
	struct sockaddr_ns *sns = snstab[which];

	sns->sns_family = AF_NS;
	sns->sns_len = sizeof(*sns);
	sns->sns_addr = ns_addr(addr);
	if (which == MASK)
		printf("Attempt to set XNS netmask will be ineffectual\n");
}
#endif

#ifdef INET6
int
prefix(val, size)
        void *val;
        int size;
{
        register u_char *name = (u_char *)val;
        register int byte, bit, plen = 0;

        for (byte = 0; byte < size; byte++, plen += 8)
                if (name[byte] != 0xff)
                        break;
	if (byte == size)
		return (plen);
	for (bit = 7; bit != 0; bit--, plen++)
                if (!(name[byte] & (1 << bit)))
                        break;
        for (; bit != 0; bit--)
                if (name[byte] & (1 << bit))
                        return(0);
        byte++;
        for (; byte < size; byte++)
                if (name[byte])
                        return(0);
        return (plen);
}

static char *
sec2str(total)
	time_t total;
{
	static char result[256];
	int days, hours, mins, secs;
	int first = 1;
	char *p = result;

	if (0) {
		days = total / 3600 / 24;
		hours = (total / 3600) % 24;
		mins = (total / 60) % 60;
		secs = total % 60;

		if (days) {
			first = 0;
			p += sprintf(p, "%dd", days);
		}
		if (!first || hours) {
			first = 0;
			p += sprintf(p, "%dh", hours);
		}
		if (!first || mins) {
			first = 0;
			p += sprintf(p, "%dm", mins);
		}
		sprintf(p, "%ds", secs);
	} else
		sprintf(result, "%lu", (unsigned long)total);

	return(result);
}
#endif /*INET6*/


void
list_cloners(void)
{
	struct if_clonereq ifcr;
	char *cp, *buf;
	int idx;
	int s;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		perror("socket");
		return;
	}

	memset(&ifcr, 0, sizeof(ifcr));

	if (ioctl(s, SIOCIFGCLONERS, &ifcr) < 0) {
		perror("SIOCIFGCLONERS for count");
		close(s);
		return;
	}

	buf = malloc(ifcr.ifcr_total * IFNAMSIZ);
	if (buf == NULL) {
		fprintf(stderr,"unable to allocate cloner name buffer\n");
		close(s);
		return;
	}

	ifcr.ifcr_count = ifcr.ifcr_total;
	ifcr.ifcr_buffer = buf;

	if (ioctl(s, SIOCIFGCLONERS, &ifcr) < 0) {
		perror("SIOCIFGCLONERS for names");
		close(s);
		free(buf);
		return;
	}

	/*
	 * In case some disappeared in the mean time, clamp it down.
	 */
	if (ifcr.ifcr_count > ifcr.ifcr_total)
		ifcr.ifcr_count = ifcr.ifcr_total;

	for (cp = buf, idx = 0; idx < ifcr.ifcr_count; idx++, cp += IFNAMSIZ) {
		if (idx > 0)
			putchar(' ');
		printf("%s", cp);
	}

	putchar('\n');
	free(buf);
	close(s);
}
shell_cmd("mac",
	 "Get or Set MAC",
	 "",
	 _mac);

#define ISXDIGIT(x) ((x) >= 0x0 && (x) <= 0xff)

bool
_token_convert(char *mac_addr, char *mac_addrs)
{
    char *token = strtok(mac_addr, ":");

	char *cp = NULL ;

    int l = 0;

	int result = 0 ;

	unsigned char zero_count = 0;

	unsigned char ff_count = 0;

    while (token) {

        result = simple_strtoul(token, &cp, 16);
		
		if ((cp && *cp ) || result > 0xff)
				return false;

		mac_addrs[l] = result ;

		if ( mac_addrs[l] == 0x00)
				zero_count++;
		else if (  (unsigned char)mac_addrs[l] == 0xff)
				ff_count++;

		if ( ! ISXDIGIT((unsigned char)mac_addrs[l]) || l > ETHER_ADDR_LEN-1 ) {
				return false;
		}

        token = strtok(NULL, ":");

        l++;
    }

	if (l != ETHER_ADDR_LEN)
		return false;

	if ( zero_count == 6 || ff_count == 6)
			return false;

	return true;
}

void mac_usage(void)
{
	SHELL_USAGE("usage: mac <eth0/eth1/wifi0> [xx:xx:xx:xx:xx:xx]\n");
	SHELL_USAGE("notice 8c:fd:f0 as first three octates they are must for wifi0\n");
	SHELL_USAGE("ex: mac eth0 - to get mac of eth0\n");
	SHELL_USAGE("ex: mac eth1 00:aa:bb:11:22:33 - to set mac of eth1\n");
	SHELL_USAGE("ex: mac wifi0 8c:fd:f0:11:22:33 - to set mac of wifi0\n");
}

CMD_DECL(_mac) 
{ 
	struct ifreq ifr; 
 
    int s; 

	unsigned long FLAG ;

	if (argc < 1 || argc > 2) {

		mac_usage();

		return SHELL_INVALID_ARGUMENT ;
	}

	if ( (strlen((char *)argv[0]) < 4 ) || 
		 (strncmp((char *)argv[0], "eth0",4) && 
		  strncmp((char *)argv[0], "eth1",4)  && strncmp((char *)argv[0], "wifi0",5))) {

		mac_usage();

		return SHELL_INVALID_ARGUMENT ;
	}

	if ( argc == 2 && argv[1] != NULL ) {

		if (! _token_convert((char *)argv[1], (char *)&ifr.ifr_hwaddr.sa_data) ) {

			mac_usage();

			return SHELL_INVALID_ARGUMENT ;
		}

	}
	
	strlcpy(ifr.ifr_name,(char *)argv[0], sizeof (ifr.ifr_name)); 
		
    s = socket(AF_INET, SOCK_DGRAM, 0); 
 
	if ( s < 0 ) {
			PLAT_ERROR("Socket Failed\n");
			return SHELL_INVALID_ARGUMENT;
	}

	if (argc == 1 ) {
			ifr.ifr_addr.sa_family = AF_INET ;
			FLAG = SIOCGIFHWADDR ;
	}
	else {
			FLAG = SIOCSIFHWADDR ;
	}
 
    if ( ioctl(s, FLAG, &ifr)) {
		PLAT_ERROR ("mac set/get failed\n") ; 
		close(s); 
		return SHELL_INVALID_ARGUMENT ;
	}

	close(s); 
	s = 6 ;
	char *cp =(char *) &ifr.ifr_hwaddr.sa_data ;
	if (argc == 1) {
		    while (--s >= 0)
            printf("%02x%c",*cp++ & 0xff, s>0? ':' : '\n');
	}

    return SHELL_OK; 
}
