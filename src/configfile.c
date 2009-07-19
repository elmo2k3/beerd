/*
 * Copyright (C) 2009 Bjoern Biesenbach <bjoern@bjoern-b.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*!
* \file	configfile.c
* \brief	config file handling
* \author	Bjoern Biesenbach <bjoern at bjoern-b dot de>
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "configfile.h"

#define NUM_PARAMS 5
static gchar *config_params[NUM_PARAMS] = { "rfid_serial_port", "rfid_timeout", "db_file",
	"port","max_clients"	};

int config_save(char *conf)
{
	FILE *config_file = fopen(conf,"w");
	if(!config_file)
		return 0;
	fprintf(config_file,"rfid_serial_port = %s\n",config.rfid_serial_port);
	fprintf(config_file,"rfid_timeout = %d\n",config.rfid_timeout);
	fprintf(config_file,"db_file = %s\n",config.sqlite_file);
	fprintf(config_file,"port = %d\n",config.server_port);
	fprintf(config_file,"max_clients = %d\n",config.max_clients);
	fclose(config_file);
	return 1;
}

int config_load(char *conf)
{
	FILE *config_file;
	char line[120];
	char value[100];
	char *lpos;
	int param;

	/* set everything to zero */
	memset(&config, 0, sizeof(config));
	
	/* default values */
	strcpy(config.rfid_serial_port, CONFIG_DEFAULT_RFID_SERIAL_PORT);
	config.rfid_timeout = CONFIG_DEFAULT_RFID_TIMEOUT;
	strcpy(config.sqlite_file, CONFIG_DEFAULT_SQLITE_FILE);
	config.server_port = CONFIG_DEFAULT_PORT;
	config.max_clients = CONFIG_DEFAULT_MAX_CLIENTS;

	config_file = fopen(conf,"r");
	if(!config_file)
	{
		return -1;
	}

	/* step through every line */
	while(fgets(line, sizeof(line), config_file) != NULL)
	{
		/* skip comments and empty lines */
		if(line[0] == '#' || line[0] == '\n' || line[0] == '\r')
			continue;
		for(param = 0; param < NUM_PARAMS; param++)
		{
			/* if param name not at the beginning of line */
			if(strstr(line,config_params[param]) != line)
				continue;
			/* go beyond the = */
			if(!(lpos =  strstr(line, "=")))
				continue;
			/* go to the beginning of value 
			 * only whitespaces are skipped, no tabs */
			do
				lpos++;
			while(*lpos == ' ');
			
			strcpy(value, lpos);

			/* throw away carriage return 
			 * might only work for *nix */
			lpos = strchr(value,'\n');
			*lpos = 0;
			if((lpos = strchr(value,'\r')))
				*lpos = 0;

			/* put the value where it belongs */
			switch(param)
			{
				/* serial port for rfid tag reader */
				case 0: strncpy(config.rfid_serial_port,value,
							sizeof(config.rfid_serial_port));
						break;
				/* timeout for rfid tag reader */
				case 1: config.rfid_timeout = atoi(value);
						break;
				/* sqlite database file */
				case 2: strncpy(config.sqlite_file,value,
							sizeof(config.sqlite_file));
						break;
				/* server listen port */
				case 3: config.server_port = atoi(value);
						break;
				/* max number of clients */
				case 4: config.max_clients = atoi(value);
						break;
			}
		}
	}

	fclose(config_file);
	return 0;
}

