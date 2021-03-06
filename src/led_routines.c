/*
 * Copyright (C) 2008-2009 Bjoern Biesenbach <bjoern@bjoern-b.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*!
* \file	led_routines.c
* \brief	functions for the ledmatrix display
* \author	Bjoern Biesenbach <bjoern at bjoern-b dot de>
*/

#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "led_routines.h"
#include "fonts/arial_bold_14.h"
//#include "fonts/arial_8.h"
//#include "fonts/Comic_8.h"
//#include "fonts/Comic_9.h"
//#include "fonts/Comic_10.h"
#include "configfile.h"

static uint16_t charGetStart(char c);
static int initNetwork(void);
static void ledDisplayMain(struct _ledLine *ledLineToDraw, int shift_speed);

//static uint8_t *font = &Arial_Bold_14;
static uint8_t *font = Arial_Bold_14;

/* Diese Arrays werden nur zur Uebertragung ans Modul genutzt */
static uint16_t RED[4][16];
static uint16_t GREEN[4][16];

static struct _ledLine ledLineStack[LED_MAX_STACK];

int led_line_stack_time[LED_MAX_STACK];
int led_line_stack_shift[LED_MAX_STACK];

static int client_sock;
static int running;

volatile int led_stack_size = 0;


int ledIsRunning(void)
{
	return running;
}

void updateDisplay(struct _ledLine ledLine)
{
	int bytes_send;
	int i,p,m;

	memset(&RED,0,sizeof(RED));
	memset(&GREEN,0,sizeof(GREEN));

	for(m=0;m<4;m++) // for every module
	{
		for(i=0;i<16;i++) // for every row
		{
			for(p=0;p<16;p++) // for every single led in row
			{
#ifdef LED_HEADFIRST
				/* was there a shift yet? if no, print the unshifted arrays */
				if(ledLine.shift_position)
				{
					RED[3-m][15-i] |= ((ledLine.column_red_output[15-p+m*16] & (1<<i))>>(i)<<p);
					GREEN[3-m][15-i] |= ((ledLine.column_green_output[15-p+m*16] & (1<<i))>>(i)<<p);
				}
				else
				{
					RED[3-m][15-i] |= ((ledLine.column_red[15-p+m*16] & (1<<i))>>(i)<<p);
					GREEN[3-m][15-i] |= ((ledLine.column_green[15-p+m*16] & (1<<i))>>(i)<<p);
				}
#else
				/* was there a shift yet? if no, print the unshifted arrays */
				if(ledLine.shift_position)
				{
					RED[m][i] |= ((ledLine.column_red_output[p+m*16] & (1<<i))>>(i)<<p);
					GREEN[m][i] |= ((ledLine.column_green_output[p+m*16] & (1<<i))>>(i)<<p);
				}
				else
				{
					RED[m][i] |= ((ledLine.column_red[p+m*16] & (1<<i))>>(i)<<p);
					GREEN[m][i] |= ((ledLine.column_green[p+m*16] & (1<<i))>>(i)<<p);
				}
#endif

			}
		}
	}
	bytes_send = send(client_sock, &RED, sizeof(RED),0);
	bytes_send = send(client_sock, &GREEN, sizeof(GREEN),0);
	bytes_send = send(client_sock, &ledLine.beep, sizeof(ledLine.beep),0);
}

/* Achtung, funktioniert derzeit nur fuer font ! */
static uint16_t charGetStart(char c)
{
	uint8_t first_char = font[4];
	uint8_t char_height = font[3];

	uint8_t factor = 1;

	if(char_height > 8)
		factor = 2;

	uint8_t counter;
	uint16_t position = 0;

	for(counter=0;counter<(c-first_char);counter++)
	{
		position += font[6+counter]*factor;
	}

	return position;
}

int charWidth(char c)
{
	if(c == '\a' || c == '\b' || c == '\n' || c == '\r')
		return 0;
	else if(c == 32)
		return 4;
	else
		return font[6+c-font[4]] + 1;
}


int stringWidth(char *string)
{
	int width = 0;
	if(!string)
		string = "null";
	while(*string)
	{
		width += charWidth(*string++);
	}
	return width - 1;
}


