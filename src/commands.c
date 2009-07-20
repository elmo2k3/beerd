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

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include "network.h"
#include "commands.h"
#include "tag_database.h"
#include "misc.h"

struct command
{
	gchar *cmd;
	gint min_params;
	gint max_params;
	enum commands_status (*action)(struct client *client, int argc, char **argv);
};

static enum commands_status action_commands(struct client *client, int argc, char **argv);
static enum commands_status action_last_tagid(struct client *client, int argc, char **argv);
static enum commands_status action_disconnect(struct client *client, int argc, char **argv);
static enum commands_status action_insert_user(struct client *client, int argc, char **argv);

#define NUM_COMMANDS 4
static struct command commands[] = {
	{"commands", 0, 0, action_commands},
	{"last_tagid", 0, 0, action_last_tagid},
	{"quit", 0, 0, action_disconnect},
	{"insert_user", 9, 9, action_insert_user}
	};

static enum commands_status action_insert_user(struct client *client, int argc, char **argv)
{
	struct TagUser user;
	g_strlcpy(user.name, argv[1], sizeof(user.name));
	g_strlcpy(user.surname, argv[2], sizeof(user.surname));
	g_strlcpy(user.nick, argv[3], sizeof(user.nick));
	g_strlcpy(user.email, argv[4], sizeof(user.email));
	user.age = atoi(argv[5]);
	user.weight = atoi(argv[6]);
	user.size = atoi(argv[7]);
	user.gender = atoi(argv[8]);
	user.permission = atoi(argv[9]);
	tag_database_user_insert(client->database, &user);
	return COMMANDS_OK;
}

static enum commands_status action_disconnect(struct client *client, int argc, char **argv)
{
	network_client_disconnect(client);
	return COMMANDS_DISCONNECT;
}

static enum commands_status action_commands(struct client *client, int argc, char **argv)
{
	int i;
	network_client_printf(client,"Available commands:\r\n");
	for(i=1; i< NUM_COMMANDS; i++)
	{
		network_client_printf(client,"%s\r\n",commands[i].cmd);
	}
	return COMMANDS_OK;
}

static enum commands_status action_last_tagid(struct client *client, int argc, char **argv)
{
	time_t timestamp;
	gchar *last_tag;
	gchar date[256];
	struct tm ptm;

	last_tag = tag_database_tag_last_read(client->database, &timestamp);
	localtime_r(&timestamp, &ptm);
	strftime(date, sizeof(date), "%c", &ptm);
	
	g_debug("argv[0] = %s argv[1] = %s",argv[0],argv[1]);
	if(!network_client_printf(client, "time_last_tagid: %s\r\n",date))
		return COMMANDS_DISCONNECT;
	if(!network_client_printf(client, "last_tagid: %s\r\n",last_tag))
		return COMMANDS_DISCONNECT;
/*	g_io_channel_write_chars(client->channel, "last_tagid: ", 12, &bytes_written, NULL);
	g_io_channel_write_chars(client->channel, last_tag, strlen(last_tag), &bytes_written, NULL);
	g_io_channel_write_chars(client->channel, "\r\n", 2, &bytes_written, NULL);*/
	return COMMANDS_OK;
}

enum commands_status commands_process(struct client *client)
{
	int i;
	gchar *pos;
	gint ret;
	char *argv[1024] = { NULL };
	int argc;
	
	g_debug("processing command from client %d: %s", client->num, client->buf);
	ret = COMMANDS_FAIL;
	for(i = 0;i < NUM_COMMANDS; i++)
	{
		pos = g_strstr_len(client->buf, -1, commands[i].cmd);
		if(pos == client->buf) // command must be at the beginning of the line
		{
			argc = buffer2array(client->buf, argv, 1024) -1 ;
			if(argc < commands[i].min_params || argc > commands[i].max_params)
				return COMMANDS_FAIL;
			ret = commands[i].action(client, argc, argv);
		}
	}
	return ret;
}
