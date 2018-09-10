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

#ifndef _APWLAN_H_
#define _APWLAN_H_

#include <cyg/dbg_print/dbg_print.h>

#define MAX_CMD_SIZE	128

int run_command(char *cmd_buf, int (*func)(int argc, char *argv[]));
int wlan_config(int argc, char *argv[]);
int iw_config(int argc, char *argv[]);
int iw_priv(int argc, char *argv[]);
int hostapd_cmds(int argc, char *argv[]);
int wpa_supplicant_cmds(int argc, char *argv[]);
extern int ifconfig_main(int argc, char *argv[]);
extern int brconfig_main(int argc, char *argv[]);
int makevap(int argc, char *argv[]);
int activatevap(int argc, char* argv[]);
int apwlan_start(void);
void apwlan_stop(void);
int wlan_acfg_cli(int argc, unsigned char *argv[]);

#endif // _APWLAN_H_