int putChar(char c, uint8_t color, struct _ledLine *ledLine)
{

	uint8_t first_char = font[4];
	uint8_t char_count = font[5];
	uint8_t char_width;
	uint8_t char_height = font[3];

	uint16_t start;

	uint8_t i;

	if(c == '\n')
	{
		ledLine->x = 0;
		ledLine->y = 8;
		return 1;
	}

	/* if char is not in our font just leave */
	if(c < first_char || c > (first_char + char_count))
		return 1;
	
	/* Leerzeichen abfangen */
	if(c == 32)
		char_width = 4;
	else
	{
		char_width = font[6+c-first_char];
		start = charGetStart(c);
	}
	
	if((ledLine->x + char_width) >= LINE_LENGTH-50)
		return 0;

	if(c == ' ')
	{
		ledLine->x += 4;
		return 1;
	}

	for(i=0;i<char_width;i++)
	{
		if(color == COLOR_RED)
		{
			ledLine->column_red[i+ledLine->x] |= font[6+char_count+start+i]<<ledLine->y;
		}
		else if(color == COLOR_GREEN)
		{
			ledLine->column_green[i+ledLine->x] |= font[6+char_count+start+i]<<ledLine->y;
		}
		else if(color == COLOR_AMBER)
		{
			ledLine->column_red[i+ledLine->x] |= font[6+char_count+start+i]<<ledLine->y;
			ledLine->column_green[i+ledLine->x] |= font[6+char_count+start+i]<<ledLine->y;
		}
	}
	/* unteren Teil der Zeichen schreiben (noch nicht dynamisch fuer verschiedene Schriftgroessen) */
	for(i=0;i<char_width;i++)
	{
		if(color == COLOR_RED)
		{
			/* Man erklaere mir was ich hier geschrieben. Aber funktionieren tuts! :-) */
			ledLine->column_red[i+ledLine->x] |= font[6+char_count+start+i+char_width]<<(char_height - 8)<<ledLine->y;
		}
		else if(color == COLOR_GREEN)
		{
			ledLine->column_green[i+ledLine->x] |= font[6+char_count+start+i+char_width]<<(char_height - 8)<<ledLine->y;
		}
		else if(color == COLOR_AMBER)
		{
			ledLine->column_red[i+ledLine->x] |= font[6+char_count+start+i+char_width]<<(char_height - 8)<<ledLine->y;
			ledLine->column_green[i+ledLine->x] |= font[6+char_count+start+i+char_width]<<(char_height -8)<<ledLine->y;
		}
	}

	ledLine->x += char_width + 1;

	return 1;
}

void putString(char *string, struct _ledLine *ledLine)
{
	static int color = COLOR_RED;

	if(!string)
		string = "null";
	while(*string)
	{
		if(*string == '\b')
			color = COLOR_GREEN;
		else if(*string == '\r')
			color = COLOR_RED;
		else if(*string == '\a')
			color = COLOR_AMBER;
		else if(!putChar(*string,color,ledLine))
			return;
		string++;
	}
}

void clearScreen(struct _ledLine *ledLine)
{
	memset(ledLine->column_red,0,sizeof(uint16_t)*LINE_LENGTH);
	memset(ledLine->column_green,0,sizeof(uint16_t)*LINE_LENGTH);
	
	memset(ledLine->column_red_output,0,sizeof(uint16_t)*LINE_LENGTH);
	memset(ledLine->column_green_output,0,sizeof(uint16_t)*LINE_LENGTH);

	ledLine->x = 0;
	ledLine->y = 1;
}


int shiftLeft(struct _ledLine *ledLine)
{
	int counter;

	int scroll_length;
	
	if(ledLine->x + 11 > LINE_LENGTH)
		scroll_length = LINE_LENGTH;
	else
		scroll_length = ledLine->x + 11;

	for(counter=0;counter< scroll_length - 1  ;counter++)
	{
		if(ledLine->shift_position + counter > scroll_length - 1)
		{
			ledLine->column_red_output[counter] = ledLine->column_red[counter + ledLine->shift_position - (scroll_length)];
			ledLine->column_green_output[counter] = ledLine->column_green[counter + ledLine->shift_position - (scroll_length)];
		}
		else
		{
			ledLine->column_red_output[counter] = ledLine->column_red[ledLine->shift_position+counter];
			ledLine->column_green_output[counter] = ledLine->column_green[ledLine->shift_position+counter];
		}
	}

	ledLine->shift_position++;
	if(!(ledLine->shift_position % 20))
		ledLine->beep = 0;
	if(ledLine->shift_position > ledLine->x + 11)
	{
		ledLine->shift_position = 1;
		return 0;
	}
	else
		return 1;
}

static int initNetwork(void)
{
	struct sockaddr_in server;

	client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(client_sock < 0)
		printf("Keine Verbindung zum LED-Modul\n");
	server.sin_family = AF_INET;
	server.sin_port = htons(config.led_matrix_port);
	inet_aton(config.led_matrix_ip, &server.sin_addr);
	

	if(connect(client_sock, (struct sockaddr*)&server, sizeof(server)) != 0)
	{
		printf("Konnte nicht zum LED-Modul verbinden\n");
		return -1;
	}	

	return 0;
}

void stopLedMatrixThread()
{
	printf("LedMatrixThread gestoppt\n");
	running = 0;
}

