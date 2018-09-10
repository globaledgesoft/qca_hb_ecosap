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

#ifndef _DUMPREGS_H_
#define _DUMPREGS_H_

#include <cyg/hal/hal_arch.h>
#include <cyg/kernel/kapi.h>
#include <pkgconf/system.h>

#define CDUMP                   // define to enable CRASH DUMP
#define DEBUG
#define RESET_ENTRY_ADDR 0x9f000000
#define STACK_SIZE 120
#define prints_regs(REG, DATA, ES) diag_printf(#REG" 0x%08x"#ES, DATA)
#define MAGICNUMBER 0xDEAD0FFF
#define MAX_FRAME_DEPTH 6
#define RA 31
#define SP 29
#define FP 30
#define GP 28

struct cprm {
    unsigned int magicnumber;
    HAL_SavedRegisters pt_regs;
    unsigned int stack[STACK_SIZE];
};

void dumpregs(HAL_SavedRegisters * regs);

#endif /* _DUMPREGS_H_ */
