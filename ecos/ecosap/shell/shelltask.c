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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>

#include <shell_thread.h>
#include <shell_err.h>
#include <commands.h>
#include <cyg/io/serial.h>

#define SHELLTASK_PRIORITY	28			/* This runs at a low priority */
#define SHELL_NAME		"eCosShell>"		/* The default command line */
#define ERRORCOMMAND		255
#define COM_BUF_LEN		256
#define SHELL_HIST_SIZE		10

CYG_HAL_TABLE_BEGIN(__shell_CMD_TAB__, shell_commands);
CYG_HAL_TABLE_END(__shell_CMD_TAB_END__, shell_commands);

uint8_t
qca_serial_getc(serial_channel * priv);

unsigned char count_argc(char *buf, shell_st_call *func);
void shelltask(cyg_addrword_t data);

unsigned char st_stack[SHELL_STACK_MAX];
unsigned char *argv[10];
unsigned char argc;

typedef struct _shell_hist_t {
    char command_buf[COM_BUF_LEN + 1];
    unsigned int pos;
    struct _shell_hist_t *prev, *next;
} shell_hist_t;

shell_hist_t *
add_shell_history_node(shell_hist_t *history, unsigned int pos)
{
    shell_hist_t *s;

    if(!(s = (shell_hist_t *)malloc(sizeof(shell_hist_t))))
	return(NULL);

    memset(s, 0, sizeof(shell_hist_t));

    if(history) 
	history->next = s;

    s->prev = history;
    s->pos = pos;

    return s;
}

shell_hist_t *
build_hist_list(unsigned int size)
{
    int i = size;
    shell_hist_t *sh = NULL, *start = NULL;

    while(i) {
	if(i == size) {
	    if(!(start = sh = add_shell_history_node(NULL, i))) {
		printf("Aiee -- add_shell_history_node() returned NULL\n");
		return(NULL);
	    }
	}
	else {
	    if(!(sh = add_shell_history_node(sh, i))) {
		printf("Aiee -- add_shell_history_node() returned NULL\n");
		return(NULL);
	    }
	}
	i--;
    }

    sh->next = start;
    start->prev = sh;

    return start;
}

void
create_shell_thread(void)
{
    if (shell_create_thread(NULL,
			  SHELLTASK_PRIORITY,
			  shelltask,
			  0,
			  "Shell Shell",
			  &st_stack,
			  sizeof(st_stack),
			  NULL) != SHELL_OK) {
	SHELL_ERROR("Failed to create Shell thread\n");
	HAL_PLATFORM_RESET();
    }

    SHELL_DEBUG_PRINT("Created Shell thread\n");
}


#undef putchar
#undef printf
#define putchar diag_write_char
#define printf diag_printf

