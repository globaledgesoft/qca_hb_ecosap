/*
 * Copyright (c) 2005, 2006
 *
 * James Hook (james@wmpp.com)
 * Chris Zimman (chris@wmpp.com)
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
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include <cyg/hal/qca953x_gpio.h>
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

#define INTR_STATS
#include "shell.h"
#include "misc.h"
#include "shell_err.h"
#include "shell_thread.h"
#include "commands.h"

void print_eloop_ctx_stats(void);
void print_eloop_stats(void);
void cyg_kmem_print_stats( void );
void get_mempool_stats(void);
void get_thread_info(void);
extern int brconfig_main(int argc, char *argv[]);

int ag7100_count(int unit, int tx);




shell_cmd("dma_stat",
	"Read dma stat value",
	"",
	read_dma_reg);

shell_cmd("wr",
     "Write register",
     "",
     write_reg);
shell_cmd("rr",
     "Read register",
     "",
     read_reg);

CMD_DECL(read_reg)
{
    if (argc < 1 ) {
        printf ("rr <reg>\n");
        return 1 ;
    }
    printf ("%s = %x\n", argv[0], ath_reg_rd(simple_strtoul((const char *)argv[0], NULL, 16)));
    return SHELL_OK;
}

CMD_DECL(write_reg)
{
    if (argc < 2 ) {
        printf ("wr <reg> <value>\n");
        return 1 ;
    }
    ath_reg_wr( simple_strtoul((const char *)argv[0], NULL, 16),  simple_strtoul((const char *)argv[1], NULL, 16));
    return SHELL_OK;
}

CMD_DECL(read_dma_reg)
{
    printf ("DMARXSTATUS GMAC0= %x\n", ath_reg_rd(0x19000194));
    printf ("DMARXSTATUS GMAC1= %x\n", ath_reg_rd(0x1A000194));
    printf ("DMATXSTATUS GMAC0= %x\n", ath_reg_rd(0x19000188));
    printf ("DMATXSTATUS GMAC1= %x\n", ath_reg_rd(0x1A000188));
    return SHELL_OK;
}

shell_cmd("desc",
	  "Check eth drv desc stats",
	  "",
	  check_desc_cnt);

shell_cmd("help",
	 "Displays a list of commands",
	 "",
	 help_func);

shell_cmd("?",
	 "Displays a list of commands",
	 "",
	 help_func2);

shell_cmd("version",
	 "Shows build version",
	 "",
	 print_build_tag);

#ifdef ALLOC_NODE_FIXED
shell_cmd("npool",
        "Show mempool stats",
        "",
        mempool_stats);

CMD_DECL(mempool_stats)
{
    get_mempool_stats();
	
    return SHELL_OK;
}
#endif

shell_cmd("regdump",
	"dumping reg values",
	"",
	regdump);

CMD_DECL(regdump)
{
    
    cyg_handle_t system_clock;

    cyg_tick_count_t ticks;
                           
    system_clock = cyg_real_time_clock();

    ticks = cyg_current_time(); 

    unsigned int cp0_reg;

    diag_printf("tick=%llu\n", ticks);

    cp0_reg = __read_32bit_c0_register($12, 0);

    diag_printf("status register:%x\n", cp0_reg);

            cp0_reg = __read_32bit_c0_register($13, 0);

    diag_printf("cause register:%x\n", cp0_reg);

            cp0_reg = __read_32bit_c0_register($9, 0);

    diag_printf("count register:%x\n", cp0_reg);

            cp0_reg = __read_32bit_c0_register($11, 0);

    diag_printf("compare register:%x\n", cp0_reg);

            cp0_reg = __read_32bit_c0_register($15, 1);

    diag_printf("base register:%x\n", cp0_reg);

    return SHELL_OK;
}

shell_cmd("ifconfig",
          "config tool for bsd tcp/ip",
          "",
          ifconfig_cmd);

CMD_DECL(ifconfig_cmd){
	ifconfig_main(argc,argv);
	return SHELL_OK;
}

#ifdef IPFLOW_NAT_SHOW_ENABLE
shell_cmd("ipflow_nat_show","ipflow","",ipflow_nat_show_cmd);
CMD_DECL(ipflow_nat_show_cmd){
        ipflow_nat_show();
        return SHELL_OK;
}

shell_cmd("alias_show","alias","",alias_show_cmd);
CMD_DECL(alias_show_cmd){
        alias_show();
        return SHELL_OK;
}

shell_cmd("alias_ipflow_show","alias","",alias_ipflow_show_cmd);
CMD_DECL(alias_ipflow_show_cmd){
        ipflow_nat_show();
        alias_show();
        return SHELL_OK;
}
#endif

CMD_DECL(check_desc_cnt) {
    diag_printf("Tx descriptor Stats for eth 0\n");
    diag_printf("eth0 txcount=%d\n\n", ag7100_count(0,1));
    diag_printf("Tx descriptor Stats for eth 1\n");
    diag_printf("eth1 txcount=%d\n\n", ag7100_count(1,1));
    diag_printf("Rx descriptor Stats for eth 0\n");
    diag_printf("eth1 rxcount=%d\n\n", ag7100_count(0,0));
    diag_printf("Rx descriptor Stats for eth 1\n");
    diag_printf("eth1 rxcount=%d\n\n", ag7100_count(1,0));
    return SHELL_OK;
}

#ifdef CYGPKG_CPULOAD
shell_cmd("cpuload",
	"cpu load",
	"",
	cpuload_cmd);

#endif

#ifdef CONFIG_SHELL_PS

shell_cmd("ps",
	 "Shows a list of threads",
	 "",
	 ps);

CMD_DECL(ps)
{
    get_thread_info();

    return SHELL_OK;
}
#endif /* PS */

