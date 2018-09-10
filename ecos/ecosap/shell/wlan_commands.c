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

#include <stdlib.h>
#include "shell.h"
#include "shell_err.h"
#include "shell_thread.h"
#include "commands.h"
#include "ecosap_nw_config.h"
#include "apwlan.h"
#include "pkgconf/devs_wlan_qca953x.h"

#include "ecosap_nw_http.h"

#include <net/if.h>

#if defined(CYGPKG_COMPONENT_WLAN_CONFIG_TOOLS) && defined(CONFIG_WLAN_WLANCONFIG)
shell_cmd("wlanconfig",
	 "wlan utility",
	 "",
	 wlanconfig);

CMD_DECL(wlanconfig)    { return wlan_config(argc, (char **)argv); }
#endif

#ifdef RX_PROFILING
shell_cmd("profiling",
	  "RX PROFILING",
	  "",
	  profiling);

CMD_DECL(profiling) {
    if (argc >= 1 ) {
       if (strcmp((char*)argv[0], "show") == 0) {
            return rx_profiling();
        } else if (strcmp((char*)argv[0], "reset") == 0) {
            return rx_reset();
        }
    }
    return rx_profiling();
}

shell_cmd("print_amsdu_info",
      "count msdu",
      "",
      print_amsdu_info);

CMD_DECL(print_amsdu_info) {
    if (argc >= 1 ) {
       if (strcmp((char*)argv[0], "show") == 0) {
            return no_sub_frames();
        } else if (strcmp((char*)argv[0], "reset") == 0) {
            return reset_no_sub_frames();
        }
    }
    return no_sub_frames();
}
#endif
#if defined(CYGPKG_COMPONENT_WLAN_WIRELESS_TOOLS) && defined(CONFIG_WLAN_IWCONFIG)
shell_cmd("iwconfig",
	 "wlan utility",
	 "",
	 iwconfig);

CMD_DECL(iwconfig)      { return iw_config(argc, (char **)argv); }
#endif

#if defined(CYGPKG_COMPONENT_WLAN_WIRELESS_TOOLS) && defined(CONFIG_WLAN_IWPRIV)
shell_cmd("iwpriv",
	 "wlan utility",
	 "",
	 iwpriv);

CMD_DECL(iwpriv)        { return iw_priv(argc, (char **)argv); }
#endif

#if defined(CYGPKG_COMPONENT_WLAN_HOSTAPD) && defined(CONFIG_WLAN_HOSTAPD)
shell_cmd("hostapd",
	 "wlan utility",
	 "",
	 hostapd);

CMD_DECL(hostapd) {

    if (argc > 1 && !strcmp(argv[0],"conf")) {
        pthread_mutex_lock(&write_nw_mutex);
        if (!strcmp(argv[2],"ath0")) {
            char *str = NULL;
            ecosap_config_struct.WLAN_config.ap_config.if_idx = strtol(argv[2], &str, 10);
            strcpy(ecosap_new_nw_config.WLAN_config.ap_config.ssid, argv[4]);
            ecosap_new_nw_config.WLAN_config.ap_config.channel = atoi(argv[5]);
            strcpy(ecosap_new_nw_config.WLAN_config.ap_config.security, argv[3]);

            if (argc > 6)
                strcpy(ecosap_new_nw_config.WLAN_config.ap_config.passphrase, argv[6]);
            write_nw_flash();
        }
        else {
            strcpy(ecosap_new_nw_config.WLAN_config.vlan_iptv_ap_config.ssid, argv[4]);
            ecosap_new_nw_config.WLAN_config.vlan_iptv_ap_config.channel = atoi(argv[5]);
            strcpy(ecosap_new_nw_config.WLAN_config.vlan_iptv_ap_config.security, argv[3]);

            if (argc > 6)
                strcpy(ecosap_new_nw_config.WLAN_config.vlan_iptv_ap_config.passphrase, argv[6]);
            write_nw_flash();
        }
        pthread_mutex_unlock(&write_nw_mutex);
    }
    return hostapd_cmds(argc, (char **)argv);
}
#endif

/* ToDo: Need to add condition for checking
 * if wpasupplicant package is enabled or not */
