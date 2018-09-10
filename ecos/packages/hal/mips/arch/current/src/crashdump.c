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

#include <cyg/hal/crashdump.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <string.h>

#ifdef CYGDBG_HAL_CRASHDUMP
#define CRASHDUMP (unsigned int)0xff

static int flag_crashdump;

extern char *_buildtag __attribute__ ((__weak__)) = "No-Tag-info";
extern char *_build_version_tag __attribute__ ((__weak__)) =
    "No-version-Tag-info";
extern char *_build_version_hash __attribute__ ((__weak__)) =
    "No-version-Hash-info";
extern char *_compiler_release __attribute__ ((__weak__)) = "No-Compiler-info";

static const char * const thread_state_str[] = {
    "RUN",      /* 0 - Running */
    "SLEEP",    /* 1 - Sleeping */
    "CNTSLP",   /* 2 - Counted sleep */
    "SLPSET",   /* 3 - Sleep set */
    "SUSP",     /* 4 - Suspended */
    NULL,       /* 5 - INVALID */
    NULL,       /* 6 - INVALID */
    NULL,       /* 7 - INVALID */
    "CREAT",    /* 8 - Creating */
    NULL,       /* 9 - INVALID */
    NULL,       /* 10 - INVALID */
    NULL,       /* 11 - INVALID */
    NULL,       /* 12 - INVALID */
    NULL,       /* 13 - INVALID */
    NULL,       /* 14 - INVALID */
    NULL,       /* 15 - INVALID */
    "EXIT"      /* 16 - Exiting */
};

static void
print_thread_info(cyg_thread_info * tinfo)
{
    diag_printf("%-8x %-2d %-6s %-22s %-2d %-2d 0x%08x 0x%08x 0x%08x\n",
                tinfo->handle,
                tinfo->id,
                thread_state_str[tinfo->state],
                tinfo->name,
                tinfo->set_pri,
                tinfo->cur_pri,
                tinfo->stack_base, tinfo->stack_size, tinfo->stack_used);

}

static void
prettyprint(void)
{
    diag_printf("%-8s %-2s %-6s %-15s %-2s %-2s %-10s %-10s %-10s\n",
                "-------",
                "--",
                "------",
                "----------------------",
                "--", "--", "----------", "----------", "----------");
    diag_printf("%-8s %-2s %-6s %-22s %-2s %-2s %-10s %-10s %-10s\n",
                "Handle",
                "ID",
                "State",
                "Name", "SP", "CP", "Stack Base", "Stack Size", "Stack Used");
    diag_printf("%-8s %-2s %-6s %-22s %-2s %-2s %-10s %-10s %-10s\n",
                "-------",
                "--",
                "------",
                "----------------------",
                "--", "--", "----------", "----------", "----------");

    return;
}

int
get_prev_sp_ra(void **prev_sp, void **prev_ra,
               void *sp, void *ra, int *funcaddrs)
{
    unsigned int *wra = (unsigned int *)ra;
    int spofft = 0;

    if (ra == NULL || sp == NULL)
        return 0;
    /* scan towards the beginning of the function -
       addui sp,sp,spofft should be the first command */
    while((*wra >> 16) != (0xafbf)) {
	if((*wra >> 16) == (0x27bd))
	    if((((int)*wra << 16) >> 16) < 0)
	    	spofft += (((int)*wra << 16) >> 16); /* sign-extend */
	wra--;
    }
    while ((*wra >> 16) != (0x27bd)) {  /* addiu sp, sp, -0xXX */
        /* test for "scanned too much" elided */
	wra--;
    }

    *funcaddrs = wra;
    spofft += (((int)*wra << 16) >> 16); /* sign-extend */
    *prev_sp = (char *)sp - spofft;
    /* now scan forward for sw r31,raofft(sp) */
    while (wra < (unsigned int *)ra) {
        if ((*wra >> 16) == 0xafbf) {   /* sw r31, 0xXX(sp) */
            int raoffset = (((int)*wra << 16) >> 16);   /* sign */
            *prev_ra = *(void **)((char *)sp + raoffset);

            return 1;
        }
        wra++;
    }
    return 0;                   /* failed to find where ra is saved */
}

int
get_call_stack_no_fp(void *sp, void *ra, int *retaddrs, int *stackaddrs,
                     int *funcaddrs, int max_size)
{
    /* adjust sp by the offset by which this function   */
    /* has just decremented it */
    int *funcbase = (int *)(int)&get_call_stack_no_fp;

    /* funcbase points to an addiu sp,sp,spofft command */
    int spofft = (*funcbase << 16) >> 16;   /* 16 LSBs */
    int i = 0;

    do {
        if (i < max_size) {
            retaddrs[i] = (int *)ra;
            stackaddrs[i] = (int *)sp;
        } else {
            return i;
        }
	if (ra < 0x80000000 || ra > 0x80800000)
	    return i;
        i++;

    } while ((get_prev_sp_ra(&sp, &ra, sp, ra, funcaddrs + i)));

    return i;                   /* stack size */
}

