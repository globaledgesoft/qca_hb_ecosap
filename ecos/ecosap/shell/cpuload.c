//=================================================================
//
//        cpuload.c
//
//        CPU load shell command.
//
//==========================================================================

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


#include <pkgconf/system.h>

#ifdef CYGPKG_CPULOAD

#include <cyg/hal/hal_arch.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/testcase.h>
#include <cyg/infra/diag.h>
#include <cyg/cpuload/cpuload.h>

#include"shell_err.h"

void cpuload_cmd(void);
static cyg_uint32 calibration;
static cyg_cpuload_t cpuload;
static cyg_handle_t handle;
  
void
cpuload_thread(cyg_addrword_t data)
{
#ifdef NOTUSED
    cyg_uint32 i, j;
    static int onetime = 0;
#endif
  
    cyg_cpuload_calibrate(&calibration);
  
    cyg_cpuload_create(&cpuload, calibration, &handle);
  
    cpuload_cmd();
}

void zero_load(void)
{
    int i;

    for (i = 350; i--; ) {
        cyg_thread_delay(10);
        cpuload_cmd();
    }
}

void cpuload_init(void)
{
    static char stack[CYGNUM_HAL_STACK_SIZE_MINIMUM];
    static cyg_handle_t handle;
    static cyg_thread thread;

    cyg_thread_create(4,cpuload_thread,0,"cpuload",
                      stack,sizeof(stack),&handle,&thread);
    cyg_thread_resume(handle);
}

void cpuload_cmd(void) {
    cyg_uint32 average_point1s;
    cyg_uint32 average_1s;
    cyg_uint32 average_10s;

    cyg_cpuload_get(handle, &average_point1s, &average_1s, &average_10s);
    SHELL_PRINT("cpu load %d /100 msec, %d /sec, %d /10sec\n",
                average_point1s, average_1s, average_10s);
}

#endif /* CYGPKG_CPULOAD */
