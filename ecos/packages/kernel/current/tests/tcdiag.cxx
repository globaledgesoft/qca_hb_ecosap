//==========================================================================
//
//        tcdiag.cxx
//
//        Kernel diag test harness.
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):     dsm
// Contributors:    dsm
// Date:          1998-03-17
// Description:   Test harness implementation that uses the kernel's
//                diag channel.  This is intended for manual testing
//                of the kernel.
//####DESCRIPTIONEND####

#include <cyg/infra/testcase.h>
#include <cyg/infra/diag.h>


void cyg_test_init()
{
    diag_init();
}

void cyg_test_output(int status, char *msg, int line, char *file)
{
    char *st;

    switch (status) {
    case 0:
        st = "FAIL:";
        break;
    case 1:
        st = "PASS:";
        break;
    case 2:
        st = "EXIT:";
        break;
    case 3:
        st = "INFO:";
        break;
    }

    diag_write_string(st);
    diag_write_char('<');
    diag_write_string(msg);
    diag_write_string("> Line: ");
    diag_write_dec(line);
    diag_write_string(", File: ");
    diag_write_string(file);
    diag_write_char('\n');

}

// This is an appropriate function to set a breakpoint on
void cyg_test_exit()
{
    for(;;)
        ;
}
// EOF tcdiag.cxx
