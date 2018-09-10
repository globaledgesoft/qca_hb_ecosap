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
#include <cyg/hal/var_intr.h>
#include <cyg/kernel/kapi.h>
#include<stdlib.h>

#include <commands.h>
#include <shell_err.h>

static const char * const thread_state_str[] = {
    "RUN",	/* 0 - Running */
    "SLEEP",	/* 1 - Sleeping */
    "CNTSLP",	/* 2 - Counted sleep */
    "SLPSET",	/* 3 - Sleep set */	
    "SUSP",	/* 4 - Suspended */
    NULL,	/* 5 - INVALID */
    NULL,	/* 6 - INVALID */
    NULL,	/* 7 - INVALID */
    "CREAT",	/* 8 - Creating */
    NULL,	/* 9 - INVALID */
    NULL,	/* 10 - INVALID */
    NULL,	/* 11 - INVALID */
    NULL,	/* 12 - INVALID */
    NULL,	/* 13 - INVALID */
    NULL,	/* 14 - INVALID */
    NULL,	/* 15 - INVALID */
    "EXIT"	/* 16 - Exiting */
};

/*
 * This function produces a list of the threads
 * that are currently scheduled.  The output is roughly
 * analagous to 'ps', with the exception of VSS for obvious
 * reasons
 */

void prettyprint(void)
{
    SHELL_PRINT("%-8s %-2s %-6s %-15s %-2s %-2s %-10s %-10s %-10s\n",
	       "-------",
	       "--",
	       "------",
	       "----------------------",
	       "--",
	       "--",
	       "----------",
	       "----------",
	       "----------");
    SHELL_PRINT("%-8s %-2s %-6s %-22s %-2s %-2s %-10s %-10s %-10s\n",
	       "Handle",
	       "ID",
	       "State",
	       "Name",
	       "SP",
	       "CP",
	       "Stack Base",
	       "Stack Size",
	       "Stack Used");
    SHELL_PRINT("%-8s %-2s %-6s %-22s %-2s %-2s %-10s %-10s %-10s\n",
	       "-------",
	       "--",
	       "------",
	       "----------------------",
	       "--",
	       "--",
	       "----------",
	       "----------",
	       "----------");

	return ;
}

void print_thread_info(cyg_thread_info *tinfo)
{
	SHELL_PRINT("%-8x %-2d %-6s %-22s %-2d %-2d 0x%08x 0x%08x 0x%08x\n",
		   tinfo->handle,
		   tinfo->id,
		   thread_state_str[tinfo->state],
		   tinfo->name,
		   tinfo->set_pri,
		   tinfo->cur_pri,
		   tinfo->stack_base,
		   tinfo->stack_size,
		   tinfo->stack_used);

}

void get_thread_info(void)
{
    cyg_handle_t thread = 0;

    cyg_uint16 id = 0;
    
	int total_stack_size = 0, total_stack_used = 0;

	prettyprint();

	while( cyg_thread_get_next( &thread, &id ) ) {

			cyg_thread_info tinfo;

			if( !cyg_thread_get_info( thread, id, &tinfo ) )
					break;

			print_thread_info(&tinfo);

			total_stack_size += tinfo.stack_size;

			total_stack_used += tinfo.stack_used;
	}

	SHELL_PRINT("\n\n");
    
	SHELL_PRINT("Total Stack Size:  %d\n", total_stack_size);

    SHELL_PRINT("Total Stack Used:  %d\n", total_stack_used);
}



void cyg_thread_by_prio(int prio)
{
    cyg_handle_t thread = 0;

    cyg_uint16 id = 0;

    while( cyg_thread_get_next( &thread, &id ) )
    {
        cyg_thread_info info;

        if( !cyg_thread_get_info( thread, id, &info ) )
            break;
			
			if ( prio == info.set_pri) {

				print_thread_info(&info);
		}
    }
}

int descend(void)
{

    int l = 1 ;

    for(l = 1; l <= 32 ; l++)         /*Yeah Its ugly, but okay for debugging purpose */
        cyg_thread_by_prio(l);
        
   return 0; 
}


void get_thread_descend(void)
{

	prettyprint();

	descend();

	return;
}
