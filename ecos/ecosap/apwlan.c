/*
 * Start and Stop of LAN API
 */

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

#include <network.h>
#include <stdio.h>
#include <netinet/in.h>
#include "debug.h"
#include "apwlan.h"
#include <cyg/dbg_print/dbg_print.h>

#include "ecosap_nw_config.h"
#include "ecosap_nw_http.h"

int run_command(char *cmd_buf, int (*func)(int argc, char *argv[]))
{
	char *argptr[16];
	char *ptr;
	int count = 0;

	if (cmd_buf == NULL || func == NULL) {
		return -1;
	}

	WLAN_DEBUG("cmd - %s\n", cmd_buf);

	for (ptr = strtok(cmd_buf, " "); ptr != NULL; ptr=strtok(NULL, " ")) {
		argptr[count++] = ptr;
	}

    return (*func)(count, argptr);
}

int apwlan_create_vaps(void)
{
    char cmd_buf[MAX_CMD_SIZE];
    struct WLAN_AP_Config *ap_config = &ecosap_config_struct.WLAN_config.ap_config;
    struct WLAN_STA_Config *sta_config = &ecosap_config_struct.WLAN_config.sta_config;
    int ret;
#ifdef CYGPKG_COMPONENT_IPTV
    struct WLAN_AP_VLAN_IPTV_Config *vlan_iptv_ap_config =
         &ecosap_config_struct.WLAN_config.vlan_iptv_ap_config;
#endif

    /* Create VAP Interface for AP (ath0) and apply other parameters */
    snprintf(cmd_buf, sizeof(cmd_buf), "ap ath0 %s %d %d",
				ap_config->ssid, ap_config->channel,
				ap_config->beacon_int);
    ret = run_command(cmd_buf, makevap);

    /* Put the ath0 interface down by default */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 down");
    ret = run_command(cmd_buf, ifconfig_main);

    if (!strcmp(ecosap_config_struct.WLAN_config.mode, "repeater")
		    && sta_config->wds == 1) {
        /* Create VAP Interface for STA (ath1) and apply other parameters */
        snprintf(cmd_buf, sizeof(cmd_buf), "sta ath1 %s", sta_config->ssid);
        ret = run_command(cmd_buf, makevap);

        /* Put the ath1 interface down by default */
        snprintf(cmd_buf, sizeof(cmd_buf), "ath1 down");
        ret = run_command(cmd_buf, ifconfig_main);
    }

#ifdef CYGPKG_COMPONENT_IPTV
	if (ecosap_new_nw_config.vlan_config.vlan_flag && ecosap_new_nw_config.vlan_config.is_tagged) {
		/* Create VAP Interface for AP (ath2) and apply other parameters */
		snprintf(cmd_buf, sizeof(cmd_buf), "ap ath2 %s %d %d",
				vlan_iptv_ap_config->ssid, vlan_iptv_ap_config->channel,
				vlan_iptv_ap_config->beacon_int);
		ret = run_command(cmd_buf, makevap);

		/* Put the ath2 interface down by default */
		snprintf(cmd_buf, sizeof(cmd_buf), "ath2 down");
		ret = run_command(cmd_buf, ifconfig_main);
	}
#endif

    return ret;
}

