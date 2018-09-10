//==========================================================================
//
//      ftpclient.c
//
//      Simple FTP Client
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.
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
// Author(s):    andrew.lunn
// Contributors: andrew.lunn, jskov
// Date:         2001-09-18
// Purpose:      
// Description:  Test FTPClient functions. Note that the _XXX defines below
//               control what addresses the test uses. These must be
//               changed to match the particular testing network in which
//               the test is to be run.
//              
//####DESCRIPTIONEND####
//
//==========================================================================

#include <network.h>
#include <cyg/io/flash.h>
#include <cyg/io/flash_dev.h>
#include <cyg/io/io.h>
#include <cyg/infra/testcase.h>
#include <pkgconf/system.h>
#include <shell_err.h>

#if (defined CONFIG_BSP_FLASH_API) && (defined CYGPKG_NET_FTPCLIENT)

#include <ftpclient.h>

#define __string(_x) #_x

#define __xstring(_x) __string(_x)

#define _FTP_SRV           __xstring(IP address)   // farmnet dns0 address

#define _FTP_SRV_V6         __xstring(fec0:0:0:2::1)

#define _FTP_USR "User name"			// ftp username

#define _FTP_PASSWD "password"			// ftp password 

#define IMAGESIZE 20				// Max Number of bytes in "size" file(image size)

#define DEBUG_FTP

int _ftp_put(int argc, char **argv, void *addr, size_t size, char *file_name)
{
    int ret;

    ipfw_add_ftp_rules();

    ret = ftp_put(argv[0], argv[1], argv[2], file_name, addr, size, ftpclient_printf);

    ipfw_del_ftp_rules();

    return ret;
}

int
_ftp_get(int argc, char **argv)
{

    int ret, size;

    char ftpbuf[IMAGESIZE];

    char *imagebuf;

    SHELL_PRINT("Getting image size from ", argv[(0)]);

    ipfw_add_ftp_rules();

    ret = ftp_get(argv[0], argv[1], argv[2],
                  "size", ftpbuf, IMAGESIZE, ftpclient_printf);

    if (ret > 0) {

        SHELL_PRINT("PASS:< %d bytes received>\n", ret);

    } else {

        SHELL_PRINT("FAIL:< ftp_get returned %d>\n", ret);

	return -1;
    }
#ifdef DEBUG_FTP
    SHELL_PRINT("SIZE %s \n", ftpbuf);
#endif

    ftpbuf[IMAGESIZE - 1] = '\0';

    size = strtoul(ftpbuf, NULL, 10);

#ifdef DEBUG_FTP
    SHELL_PRINT("size %u\n", size);
#endif

    imagebuf = (char *)malloc(size);

    if (imagebuf == NULL) {

        SHELL_PRINT("malloc error \n");

        return -1;
    }

    ret = ftp_get(argv[0], argv[1], argv[2],
                  "image", imagebuf, size + 10, ftpclient_printf);
    if (ret > 0) {

        SHELL_PRINT("PASS:< %d bytes received>\n", ret);

    } else {

        SHELL_PRINT("FAIL:< ftp_get returned %d>\n", ret);

	free (imagebuf);

        return -1;

    }

#ifdef DEBUG_FTP
	printf ("data from file %s\n", imagebuf);
#endif

    ipfw_del_ftp_rules();

    cyg_scheduler_lock();

    if ((ret = cyg_flash_init(NULL))) {

	free (imagebuf);
	cyg_scheduler_unlock();	

	return ret;
    }

    if ( (ret = cyg_flash_erase(ECOS_IMAGE_BASE, size, NULL))) {

	free (imagebuf);
	cyg_scheduler_unlock();	

	return ret;
    }

    if (( ret = cyg_flash_program(ECOS_IMAGE_BASE, imagebuf, size, NULL))) {
		
	free (imagebuf);
	cyg_scheduler_unlock();

	return ret;	
    }
	
    free (imagebuf);

    cyg_scheduler_unlock();	

    return 0;
}

#endif /* CYGPKG_NET_FTPCLIENT */ 