int allocateLedLine(struct _ledLine *ledLine, int line_length)
{
	ledLine->column_red = calloc(sizeof(uint16_t), line_length);
	if(!ledLine->column_red)
		return 0;
	ledLine->column_green = calloc(sizeof(uint16_t), line_length);
	if(!ledLine->column_green)
		return 0;
	
	ledLine->column_red_output = calloc(sizeof(uint16_t), line_length);
	if(!ledLine->column_red_output)
		return 0;
	ledLine->column_green_output = calloc(sizeof(uint16_t), line_length);
	if(!ledLine->column_green_output)
		return 0;
	

	clearScreen(ledLine);

	ledLine->x = 0;
	ledLine->y = 1;
	ledLine->shift_position = 0;
	return 1;
}

void freeLedLine(struct _ledLine *ledLine)
{
	if(!ledLine->column_red)
		printf("ledLine.column_red was not allocated\n");
	if(!ledLine->column_green)
		printf("ledLine.column_green was not allocated\n");
	if(!ledLine->column_red_output)
		printf("ledLine.column_red_output was not allocated\n");
	if(!ledLine->column_green_output)
		printf("ledLine.column_green_output was not allocated\n");
	free(ledLine->column_red);
	free(ledLine->column_green);
	
	free(ledLine->column_red_output);
	free(ledLine->column_green_output);
}

void ledPushToStack(char *string, int shift, int lifetime)
{
	if(!running)
		return;
	if(led_stack_size + 1 < LED_MAX_STACK)
	{
		int x;
		if(!allocateLedLine(&ledLineStack[led_stack_size], LINE_LENGTH))
		{
			printf("Could not allocate memory!!\n");
			return;
		}
		
		printf("String pushed to stack: %s\n",string);
		putString(string,&ledLineStack[led_stack_size]);
		
		x = ledLineStack[led_stack_size].x;

		lifetime *= (x+11);
		if(x > 64)
			lifetime -= 64;
		else
			shift = 0;
		led_line_stack_time[led_stack_size] = lifetime;
		led_line_stack_shift[led_stack_size] = shift;
		ledLineStack[led_stack_size].beep = 1;
		led_stack_size++;
	}
	else
	{
		printf("LED-Stack is full!!\n");
	}
}

static void ledPopFromStack(void)
{
	printf("String popped from stack\n");
	freeLedLine(&ledLineStack[led_stack_size-1]);
	led_stack_size--;

}

void ledMatrixThread(void)
{
	if(running) // something went terribly wrong
		return;
	printf("LedMatrixThread gestartet\n");

	struct _ledLine ledLineTime;
	struct _ledLine ledLineGeraffel;
	struct _ledLine *ledLineToDraw;
	int shift_speed = 0;
	time_t rawtime;
	struct tm *ptm;


	char time_string[200];

	running = 1;
	
	allocateLedLine(&ledLineTime, LINE_LENGTH);
	allocateLedLine(&ledLineGeraffel, LINE_LENGTH);

	if(initNetwork() < 0)
	{
		printf( "Stopping led_matrix thread\n");
		running = 0;
	}
	
	sprintf(time_string,"\bG\re\ar\ba\rf\af\be\rl \aV\bi\rl\al\ba\rg\ae          ");
	putString(time_string,&ledLineGeraffel);

	ledLineToDraw = &ledLineGeraffel;
//	ledLineToDraw = &ledLineTime;
	while(running)
	{
		if(led_stack_size)
		{
			ledLineToDraw = &ledLineStack[led_stack_size-1];
			led_line_stack_time[led_stack_size-1]--;
			shift_speed = led_line_stack_shift[led_stack_size-1];
			if(!led_line_stack_time[led_stack_size-1])
			{
				ledPopFromStack();
			}
			else
				ledDisplayMain(ledLineToDraw, shift_speed);

		}
		/* Important! else doesn't work here */
/*		if(!led_stack_size)
		{
			time(&rawtime);
			ptm = localtime(&rawtime);
			sprintf(time_string,"\r%02d:%02d:%02d",ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
			clearScreen(&ledLineTime);
			ledLineTime.x = (64-stringWidth(time_string))/2;
			ledLineTime.y = 2;
			putString(time_string,&ledLineTime);
			ledLineToDraw = &ledLineTime;
			shift_speed = 0;
			ledDisplayMain(ledLineToDraw, shift_speed);
		}*/
		if(!led_stack_size)
		{
			shift_speed = 40000;
			ledLineToDraw = &ledLineGeraffel;
			ledDisplayMain(ledLineToDraw, shift_speed);
		}
		
	}

	freeLedLine(&ledLineTime);
	
	close(client_sock);

	pthread_exit(0);
}

static void ledDisplayMain(struct _ledLine *ledLineToDraw, int shift_speed)
{
	static int counter = 0;
	if(shift_speed)
	{
		shiftLeft(ledLineToDraw);
		updateDisplay(*ledLineToDraw);
		usleep(40000);
	}
	else
	{
		updateDisplay(*ledLineToDraw);
		usleep(1000);
	}
}