int apwlan_ap_start(void)
{
    char cmd_buf[MAX_CMD_SIZE];
    struct WLAN_AP_Config *ap_config = &ecosap_config_struct.WLAN_config.ap_config;
    int ret;

#ifdef CYGPKG_COMPONENT_IPTV
    struct WLAN_AP_VLAN_IPTV_Config *vlan_iptv_ap_config =
         &ecosap_config_struct.WLAN_config.vlan_iptv_ap_config;
#endif

    /* Set the country code */
    snprintf(cmd_buf, sizeof(cmd_buf), "wifi0 setCountryID 841");
    ret = run_command(cmd_buf, iw_priv);

    /* Bring up the interface */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 %s",
#ifdef BRIDGE_SUPPORT
					Server_Int  /* Bridge Inteface configured in net stack */
#else
					"none"
#endif
					);
    run_command(cmd_buf, activatevap);

    /* Set 80211 mode */
    /* iwpriv ${APNAME} mode ${CH_MODE} */
    if (strcmp(ap_config->wlan_mode, "11NG") == 0) {
        if (ap_config->bw == 40) {
	    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 mode 11NGHT40");
	} else {
	    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 mode 11NGHT20");
	}
        ret = run_command(cmd_buf, iw_priv);

        /* Set Bandwidth */

        /* Set chwidth */
        snprintf(cmd_buf, sizeof(cmd_buf), "ath0 chwidth %d",
		    		(ap_config->bw == 20) ? 1 : 2);
        ret = run_command(cmd_buf, iw_priv);
    } else {
        snprintf(cmd_buf, sizeof(cmd_buf), "ath0 mode %s",
					ap_config->wlan_mode);
        ret = run_command(cmd_buf, iw_priv);
    }

    /* Enable WDS if it is configured */
    if (ap_config->wds) {
        snprintf(cmd_buf, sizeof(cmd_buf), "ath%d wds %d", 0, ap_config->wds);
	ret = run_command(cmd_buf, iw_priv);
    }

    /* Set interface down to avoid beacon stop */
	snprintf(cmd_buf, sizeof(cmd_buf), "ath0 down");
	ret = run_command(cmd_buf, ifconfig_main);
    /* Start hostapd */
    /* hostapd start -i <ifname> <open|wpa-ccmp|wpa-tkip|wpa2|mixed>
                            <ssid> <channel> [passphrase] */
    if (strcmp(ap_config->security, "open") == 0) {
         snprintf(cmd_buf, sizeof(cmd_buf), "conf -i ath0 open %s %d",
                ap_config->ssid, ap_config->channel);
    } else {
        snprintf(cmd_buf, sizeof(cmd_buf), "conf -i ath0 %s %s %d %s",
                ap_config->security, ap_config->ssid,
                ap_config->channel, ap_config->passphrase);
    }
    ret = run_command(cmd_buf, hostapd_cmds);

#ifdef CYGPKG_COMPONENT_IPTV
    if (ecosap_new_nw_config.vlan_config.vlan_flag && ecosap_new_nw_config.vlan_config.is_tagged) {
        if (strcmp(vlan_iptv_ap_config->security, "open") == 0) {
            snprintf(cmd_buf, sizeof(cmd_buf), "conf -i ath2 open %s %d",
                    vlan_iptv_ap_config->ssid, vlan_iptv_ap_config->channel);
        } else {
            snprintf(cmd_buf, sizeof(cmd_buf), "conf -i ath2 %s %s %d %s",
                    vlan_iptv_ap_config->security, vlan_iptv_ap_config->ssid,
                    vlan_iptv_ap_config->channel, vlan_iptv_ap_config->passphrase);
        }
        ret = run_command(cmd_buf, hostapd_cmds);
    }
#endif

    snprintf(cmd_buf, sizeof(cmd_buf), "start");
    ret = run_command(cmd_buf, hostapd_cmds);

    return ret;
}

int apwlan_repeater_start(void)
{
    char cmd_buf[MAX_CMD_SIZE];
    struct WLAN_AP_Config *ap_config = &ecosap_config_struct.WLAN_config.ap_config;
    struct WLAN_STA_Config *sta_config = &ecosap_config_struct.WLAN_config.sta_config;
    int ret;

#ifdef CYGPKG_COMPONENT_IPTV
    struct WLAN_AP_VLAN_IPTV_Config *vlan_iptv_ap_config =
         &ecosap_config_struct.WLAN_config.vlan_iptv_ap_config;
#endif

    /* If WDS is not enabled, can not start repeater */
    if (sta_config->wds != 1) {
        return -1;
    }

    /* Set the country code */
    snprintf(cmd_buf, sizeof(cmd_buf), "wifi0 setCountryID 841");
    ret = run_command(cmd_buf, iw_priv);

    /* Bring up the STA interface */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath1 %s",
#ifdef BRIDGE_SUPPORT
					Server_Int  /* Bridge Inteface configured in net stack */
#else
					"none"
#endif
					);
    run_command(cmd_buf, activatevap);

    /* Fix this block of code once we are sure about VLAN in repeater mode */
