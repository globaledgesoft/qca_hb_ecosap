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

/**
 *
 */

#ifndef _DBG_PRINT_H_
#define _DBG_PRINT_H_

#include <stdio.h>
#include <stdarg.h>
#include <pkgconf/system.h>
#include <cyg/infra/diag.h>

#define DBG_MSG_CRITICAL	0x00000001
#define DBG_MSG_ERROR		0x00000002
#define DBG_MSG_WARNING		0x00000004
#define DBG_MSG_DEBUG		0x00000008
#define DBG_MSG_INFO		0x00000010

/* Modules */
#define DBG_MODULE_WLAN		0x00000100
#define DBG_MODULE_NETWORK	0x00010000
#define DBG_MODULE_PLATFORM	0x01000000


/*For enabling critical messages */
#ifdef CYGPKG_DBGLVL_CRITICAL
#define CRITICAL_MESSAGES
#endif /* CYGPKG_DBGLVL_CRITICAL */

/* For enabling error and critical messages */
#ifdef CYGPKG_DBGLVL_ERROR 
#define CRITICAL_MESSAGES
#define ERROR_MESSAGES
#endif /* CYGPKG_DBGLVL_ERROR */

/*For enabling warnig, error, critical messages */
#ifdef CYGPKG_DBGLVL_WARNING
#define CRITICAL_MESSAGES
#define ERROR_MESSAGES
#define WARNING_MESSAGES
#endif /* CYGPKG_DBGLVL_WARNING */

/* For enabling info, warnig, error, critical messages */
#ifdef CYGPKG_DBGLVL_INFO
#define CRITICAL_MESSAGES
#define ERROR_MESSAGES
#define WARNING_MESSAGES
#define INFO_MESSAGES
#endif /* CYGPKG_DBGLVL_INFO */

/* For enabling all messages */
#ifdef CYGPKG_DBGLVL_DEBUG //DEBUG
#define DEBUG_MESSAGES
#define INFO_MESSAGES
#define WARNING_MESSAGES
#define ERROR_MESSAGES
#define CRITICAL_MESSAGES 
#endif /* CYGPKG_DBGLVL_DEBUG */



#ifdef ENABLE_DPRINT_MSGS

#define DEFAULT_DEBUG_LEVEL	(DBG_MODULE_WLAN | DBG_MODULE_NETWORK | \
				DBG_MSG_CRITICAL | DBG_MSG_ERROR | DBG_MSG_WARNING | DBG_MSG_INFO | DBG_MSG_DEBUG)

/* ToDo: Set debug level at runtime */
extern unsigned int debug_level;

#ifdef INFO_MESSAGES
#define WLAN_INFO(...)	do {		\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_WLAN) &&			\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_INFO)) {	\
            diag_printf(__VA_ARGS__);	\
        }    \
	} while(0)
#else 
#define WLAN_INFO(...)
#endif /*INFO_MESSAGES*/

#ifdef CRITICAL_MESSAGES
#define WLAN_CRITICAL(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_WLAN) &&			\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_CRITICAL)) {	\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else 
#define WLAN_CRITICAL(...)
#endif /* CRITICAL_MESSAGES */


#ifdef ERROR_MESSAGES
#define WLAN_ERROR(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_WLAN) &&			\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_ERROR)) {		\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else 
#define WLAN_ERROR(...)
#endif /* ERROR_MESSAGES*/

#ifdef WARNING_MESSAGES
#define WLAN_WARNING(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_WLAN) &&			\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_WARNING)) {		\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else 
#define WLAN_WARNING(...)
#endif /*WARNING_MESSAGES */

#ifdef DEBUG_MESSAGES
#define WLAN_DEBUG(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_WLAN) &&			\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_DEBUG)) {		\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else 
#define WLAN_DEBUG(...)
#endif /*DEBUG_MESSAGES*/

#ifdef INFO_MESSAGES
#define NET_INFO(...)	do {		\
		diag_printf(__VA_ARGS__);	\
	} while(0)
#else
#define NET_INFO(...)
#endif /*INFO_MESSAGES*/

#ifdef CRITICAL_MESSAGES
#define NET_CRITICAL(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_NETWORK) &&		\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_CRITICAL)) {		\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else 
#define NET_CRITICAL(...)
#endif /*CRITICAL_MESSAGES*/

#ifdef ERROR_MESSAGES
#define NET_ERROR(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_NETWORK) &&		\
			 (DEFAULT_DEBUG_LEVEL & DBG_MSG_ERROR)) {		\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else 
#define NET_ERROR(...)
#endif /*ERROR_MESSAGES*/

#ifdef WARNING_MESSAGES
#define NET_WARNING(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_NETWORK) &&		\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_WARNING)) {		\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else 
#define NET_WARNING(...)
#endif /*WARNING_MESSAGES*/

#ifdef DEBUG_MESSAGES
#define NET_DEBUG(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_NETWORK) &&		\
			(DEFAULT_DEBUG_LEVEL & 	DBG_MSG_DEBUG)) {		\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else 
#define NET_DEBUG(...)
#endif /*DEBUG_MESSAGES*/

#ifdef INFO_MESSAGES
#define PLAT_INFO(...)	do {		\
		diag_printf(__VA_ARGS__);	\
	} while(0)
#else 
#define PLAT_INFO(...)
#endif /* INFO_MESSAGES */

#ifdef CRITICAL_MESSAGES
#define PLAT_CRITICAL(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_PLATFORM) &&		\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_CRITICAL)) {	\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else
#define PLAT_CRITICAL(...)
#endif /* CRITICAL_MESSAGES */

#ifdef ERROR_MESSAGES
#define PLAT_ERROR(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_PLATFORM) &&		\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_ERROR)) {		\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else
#define PLAT_ERROR(...)
#endif /* ERROR_MESSAGES */

#ifdef WARNING_MESSAGES
#define PLAT_WARNING(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_PLATFORM) &&		\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_WARNING)) {		\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else
#define PLAT_WARNING(...)
#endif /* WARNING_MESSAGES */

#ifdef DEBUG_MESSAGES
#define PLAT_DEBUG(...)  do {						\
		if ((DEFAULT_DEBUG_LEVEL & DBG_MODULE_PLATFORM) &&		\
			(DEFAULT_DEBUG_LEVEL & DBG_MSG_DEBUG)) {		\
			diag_printf(__VA_ARGS__);				\
		}								\
	} while(0)
#else 
#define PLAT_DEBUG(...) 
#endif /*DEBUG_MESSAGES*/

#else
#define WLAN_CRITICAL(...)
#define WLAN_ERROR(...)
#define WLAN_WARNING(...)
#define WLAN_INFO(...)
#define WLAN_DEBUG(...)

#define NET_CRITICAL(...)
#define NET_ERROR(...)
#define NET_WARNING(...)
#define NET_INFO(...)
#define NET_DEBUG(...)

#define PLAT_CRITICAL(...)
#define PLAT_ERROR(...)
#define PLAT_WARNING(...)
#define PLAT_INFO(...)
#define PLAT_DEBUG(...)

#endif /* ENABLE_DPRINT_MSGS */

#endif /* _DBG_PRINT_H_ */

