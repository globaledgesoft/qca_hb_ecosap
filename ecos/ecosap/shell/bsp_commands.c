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

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/diag.h>
#include <cyg/io/devtab.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/var_intr.h>
#include <cyg/hal/plf_io.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/qca953x_watchdog.h>
#include <pkgconf/system.h>

#include <ctype.h>
#include <stdlib.h>
#include <shell.h>
#include <shell_err.h>
#include <shell_thread.h>
#include <commands.h>
#include <stdio.h>
#include <time.h>
#include "debug.h"
#include <cyg/hal/plf_intr.h>
#include <cyg/dbg_print/dbg_print.h>

#include <ecosap_nw_http.h>

enum WHOSE {
    CONFIG_DATA = 0,
#ifdef CRASHDUMP_SUPPORT
    CRASH_DUMP,
#endif
    DOMAIN_MAX // Invalid entry 
};


#ifdef INTERRUPT_STATS

#define INTR_MAX 27

volatile unsigned int _interrupt_counter_[INTR_MAX + 1] = {};

#endif /* INTERRUPT_STATS */ 

shell_cmd("show",
"Show various information",
USAGESTR("show"),
show_cmd);

unsigned int athrs27_reg_read(unsigned int s27_addr);

void tdump(void *sp, void *ra, cyg_uint8 tid);

externC cyg_uint32 get_thread_ptregs(cyg_uint32 tid);
unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);   //base is the input number system
void get_thread_descend(void);
int cyg_flash_setconf(void *buf, size_t size, enum WHOSE whose);

int cyg_flash_getconf(void *buf, int len, enum WHOSE whose);

void write_nw_flash(void);
void dump_spl_info(void);

extern volatile cyg_uint32 idle_thread_loops[];
extern ECOSAP_CONFIG_STRUCT ecosap_config_struct;


#define MAX_PORT_ID 3


#define PORT_BASE_VLAN_ADDR(port_id) (0x108 + 0x100 * port_id)
#define PORT_BASE2_VLAN_ADDR(port_id) (0x10c + 0x100 * port_id)
void dump_pvid(void)
{
        int i, addr, data;
        for ( i = 1; i <= MAX_PORT_ID; i++) {
                 addr = PORT_BASE2_VLAN_ADDR(i);
                 data = athrs27_reg_read(addr);
                 if ( data &= (0x3 << 30 )) {
                        addr = PORT_BASE_VLAN_ADDR(i);
                        data = athrs27_reg_read(addr);
                        data = ((data >> 16) & 0xfff);
                        SHELL_PRINT("P%d: %d\n", i - 1, data);
                        }
        }
	if ((ecosap_config_struct.vlan_config.vlan_flag)) {
		SHELL_PRINT("P3: %d\n", ecosap_config_struct.vlan_config.inet_value);
		SHELL_PRINT("P3: %d\n", ecosap_config_struct.vlan_config.iptv_value);
	}
}


CMD_DECL(show_cmd)
{

		int k=0;

		int _state_ = 0;

		dump_spl_info();

		dump_pvid();

		SHELL_PRINT("scheduler lock = %x\n",cyg_scheduler_read_lock());

		SHELL_PRINT("idle_thread_loops[0] %u\n", idle_thread_loops[0]);	

#ifdef INTERRUPT_STATS

		for(k = 0; k <= INTR_MAX; k++) {

				HAL_INTERRUPT_IN_USE( k, _state_);

				SHELL_PRINT ("%3d   %8u %10s\n",k,_interrupt_counter_[k],(_state_?"In Use":"Free"));
		}

#endif /* INTERRUPT_STATS */

	return SHELL_OK;
}

#ifdef CONFIG_BSP_DATE

shell_cmd("date",
      "show system local time",
      USAGESTR("date [-s <[HH:MM:SS] [DD/MM/YYYY]> ]"),
      date);

int set_date(char *argv)
{
    int day, month, year;
    char buff[10],status = 0;
    time_t t1=0;
    struct tm tm_ptr;
    char *date_str;

    date_str = (char *) argv;

    strncpy(buff, date_str, 2);
    day = atoi(buff);
    strncpy(buff, (date_str+3),2);
    month = atoi(buff);
    strncpy(buff, (date_str+6),4);
    year = atoi(buff);

    memset(&tm_ptr,0,sizeof(tm_ptr));
    tm_ptr.tm_year = year - 1900;
    tm_ptr.tm_mon = month - 1;
    tm_ptr.tm_mday = day;
    t1 = mktime(&tm_ptr);

    status = cyg_libc_time_settime(t1);
    return status;
}