#ifdef CONFIG_SHELL_SP

shell_cmd("sp",
	 "Sets a threads priority",
	 "[thread ID]",
	 set_priority);

CMD_DECL(set_priority)
{

    cyg_handle_t thandle = 0;

    cyg_priority_t cur_pri, set_pri;


    if (argc < 2) {

		SHELL_PRINT("Usage: sp [tid] [priority]\n");

		return 0;
	}

	thandle = cyg_thread_find(simple_strtoul((const char *)argv[0], NULL, 10));

	set_pri = simple_strtoul((const char *)argv[1], NULL, 0);

	if ( set_pri > 32 || set_pri < 1 || thandle == 0 ) {

		SHELL_PRINT("Incorrect Values\n");

		return 0;
	}

	cur_pri = cyg_thread_get_current_priority(thandle);

	SHELL_PRINT("Changing thread %x priority from %d to %d\n", thandle, cur_pri, set_pri);

	cyg_thread_set_priority(thandle, set_pri);

	cur_pri = cyg_thread_get_current_priority(thandle);

	SHELL_PRINT("Thread %x priority now @ %d\n", thandle, cur_pri);

    return SHELL_OK;
}
#endif /* SP */


CMD_DECL(help_func)
{
    ncommand_t *shell_cmd = __shell_CMD_TAB__;

    const char cmds[] = "Commands", dsr[] = "Descriptions";
    const char usage[] = "Usage", location[] = "File Location";
    unsigned char helpar[sizeof(cmds) + sizeof(dsr) + sizeof(usage) + sizeof(location) + 10 ];
    unsigned char i;

    snprintf((char *)helpar, sizeof(helpar) - 1, "%%-11s %%-60s %%-20s %%-20s\n");

    SHELL_PRINT((char *)helpar, cmds, dsr, usage, location);

    for(i = 0; i < sizeof(cmds) - 1; i++) putchar('-');
    SHELL_PRINT("    ");
    for(i = 0; i < sizeof(dsr) - 1; i++) putchar('-');
    SHELL_PRINT("                                                 ");
    for(i = 0; i < sizeof(usage) - 1; i++) putchar('-');
    SHELL_PRINT("                ");
    for(i = 0; i < sizeof(location) - 1; i++) putchar('-');
    putchar('\n');

    while(shell_cmd != &__shell_CMD_TAB_END__) {
	SHELL_PRINT("%-11s %-60s %-20s %-20s\n",
		   shell_cmd->name,
		   shell_cmd->descr,
		   shell_cmd->usage,
		   shell_cmd->file);
	shell_cmd++;
    }

    return SHELL_OK;
}

CMD_DECL(help_func2)
{
    return(help_func(argc, argv));
}

#ifdef CONFIG_SHELL_DUMP

shell_cmd("dump",
	 "Shows a memory dump",
	 "",
	 hexdump);