#if defined(CYGPKG_COMPONENT_WLAN_WPASUPPLICANT) && defined(CONFIG_WLAN_WPASUPPLICANT)
shell_cmd("wpasupplicant",
	 "wlan utility",
	 "",
	 wpasupplicant);

CMD_DECL(wpasupplicant)       { return wpa_supplicant_cmds(argc, (char **)argv); }
#endif

#if defined(CYGPKG_COMPONENT_WLAN_CONFIG_TOOLS) && defined(CONFIG_WLAN_WLANCONFIG) && \
	defined(CYGPKG_COMPONENT_WLAN_WIRELESS_TOOLS) && defined(CONFIG_WLAN_IWCONFIG)
shell_cmd("makevap",
	 "wlan utility to initialize VAP",
	 "makevap <Mode> <ESSID> <Channel_String> <beaconint>",
	 makevap_cmd);

CMD_DECL(makevap_cmd) { return makevap(argc, (char **)argv); }

int
makevap(int argc, char* argv[])
{
	int i, ret = 0;
	char apmode[32];
	char cmd_buf[MAX_CMD_SIZE];
	int channel = 0;

	if (argc < 3) {
		WLAN_INFO("Usage: makevap <Mode> <vap> <ESSID> <Channel_String> <beaconint>\n");
		return 0;
	}

	memset(apmode, 0, sizeof(apmode));

	/* Need to select the proper radio parameters based on the interface ID */

	/* WLAN driver should be initialized prior to this */

	/* Create the interface */
	WLAN_INFO("Creating interface in %s mode \n", argv[0]);
	if (strcmp((const char *)argv[0], "sta") == 0) {
		/* ToDo: Need to parse sub modes.
		    if [ "${IND_MODE}" = "sin" ]; then
		        APNAME=`wlanconfig ath create wlandev wifi$IFNUM wlanmode ${MODE}`
		    elif [ "${SUB_MODE}" = "proxy" ]; then
		        APNAME=`wlanconfig ath create wlandev wifi$IFNUM wlanmode ${MODE} wlanaddr 00:00:00:00:00:00 nosbeacon`
		    else
		        APNAME=`wlanconfig ath create wlandev wifi$IFNUM wlanmode ${MODE} nosbeacon`
		    fi
		 */

		snprintf(cmd_buf, sizeof(cmd_buf), 
				"%s create wlandev wifi%d wlanmode %s nosbeacon",
				argv[1], 0, argv[0]);
		ret = run_command(cmd_buf, wlan_config);
		WLAN_DEBUG("ret : %d\n", ret);

		strcpy(apmode, "mode managed");

		/* iwconfig ${APNAME} ${APMODE} */
		snprintf(cmd_buf, sizeof(cmd_buf), "%s %s", argv[1], apmode);
		ret = run_command(cmd_buf, iw_config);
		WLAN_DEBUG("ret : %d\n", ret);

	} else if(strcmp((const char *)argv[0], "ap") == 0) {
		/* ToDo:
			if [ "${IFNUM}" = "0" ]; then
				BCNBURST=${BCNBURST_ENABLE}
			else
				BCNBURST=${BCNBURST_ENABLE_2}
			fi
			if [ "${BCNBURST}" = "1" ]; then
				iwpriv wifi$IFNUM set_bcnburst 1
			fi
		*/
		snprintf(cmd_buf, sizeof(cmd_buf),
				"%s create wlandev wifi%d wlanmode %s",
				argv[1], 0, argv[0]);
		ret = run_command(cmd_buf, wlan_config);
		WLAN_DEBUG("ret : %d\n", ret);

		if (strcmp((const char *)argv[0], "adhoc") == 0) {
			strcpy(apmode, "mode adhoc");
		} else {
			strcpy(apmode, "mode master");
		}

		if ((argc >= 4) && argv[3] != NULL) {
			channel = atoi(argv[3]);
		}

		/* iwconfig ${APNAME} essid "${ESSID}" ${APMODE} ${FREQ} */
		snprintf(cmd_buf, sizeof(cmd_buf), "%s essid %s %s freq %d",
						argv[1], argv[2], apmode, channel);
		ret = run_command(cmd_buf, iw_config);
		WLAN_DEBUG("ret : %d\n", ret);

		/* iwpriv ${APNAME} bintval ${BEACONINT} */
		if (argv[4]) {
			snprintf(cmd_buf, sizeof(cmd_buf), "%s bintval %s", argv[1], argv[4]);
			ret = run_command(cmd_buf, iw_priv);
			WLAN_DEBUG("ret : %d\n", ret);
		}
	}

	WLAN_INFO("Added %s %s\n", argv[1], apmode);

	/* Do the simple configuration */
	/* iwpriv ${APNAME} mode ${CH_MODE} */
	snprintf(cmd_buf, sizeof(cmd_buf), "%s mode %s", argv[1], "11NGHT40");
	ret = run_command(cmd_buf, iw_priv);
	WLAN_DEBUG("ret : %d\n", ret);

	return SHELL_OK;
}