int set_time(char *argv)
{
     int hour, min, sec;
     char buff[10],status = -1;
     time_t mytime,t1=0;
     struct tm *tm_ptr;
     char *time_str;

     time_str = (char *) argv;

     strncpy(buff, time_str, 2);
     hour = atoi(buff);
     strncpy(buff, (time_str+3),2);
     min = atoi(buff);
     strncpy(buff, (time_str+6),2);
     sec = atoi(buff);

     tm_ptr = 0;
     time(&mytime);
     tm_ptr = localtime(&mytime);
     if (tm_ptr) {
         tm_ptr->tm_hour = hour;
         tm_ptr->tm_min = min;
         tm_ptr->tm_sec = sec;
         t1 = mktime(tm_ptr);
         status = cyg_libc_time_settime(t1);
     }

     return status;
}

CMD_DECL(date)
{
    int ret = 0;

    if (argc >= 2) {
        if (argv[0][0] == '-' && argv[0][1] == 's') {
                if (argv[1][2] == '/')
                        ret = set_date((char *)argv[(1)]);
                else if (argv[1][2] == ':')
                        ret = set_time((char *)argv[(1)]);
        }
        else {
                SHELL_USAGE("syntax:\n date [-s <[HH:MM:SS] [DD/MM/YYYY]> ] \n");
		}
    } else {
    time_t timer;
    struct tm *tblock;

    timer = time(NULL);
    tblock = localtime(&timer);

    if (tblock) 
        SHELL_PRINT("Local time is: %s\n",asctime(tblock));
    }

    if (ret != 0) {
        SHELL_USAGE("syntax:\n date [-s < [HH:MM:SS] [DD/MM/YYYY] > ] \n");
	}

    return ret;

}
#endif /* DATE */

#ifdef CONFIG_BSP_FLASH_API

shell_cmd("fwconfig",
	 "Write to flash",
	 USAGESTR("fwconfig <test-string>"),
	 set);

shell_cmd("frconfig",
	 "Read from flash",
	 USAGESTR("frconfig"),
	 get);

#define CONFIG_DATA 0
#define CRASH_DUMP 1

#include  <ecosap_nw_http.h> 

int cyg_flash_setmac(void *data, int which)
{

	int index = ( which*6 ) ;	
	
	int l = 0 ;
	for(l=0; l<6; l++) {
		ecosap_config_struct.mac_addr[index+l] = ((char*)data)[l] ;
		ecosap_new_nw_config.mac_addr[index+l] = ((char*)data)[l] ;
	}
	write_nw_flash();

	return 0 ;
	
}

CMD_DECL(set)
{
	int len = 0 ;

	if ( NULL == argv[0] ) {

		SHELL_USAGE ("usage: fwconfig <test-string>\n");

		return SHELL_INVALID_ARGUMENT;
	}
	if ( strlen( (const char *)argv[0] ) != ( len = cyg_flash_setconf(argv[0], strlen((const char *)  argv[0] ), CONFIG_DATA )) ) {

		SHELL_ERROR ("writing failed %d\n",len);

		return SHELL_INVALID_ARGUMENT;
	}
	return SHELL_OK;
}


CMD_DECL(get)
{

	//TODO: get size to read and which data to read ( CONFIG_DATA or CRASH_DUMP) from command line

	volatile char *buf = malloc (120); /* testing purpose 120bytes are enough */

	if (NULL == buf) {
		return -1;
	}

    if ( cyg_flash_getconf((void *)buf, 119,CONFIG_DATA) ) { /* test app is writing and reading data from CONFIG_DATA */
		free ((void *)buf);
		return -1;
	}

	SHELL_PRINT ("DATA: %s\n" ,buf);

	free ((void *)buf);

	return SHELL_OK;


}

#if (defined CYGPKG_NET_FTPCLIENT) && (defined CONFIG_BSP_UPGRADE)

shell_cmd("upgrade",
	 "Update Firmware",
	 "",
	 upgrade);

int download_update(int argc, char **argv, unsigned long address)
{
    if (argc != 3) {

			SHELL_USAGE("Usage: upgrade <ftp server> <ftp host>  <ftp password>\n");
	return SHELL_INVALID_ARGUMENT;
    }

    return _ftp_get(argc, argv);

    /* downloaded and written the update */
}