CMD_DECL(hexdump)
{
    unsigned int i = 0, j = 0;
    unsigned int len = 100, width = 16;
    unsigned char *buf = NULL;
    char *cp = NULL;

    switch(argc) {

    case 1:
	buf = (unsigned char *)simple_strtoul((const char *)argv[0], &cp, 0);

	if(cp && *cp) {
	    SHELL_PRINT("Value '%s' is not a valid address\n", argv[0]);
	    return SHELL_OK;
	}

	break;

    case 2:
	buf = (unsigned char *)simple_strtoul((const char *)argv[0], &cp, 0);

	if(cp && *cp) {
	    SHELL_PRINT("Value '%s' is not a valid address\n", argv[0]);
	    return SHELL_OK;
	}

	len = simple_strtoul((const char *)argv[1], &cp, 0);

	if(cp && *cp) {
	    SHELL_PRINT("Value '%s' is not a valid length\n", argv[1]);
	    return SHELL_OK;
	}

	break;

    case 3:
	buf = (unsigned char *)simple_strtoul((const char *)argv[0], &cp, 0);

	if(cp && *cp) {
	    SHELL_PRINT("Value '%s' is not a valid address\n", argv[0]);
	    return SHELL_OK;
	}

	len = simple_strtoul((const char *)argv[1], &cp, 0);

	if(cp && *cp) {
	    SHELL_PRINT("Value '%s' is not a valid length\n", argv[1]);
	    return SHELL_OK;
	}

	width = simple_strtoul((const char *)argv[2], &cp, 0);

	if(cp && *cp) {
	    SHELL_PRINT("Value '%s' is not a valid width\n", argv[2]);
	    return SHELL_OK;
	}

	break;

    default:
	SHELL_PRINT("Usage: hexdump [address] [length] [width]\n");
	return SHELL_OK;
    }

    SHELL_PRINT("%08X: ", (unsigned int)buf);

    for(i = 0; i < len; i++) {
	if(i && !(i % width)) {
	    j = i - width;
	    SHELL_PRINT("\t\t");
	    for(; j < i; j++) SHELL_PRINT("%c", isprint(buf[j]) ? buf[j] : '.');
	    SHELL_PRINT("\n%08X: ", (unsigned int)buf + i);
	    j = 0;
	}
	SHELL_PRINT("%02X ", buf[i]);
	j++;
    }

    if(j) {
	for(i = width - j; i > 0; i--) SHELL_PRINT("   ");
	SHELL_PRINT("\t\t");
	for(i = len - j; i < len; i++) SHELL_PRINT("%c", isprint(buf[i]) ? buf[i] : '.');
    }
    SHELL_PRINT("\n");

    return SHELL_OK;
}
#endif /* DUMP */

#ifdef CONFIG_SHELL_THREAD_KILL

shell_cmd("kill",
	 "Kills a running thread",
	 "[thread ID]",
	 thread_kill);


CMD_DECL(thread_kill)
{
    cyg_handle_t th;

    if(argc != 1) {
	SHELL_DEBUG_PRINT("Usage: kill [tid]\n");
	return SHELL_OK;
    }

	th = cyg_thread_find(simple_strtoul((const char *)argv[0], NULL, 10));

	if ( th == 0 ){
			printf ("Input Proper Value\n");
			return -1;
	}


    cyg_thread_kill(th);
    cyg_thread_delete(th);

    return SHELL_OK;
}
#endif /* THREAD_KILL */

#ifdef CONFIG_SHELL_TIMER_ON

shell_cmd("timeron",
	 "Enables the timer interrupt",
	 "",
	 timer_on);

CMD_DECL(timer_on)
{
    SHELL_PRINT("Turning timer interrupt on\n");
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_RTC);

    return 0;
}
#endif /* TIMER_ON */

#ifdef CONFIG_SHELL_TIMER_OFF

shell_cmd("timeroff",
	 "Disables the timer interrupt",
	 "",
	 timer_off);

CMD_DECL(timer_off)
{
    SHELL_PRINT("Turning timer interrupt off\n");

    cyg_interrupt_mask(CYGNUM_HAL_INTERRUPT_RTC);

    return 0;
}
#endif /* TIMER_OFF */


#ifdef CONFIG_SHELL_NETMEMINFO
shell_cmd("nmeminfo",
          "Display network memory status!",
          "",
          nmeminfo);

CMD_DECL(nmeminfo)
{
	extern void display_adf_timer_stats(void);
		cyg_kmem_print_stats();
		display_adf_timer_stats();
		print_wlan_thread_stats();
		print_eloop_stats();
		print_eloop_ctx_stats();
    return SHELL_OK;
}

#endif /* CONFIG_SHELL_NETMEMINFO */

#ifdef CONFIG_SHELL_REBOOT

shell_cmd("reboot",
     "Reboots Board",
     "",
     _reboot);

CMD_DECL(_reboot)
{
	printf ("Rebooting the board\n");

    watchdog_reboot(); /* simple Yeah ?? */
    return SHELL_OK;
}
#endif /* _REBOOT */

