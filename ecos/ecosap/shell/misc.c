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

#include <stdio.h>
#include <time.h>
#include <commands.h>
#include <cyg/kernel/kapi.h>

#if (defined CYG_COMPONENT_QCA953X_GPIO) && (defined CONFIG_BSP_GPIO)
#include <cyg/hal/qca953x_gpio.h>
#endif
 
#define LED_ON 0
#define LED_OFF 1

#define LINK_1_LED   16
#define LINK_2_LED   15 
#define LINK_3_LED   14
#define LINK_4_LED   11
#define WLAN_LED     12
#define SYS_LED      13
#define INTERNET_LED 4

/*
 * This function will generate exception to test crash dump
 */

#ifdef CONFIG_BSP_EXCEPTION

shell_cmd("exception",
         "generate exception",
         "",
         cyg_generate_exception);

CMD_DECL(cyg_generate_exception)
{
     *(int *)(0x60000000) = 12;
     *(int *)(0x85000001) = 12;

     return 0;
}

#endif /* EXCEPTION */


#if (defined CYG_COMPONENT_QCA953X_GPIO) && (defined CONFIG_BSP_GPIO)

shell_cmd("gpio_test",
	 "Simple utility to test Gpio",
	 "",
	 gpio_test);

CMD_DECL(gpio_test)
{
	int l = 0;

	int arr[] = {INTERNET_LED, LINK_2_LED, LINK_3_LED, LINK_4_LED, WLAN_LED, SYS_LED};
	
	printf ("Turning OFF LEDs\n");
	
	for(l=0; l<6; l++) { 

		qca_gpio_write(arr[l], LED_OFF);

		cyg_thread_delay(50);
	}
	
	cyg_thread_delay(100);

	printf ("Turning ON LEDs\n");
	
	for(l=0; l<6; l++) { 

		qca_gpio_write(arr[l], LED_ON);

		cyg_thread_delay(50);
	}
			
	return 0;
}
#endif /* GPIO */
