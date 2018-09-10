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
#include <cyg/io/flash.h>
#include <cyg/io/flash_dev.h>
#include <cyg/io/io.h>
#include <string.h>
#include <stdlib.h>

enum WHOSE {
    CONFIG_DATA = 0,
#ifdef CRASHDUMP_SUPPORT
    CRASH_DUMP,
#endif
    DOMAIN_MAX // Invalid entry 
};

#define ATHR_FLASH_START 0x9f000000
#define ATHR_FLASH_SIZE 0x00100000
#define ATHR_FLASH_SECTOR 0x1000

#ifdef CRASHDUMP_SUPPORT
	#define CRASH_DUMP_ADDR (ATHR_FLASH_START + ATHR_FLASH_SIZE - (3*ATHR_FLASH_SECTOR))
#endif

#define CONFIG_DATA_ADDR (ATHR_FLASH_START + ATHR_FLASH_SIZE - (2*ATHR_FLASH_SECTOR))

unsigned int addresses[DOMAIN_MAX+1] = { 
					CONFIG_DATA_ADDR,  
				#ifdef CRASHDUMP_SUPPORT
					CRASH_DUMP_ADDR,
				#endif
					0
					};
#define SECTOR_SIZE (4 * 1024)
#define BADSIZE 99
#define BADSECTOR 98
#define NOMEM 97
#define ALLGOOD 0

int cyg_flash_setconf(void *buf, size_t size, enum WHOSE whose);

int cyg_flash_getconf(void *buf, int len, enum WHOSE whose);

int
cyg_flash_setconf(void *buf, size_t size, enum WHOSE whose)
{
    char *ram_base;
    int ret;
    cyg_flashaddr_t err_address;

    if (whose >= DOMAIN_MAX) {
        return BADSECTOR;
    }

    if (size > SECTOR_SIZE) {
        return BADSIZE;
    }

    if ((ret = cyg_flash_init(NULL))) {
        return ret;
    }

    ram_base = malloc(SECTOR_SIZE);

    if (NULL == ram_base) {
        return NOMEM;
    }

    memset(ram_base, 0xff, SECTOR_SIZE);

    memcpy(ram_base, (const void *)addresses[whose], SECTOR_SIZE);

    memcpy(ram_base, buf, size);

    if ((ret = cyg_flash_erase(addresses[whose], 1, &err_address))) {
        free(ram_base);
        return (ret);
    }

    if ((ret =
         cyg_flash_program(addresses[whose], ram_base, SECTOR_SIZE,
                           &err_address))) {
        free(ram_base);
        return (ret);
    }

    free(ram_base);
    return size;
}
int
cyg_flash_getconf(void *buf, int len, enum WHOSE whose)
{
    int ret;

    if (len > SECTOR_SIZE) {
        return BADSIZE; 
    }

    if (whose >= DOMAIN_MAX) {
        return BADSECTOR;
    }

    cyg_flashaddr_t err_address;

    if ((ret = cyg_flash_init(NULL))) {
        return (ret);
    }

    if ((ret = cyg_flash_read(addresses[whose], buf, len, &err_address))) {
        return (ret);
    }

    return ALLGOOD;
}