shell_cmd("activatevap",
	 "wlan utility to activate VAP that was created using makevap command",
	 "activatevap <vap> <BR> <Security> <SEC Args> <WSC>  <VAP_TIE>",
	 activatevap_cmd);

CMD_DECL(activatevap_cmd) { return activatevap(argc, (char **)argv); }

int
activatevap(int argc, char* argv[])
{
	int i, ret = 0;
	char *apname;
	char *bridge;
	char *secmode;
	char cmd_buf[MAX_CMD_SIZE];

	if(argc < 1) {
		WLAN_INFO("Usage: <vap> <BR> <Security> <SEC Args> <WSC>  <VAP_TIE>\n");
		WLAN_INFO("Examples: Open Access Point - activatevap ath0 br0 NONE\n");
		return 0;
	}

	/* Initialize default values */
	/* ToDo: Parse argv based on argc */
	apname = (char *)argv[0];
	if (apname == NULL) {
		return -1;
	}

	bridge = (char *)argv[1];
	secmode = (char *)argv[2];

	(void) secmode;

	/* Verify if vap is created or not */

	/* Bring up the interface */
	/* ifconfig ${APNAME} up */
	snprintf(cmd_buf, sizeof(cmd_buf), "%s up", apname);
	ret = run_command(cmd_buf, ifconfig_main);
	WLAN_DEBUG("ret : %d\n", ret);

	if (!ecosap_new_nw_config.vlan_config.vlan_flag) {
#ifdef BRIDGE_SUPPORT
		if (bridge && strcmp(bridge, "none") != 0) {
			/* Add VAP to the bridge interface */
			/* brconfig <bridge iface> add <VAP> */
			snprintf(cmd_buf, sizeof(cmd_buf), "%s add %s", bridge, apname);
			ret = run_command(cmd_buf, brconfig_main);
			WLAN_DEBUG("ret : %d\n", ret);
		}
#endif
	}

	/* Standard security. WSC is not enabled */

	return SHELL_OK;
}
#endif

#if 1
#if defined(CYGPKG_COMPONENT_WLAN_CONFIG_TOOLS) && defined(CONFIG_WLAN_WLANCONFIG) && \
	defined(CYGPKG_COMPONENT_WLAN_WIRELESS_TOOLS) && defined(CONFIG_WLAN_IWCONFIG)
shell_cmd("ap_start",
	 "wlan utility",
	 "",
	 ap_start);

CMD_DECL(ap_start) {
	memcpy(&ecosap_config_struct.WLAN_config, &ecosap_new_nw_config.WLAN_config,
					sizeof(ecosap_new_nw_config.WLAN_config));
	return apwlan_start();
}

shell_cmd("ap_stop",
	 "wlan utility",
	 "",
	 ap_stop);

CMD_DECL(ap_stop) {
	apwlan_stop();

	return SHELL_OK;
}
#endif
#endif

#if defined(CYGPKG_COMPONENT_WLAN_ACFG_TOOLS) && defined(CONFIG_WLAN_ACFG)
shell_cmd("acfg",
	 "wlan utility",
	 "",
	 acfg);

CMD_DECL(acfg)          { return wlan_acfg_cli(argc, argv); }
#endif

#ifdef UMAC_SUPPORT_ACFG
CMD_DECL(acfgtest)      { return wlan_acfg_test(argc, argv); }
#endif /* UMAC_SUPPORT_ACFG */