#ifdef CYGPKG_COMPONENT_IPTV /* ath1 interface is not added to bride in activatevap command */
#ifdef BRIDGE_SUPPORT
    /* Add VAP to the bridge interface */
    /* brconfig <bridge iface> add <VAP> */
    snprintf(cmd_buf, sizeof(cmd_buf), "br0 add ath1");
    ret = run_command(cmd_buf, brconfig_main);
#endif
#endif

    /* Enable WDS - STA */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath1 wds 1");
    ret = run_command(cmd_buf, iw_priv);

    /* iwpriv ath1 vap_ind 1 */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath1 vap_ind 1");
    ret = run_command(cmd_buf, iw_priv);

    /* Start the wpa_supplicant */
    /* wpasupplicant start -i ath1 open|wpa|wpa2 wds_ssid */
    if (strcmp(sta_config->security, "open") == 0) {
        snprintf(cmd_buf, sizeof(cmd_buf), "start -i ath1 open %s",
						sta_config->ssid);
    } else {
        snprintf(cmd_buf, sizeof(cmd_buf), "start -i ath1 %s %s %s",
			sta_config->security, sta_config->ssid,
			sta_config->passphrase);
    }
    ret = run_command(cmd_buf, wpa_supplicant_cmds);

    /* Bring up the AP interface */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 %s",
#ifdef BRIDGE_SUPPORT
					Server_Int  /* Bridge Inteface configured in net stack */
#else
					"none"
#endif
					);
    run_command(cmd_buf, activatevap);

    /* Set 80211 mode */
    /* iwpriv ${APNAME} mode ${CH_MODE} */
    if (strcmp(ap_config->wlan_mode, "11NG") == 0) {
        if (ap_config->bw == 40) {
	    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 mode 11NGHT40");
	} else {
	    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 mode 11NGHT20");
	}
        ret = run_command(cmd_buf, iw_priv);

        /* Set Bandwidth */

        /* Set chwidth */
        snprintf(cmd_buf, sizeof(cmd_buf), "ath0 chwidth %d",
		    		(ap_config->bw == 20) ? 1 : 2);
        ret = run_command(cmd_buf, iw_priv);
    } else {
        snprintf(cmd_buf, sizeof(cmd_buf), "ath0 mode %s",
					ap_config->wlan_mode);
        ret = run_command(cmd_buf, iw_priv);
    }

    /* Set chwidth */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 chwidth %d",
		    		(ap_config->bw == 20) ? 1 : 2);
    ret = run_command(cmd_buf, iw_priv);

    /* Enable WDS - AP */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 wds 1");
    ret = run_command(cmd_buf, iw_priv);

    /* iwpriv ath0 vap_ind 1 */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 vap_ind 1");
    ret = run_command(cmd_buf, iw_priv);

    /* Start hostapd */
    if (strcmp(ap_config->security, "open") == 0) {
        snprintf(cmd_buf, sizeof(cmd_buf), "conf -i ath0 open %s %d",
				ap_config->ssid, ap_config->channel);
    } else {
        snprintf(cmd_buf, sizeof(cmd_buf), "conf -i ath0 %s %s %d %s",
				ap_config->security, ap_config->ssid,
				ap_config->channel, ap_config->passphrase);
    }
    ret = run_command(cmd_buf, hostapd_cmds);

#ifdef CYGPKG_COMPONENT_IPTV
    if (ecosap_new_nw_config.vlan_config.vlan_flag && ecosap_new_nw_config.vlan_config.is_tagged) {
        if (strcmp(vlan_iptv_ap_config->security, "open") == 0) {
            snprintf(cmd_buf, sizeof(cmd_buf), "conf -i ath2 open %s %d",
                    vlan_iptv_ap_config->ssid, vlan_iptv_ap_config->channel);
        } else {
            snprintf(cmd_buf, sizeof(cmd_buf), "conf -i ath2 %s %s %d %s",
                    vlan_iptv_ap_config->security, vlan_iptv_ap_config->ssid,
                    vlan_iptv_ap_config->channel, vlan_iptv_ap_config->passphrase);
        }
        ret = run_command(cmd_buf, hostapd_cmds);
    }