unsigned int shell_cnt = 1;
void
shelltask(cyg_addrword_t data)
{
    shell_st_call func;
    shell_hist_t *sh;

    cyg_io_handle_t handle;
    cyg_uint32 len;
    Cyg_ErrNo err;

    unsigned char i = 0, num;
    
    char command_buf[COM_BUF_LEN + 1];	/* store '\0' */
    char ch;

    len = sizeof(ch);

    if(!(sh = build_hist_list(SHELL_HIST_SIZE))) {
	printf("build_hist_list() failed\nResetting\n");
	HAL_PLATFORM_RESET();
    }

#if 0
    err = cyg_io_lookup("/dev/ser0", &handle);

    if(err != ENOERR) {
	printf("Unable to open /dev/ser0\nHalting\n");
	HAL_PLATFORM_RESET();
    }

    /* Unbuffer stdout */
    setvbuf(stdout, (char *)NULL, _IONBF, 0);
#endif
    command_buf[0] = '\0';

    printf("\n"SHELL_NAME" ");

    while(1) {
	do{ 
//	    cyg_io_read(handle, &ch, &len);
           ch = qca_serial_getc(NULL);
	} while(!isprint(ch) && 
		(ch != '\b') &&		/* Backspace */ 
		(ch != 0x7F) &&		/* Backspace */
		(ch != 0x0A) &&		/* LF */
		(ch != 0x0D) &&		/* CR */
		(ch != 0x01) &&		/* Ctrl-A */
		(ch != 0x02) &&		/* Ctrl-B */
		(ch != 0x15) &&		/* Ctrl-U */
		(ch != 0x06) &&		/* Ctrl-F */
		(ch != 0x10) &&		/* Ctrl-P */
		(ch != 0x0E));		/* Ctrl-N */

	shell_cnt++;
	switch(ch) {

	case 0x01:		/* Ctrl-A */
	    if(i) {
		int x = i;
		while(x) {
		    putchar('\b');
		    x--;
		}
	    }
	    break;

	case 0x10:		/* Ctrl-P - previous command */
	    /* If there's any input on the line already, erase it */
	    if(i) {
		int x = i;
		while(x) {
		    putchar('\b');
		    putchar(' ');
		    putchar('\b');
		    x--;
		}		
	    }

	    sh = sh->prev;
	    strncpy(command_buf, sh->command_buf, sizeof(command_buf) - 1);
	    i = strlen(command_buf);
	    printf("%s", command_buf);
	    break;

	case 0x0E:		/* Ctrl-N - next command */
	    /* If there's any input on the line already, erase it */
	    if(i) {
		int x = i;
		while(x) {
		    putchar('\b');
		    putchar(' ');
		    putchar('\b');
		    x--;
		}
	    }

	    sh = sh->next;
	    strncpy(command_buf, sh->command_buf, sizeof(command_buf) - 1);
	    i = strlen(command_buf);
	    printf("%s", command_buf);
	    break;
	    
	case 0x02:		/* Ctrl-B */	
	    if(i) {
		i--;
		putchar('\b');
	    }
	    break;
		
	case 0x0A:				/* LF */
	case 0x0D:				/* CR */
	    if(!i) {				/* commandbuf is NULL, begin a new line */
		printf("\n"SHELL_NAME" ");
	    }
	    else {
		/* Get rid of the end space */
		if(command_buf[i - 1]==' ') 
		    i--;

		command_buf[i] = '\0';
		
		/* Add to the shell history */
		strncpy(sh->command_buf, command_buf, sizeof(sh->command_buf) - 1);
		sh = sh->next;
		
		/* Count argv entries in the command_buf */
		num = count_argc(command_buf, &func);
		
		if(num == ERRORCOMMAND) {
		    /* Error or none exist command */
		    i = 0;
		    printf("Unknown command '%s'\n", (const char *)command_buf);
		    command_buf[i] = '\0';
		    printf(SHELL_NAME" ");
		}
		else{
		    /* Call corresponding function */
		    func(argc, argv);
		    i = 0;
		    command_buf[i] = '\0';
		    printf(SHELL_NAME" ");
		}
	    }
	    break;
	    
	case 0x15:			/* Ctrl-U */
	    if(i) {
		while(i) {
		    putchar('\b');
		    putchar(' ');
		    putchar('\b');
		    i--;
		}
	    }
	    break;

	case 0x7F:
	case '\b':		/* Backspace */
	    if(i) {
		i--;		/* Pointer back once */
		putchar('\b');	/* Cursor back once */
		putchar(' ');	/* Erase last char in screen */
		putchar('\b');	/* Cursor back again */
	    }
	    break;
	
	case ' ':
	    /* Don't allow continuous or begin space(' ') */
	    if((!i) || 
	       (i > COM_BUF_LEN) || 
	       (command_buf[i - 1] == ' ')) {
		/* do nothing */
	    }
	    else {
		command_buf[i] = ch;
		i++;
		putchar(ch);	/* display and store ch */
	    }
	    break;

	default:			/* Normal key */
	    if(i < COM_BUF_LEN) {	/* The buf is less than MAX length */
		command_buf[i] = ch;
		i++;
		putchar(ch);  /* Display and store ch */
	    }
	    break;
	}
    }
    diag_printf("########## shell exit, should never happen ################\n");
}



unsigned char 
count_argc(char *buf, shell_st_call *func)
{
    ncommand_t *shell_cmd = __shell_CMD_TAB__;
    unsigned char pointer, num;
    char name[40];

    argc = 0;			/* argc is global  */
    num = 0;
    pointer = 0;
    printf("\n\r");


    while((buf[pointer] != ' ') && 
	  (buf[pointer] != '\0') && 
	  (pointer < 20)) {
	name[pointer] = buf[pointer];
	pointer++;
    }
    name[pointer] = '\0';


    /* 
     * Now got the command name, and pointer 
     * is to the first space in the buf
     */

    /* Check the dynamic function tables as well */
    while(shell_cmd != &__shell_CMD_TAB_END__) {
	if(!strcmp(name, shell_cmd->name)) {
	    *func = shell_cmd->cfunc;
	    break;
	}
	shell_cmd++;
    }


    if(shell_cmd == &__shell_CMD_TAB_END__)
	return ERRORCOMMAND;

    while(buf[pointer] != '\0') {
	if(buf[pointer] == ' ') {

	    if(argc > 0) {
		/* End of last argv */
		buf[pointer] = '\0';
	    }

	    pointer++;

	    /* Add a parameter for every space */
	    argv[argc] = (unsigned char *)&buf[pointer];
	    argc++;
	}
	else pointer++;
    }

    return num;
}

#undef putchar
#undef printf