void
tdump(void *sp, void *ra, cyg_uint8 tid)
{
    int iter = 0, stackptr = sp;
    unsigned int stackaddrs[MAX_FRAME_DEPTH] = { }, retaddrs[MAX_FRAME_DEPTH] = {
    }, funcaddrs[MAX_FRAME_DEPTH] = {
    };
    size_t max_size = MAX_FRAME_DEPTH;  // MAX_FRAME_DEPTH 12

    cyg_handle_t current;
    cyg_thread_info current_thread_info;

    if (tid == CRASHDUMP) {
        current = cyg_thread_self();    //tid != CRASHDUMP ? cyg_thread_find(tid) : cyg_thread_self();
    }
    else {
        current = cyg_thread_find(tid);
    }

    diag_printf("Thread Information:\n");
    cyg_thread_get_info((current), cyg_thread_get_id(current),
                        &current_thread_info);

    prettyprint();
    print_thread_info(&current_thread_info);

    diag_printf("\n");
    diag_printf("Stack : ");

    for (iter = 0; iter <= current_thread_info.stack_used / sizeof(int *); iter++) {
        if (iter % 10 == 0)
            diag_printf("\n");
        diag_printf("%p ", *((int *)stackptr + iter));
    }

    diag_printf("\n\n");

    cyg_kmem_print_stats();

    diag_printf("\n\n");

    int frames =
        get_call_stack_no_fp(sp, ra, retaddrs, stackaddrs, funcaddrs,
                             MAX_FRAME_DEPTH);

    diag_printf("Call trace:\n");
    for (iter = 0; iter < frames; iter++) {
        diag_printf("STACKPTR: %p ", stackaddrs[(iter)]);
        diag_printf("RAOFFT: %p ", retaddrs[iter] - funcaddrs[iter + 1]);
        diag_printf("RA: %p ", retaddrs[(iter)]);
        diag_printf("FUNC_ADDR: %p \n", funcaddrs[iter]);
    }
    diag_printf("\n\n");

    return;
}

void
crashdump(HAL_SavedRegisters * regs)
{
    int iter = 0;
    void (*reset) (void);
    char *exception[] = { "", "TLB modification exception",
        "TLB exception (load or instruction fetch)",
        "TLB exception (store)",
        "Address error exception (load or instruction fetch)",
        "Address error exception (store)",
        "Bus error exception (instruction fetch)",
        "Bus error exception (data reference: load or store)",
        "Syscall exception",
        "Breakpoint exception",
        "Reserved instruction exception",
        "Coprocessor Unusable exception",
        "Arithmetic Overflow exception",
        "Trap exception",
        "Reserved",
        "Floating point exception",
    };

    char *reg[] = { "zero", "at", "v0", "v1",
        "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3",
        "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3",
        "s4", "s5", "s6", "s7",
        "t8", "t9", "k0", "k1",
        "gp", "sp", "fp", "ra",
    };

    reset = (void (*)(void))RESET_ENTRY_ADDR;

    if (flag_crashdump == 1) {
        reset();
    }
#ifdef DEBUG

    diag_printf("\n");
    diag_printf("Build Tag %s\n", _buildtag);
    diag_printf("Build Version %s\n", _build_version_tag);
    diag_printf("Build Hash %s\n", _build_version_hash);
    diag_printf("Using %s\n", _compiler_release);

    diag_printf("------------------------Exception------------------------\n");
    if ((regs->cause >> 2 & 0x1f) > 0 && (regs->cause >> 2 & 0x1f) < 16)
        diag_printf("Type: %s\n", exception[(unsigned int)(regs->cause) >> 2 & 0x1f]);
    diag_printf("Data Regs:\n\n");

    for (iter = 0; iter <= 31; iter++) {
        if (iter != 0 && iter % 6 == 0)
            diag_printf("\n\n");
        diag_printf("D[%d]: 0x%08x\t", iter, regs->d[(iter)]);
    }

    diag_printf("\n\n");
    prints_regs(PC:, regs->pc, \t\t);
    prints_regs(RA:, regs->d[(RA)], \t\t\t);
    prints_regs(SP:, regs->d[(SP)], \n);
    prints_regs(FP:, regs->d[(FP)], \t\t);
    prints_regs(GP:, regs->d[(GP)], \t\t\t);
    prints_regs(CAUSE:, regs->cause, \n);
    prints_regs(HI:, regs->hi, \t\t);
    prints_regs(LO:, regs->lo, \t\t\t);
    prints_regs(VECTOR:, regs->vector, \n);
    prints_regs(SR:, regs->sr, \t\t);
    prints_regs(CACHE:, regs->cache, \t\t);
    prints_regs(CONFIG:, regs->config, \n);

    diag_printf("\n\n");
    for (iter = 0; iter <= 31; iter = iter + 4) {
        diag_printf("\t\t%s\t\t%s\t\t%s\t\t%s\n", reg[iter], reg[iter + 1],
                    reg[iter + 2], reg[iter + 3]);
        diag_printf("$%02d\t: %p\t%p\t%p\t%p\n", iter, regs->d[iter],
                    regs->d[iter + 1], regs->d[iter + 2], regs->d[iter + 3]);
    }
#endif

    flag_crashdump = 1;

    tdump(regs->d[SP], regs->pc, CRASHDUMP);

    reset();

    return;                     //control does not come here
}

#endif // CYGDBG_HAL_CRASHDUMP