CMD_DECL(upgrade)
{
	bool is_there_update = 1; /* TODO:// this decision should be based on push notification */

	if ( is_there_update ) {

		if ( download_update(argc, argv, ECOS_IMAGE_BASE) ) {	/* download the update to our address 0x9f280000 */

			SHELL_ERROR ("Unable to download update\n");

		} else {

			SHELL_PRINT ("Update downloaded... Reboot!\n"); /* U-boot would boot the latest image */

		}

	}

}

void upload_caldata(int argc, char **argv)
{
    _ftp_put(argc, argv, FLASH_ADDR_CAL_DATA, FLASH_SIZE_CAL_DATA, "cal_data");
}

void upload_flash_image(int argc, char **argv)
{
    _ftp_put(argc, argv, QCA953X_FLASH_START, QCA953X_FLASH_SIZE, "flash_image");
}

shell_cmd("upload",
	 "upload flash",
	 USAGESTR("upload <ftp server> <ftp host> <ftp password> <[cal_data] [flash_image]>"),
	 upload);

CMD_DECL(upload)
{
    if (argc != 4) {
	goto err;
    }

    if (!strcmp(argv[3], "cal_data")) {
	upload_caldata(argc, argv);
    } else if (!strcmp(argv[3], "flash_image")) {
	upload_flash_image(argc, argv);
    } else {
	goto err;
    }

    return 0;

err:

    SHELL_USAGE("Usage: upload <ftp server> <ftp host> <ftp password> <[cal_data] [flash_image]>\n");
    return SHELL_INVALID_ARGUMENT;

}

#endif /* CYGPKG_NET_FTPCLIENT */

#endif /* CONFIG_BSP_FLASH_API */

#ifdef CONFIG_BSP_UPTIME
shell_cmd("uptime",
     "Shows system uptime",
     "",
     uptime);

CMD_DECL(uptime)
{
    unsigned long sec, mn, hr, day;
    time_t timer;

    timer = time(NULL);
    sec = (unsigned long)(timer);

    day = sec / 86400;
    sec %= 86400;
    hr = sec / 3600;
    sec %= 3600;
    mn = sec / 60;
    sec %= 60;

    SHELL_PRINT("%ldday %ldh %ldm %lds\n", day, hr, mn, sec);
    return SHELL_OK;
}
#endif /* UPTIME */

#ifdef CONFIG_BSP_WD

shell_cmd("wd",
          "Watchdog setup",
          USAGESTR("wd on/off/res/reboot"),
          watchdog_cmd);


CMD_DECL(watchdog_cmd)
{
    if (argc == 1) {
        if (strcmp((char*)argv[0], "on") == 0) {
            watchdog_reset();
            watchdog_start();
        }
        else if (strcmp((char*)argv[0], "off") == 0) {
            watchdog_reset();
        }
        else if (strcmp((char*)argv[0], "res") == 0) {
            unsigned long long wres = watchdog_get_resolution();
            SHELL_PRINT("%lld ms\n", wres/1000/1000);
        }
        else if (strcmp((char*)argv[0], "reboot") == 0) {
			watchdog_reboot();
        }
    }else{
	
	SHELL_USAGE("usage: wd on/off/res/reboot\n");
}
    return SHELL_OK;
}
#endif /* WD */


#ifdef CONFIG_BSP_SORT

shell_cmd("sort",
	 "Show Threads by Priority",
	 "",
	 psr);

CMD_DECL(psr)
{
    get_thread_descend();

    return SHELL_OK;
}
#endif /* SORT */

#define CONFIG_BSP_TDUMP
#ifdef CONFIG_BSP_TDUMP
#ifdef CYGDBG_HAL_CRASHDUMP

shell_cmd("tdump",
	 "Show Threads status",
	 "",
	 shell_tdump);

CMD_DECL(shell_tdump)
{
    int tid;
    if (argc != 1) {
		SHELL_USAGE("Syntax: tdump <tid>\n");
		return SHELL_INVALID_ARGUMENT;
    }

    tid = simple_strtoul((char *)argv[0], NULL, 10);

    HAL_SavedRegisters *regs;

    regs = (HAL_SavedRegisters *) get_thread_ptregs(tid);
    if (regs == NULL) {
		SHELL_ERROR("%s: invalid threadid\n");
		return SHELL_INVALID_ARGUMENT;
    }

    tdump((void *)regs->d[29], (void *)regs->d[31], tid);

    return SHELL_OK;
}

#endif /* CYGDBG_HAL_CRASHDUMP */

#endif /* CONFIG_BSP_TDUMP */