#endif

    snprintf(cmd_buf, sizeof(cmd_buf), "start");
    ret = run_command(cmd_buf, hostapd_cmds);

    return ret;
}

int apwlan_start(void)
{
    int ret = 0;

    WLAN_DEBUG("Device mode : %s\n", ecosap_config_struct.WLAN_config.mode);
    WLAN_DEBUG("ssid : %s\n", ecosap_config_struct.WLAN_config.ap_config.ssid);
    WLAN_DEBUG("channel : %d\n", ecosap_config_struct.WLAN_config.ap_config.channel);
    WLAN_DEBUG("wifi mode : %s\n", ecosap_config_struct.WLAN_config.ap_config.wlan_mode);
    WLAN_DEBUG("beacon Int : %d\n", ecosap_config_struct.WLAN_config.ap_config.beacon_int);
    WLAN_DEBUG("AP wds : %d\n", ecosap_config_struct.WLAN_config.ap_config.wds);
    WLAN_DEBUG("AP security : %s\n", ecosap_config_struct.WLAN_config.ap_config.security);
    WLAN_DEBUG("STA wds : %d\n", ecosap_config_struct.WLAN_config.sta_config.wds);

    apwlan_create_vaps();

    if (!strcmp(ecosap_config_struct.WLAN_config.mode, "ap") ||
		!strcmp(ecosap_config_struct.WLAN_config.mode, "rootap")) {
        ret = apwlan_ap_start();
    } else if (!strcmp(ecosap_config_struct.WLAN_config.mode, "repeater")) {
        ret = apwlan_repeater_start();
    }

    return ret;
}

int apwlan_init(void)
{
    int ret = 0;

    ENTER();

    if (ecosap_config_struct.WLAN_config.magic_num != CONFIG_WLAN_MAGIC_NUM) {
        WLAN_ERROR("WLAN default configuration is not set!!!!!\n");
        WLAN_ERROR("Set the default configuration and restart the device\n");
	return -1;
    }

    apwlan_start();

    return ret;
}

void apwlan_stop(void) 
{
    char cmd_buf[MAX_CMD_SIZE];
    int ret = 0;

    ENTER();

    /* hostapd stop */
    snprintf(cmd_buf, sizeof(cmd_buf), "stop");
    ret = run_command(cmd_buf, hostapd_cmds);

    /* Bring down the interfaces and remove from bridge */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 down");
    ret = run_command(cmd_buf, ifconfig_main);

#ifndef CYGPKG_COMPONENT_IPTV
#ifdef BRIDGE_SUPPORT
    snprintf(cmd_buf, sizeof(cmd_buf), "%s delete ath0", Server_Int);
    ret = run_command(cmd_buf, brconfig_main);
#endif
#endif

    /* Destroy ath0 */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath0 destroy");
    ret = run_command(cmd_buf, wlan_config);

    /* If old configuration is repeater, then cleanup ath1 */
    if (!strcmp(ecosap_config_struct.WLAN_config.mode, "repeater")) {
        /* Stop wpa_supplicant */
        snprintf(cmd_buf, sizeof(cmd_buf), "stop");
        ret = run_command(cmd_buf, wpa_supplicant_cmds);

        snprintf(cmd_buf, sizeof(cmd_buf), "ath1 down");
        ret = run_command(cmd_buf, ifconfig_main);

#ifdef BRIDGE_SUPPORT
        snprintf(cmd_buf, sizeof(cmd_buf), "%s delete ath1", Server_Int);
        ret = run_command(cmd_buf, brconfig_main);
#endif

        /* Destroy ath1 */
        snprintf(cmd_buf, sizeof(cmd_buf), "ath1 destroy");
        ret = run_command(cmd_buf, wlan_config);
    }

#if 0
#ifdef CYGPKG_COMPONENT_IPTV
    /* Put the ath2 interface down by default */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath2 down");
    ret = run_command(cmd_buf, ifconfig_main);

    /* Destroy ath2 */
    snprintf(cmd_buf, sizeof(cmd_buf), "ath2 destroy");
    ret = run_command(cmd_buf, wlan_config);
#endif
#endif

    return;
}

