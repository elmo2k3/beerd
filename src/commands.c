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
static enum commands_status action_insert_tag(struct client *client, int argc, char **argv);
static enum commands_status action_get_user_by_id(struct client *client, int argc, char **argv);
static enum commands_status action_get_user_by_tag(struct client *client, int argc, char **argv);

#define NUM_COMMANDS 7
static struct command commands[] = {
	{"commands", 0, 0, action_commands},
	{"last_tagid", 0, 0, action_last_tagid},
	{"quit", 0, 0, action_disconnect},
	{"insert_user", 9, 9, action_insert_user},
	{"insert_tag",3,3, action_insert_tag},
	{"get_user_by_id",1,1, action_get_user_by_id},
	{"get_user_by_tag",1,1, action_get_user_by_tag}
	};

static enum commands_status action_get_user_by_tag(struct client *client, int argc, char **argv)
{
	struct TagUser user;

	if(!tag_database_user_get_by_tag(client->database, (gchar*)argv[1], &user))
		return COMMANDS_FAIL;
	network_client_printf(client, "name: %s\r\n", user.name);
	network_client_printf(client, "surname: %s\r\n", user.surname);
	network_client_printf(client, "nick: %s\r\n", user.nick);
	network_client_printf(client, "email: %s\r\n", user.email);
	network_client_printf(client, "age: %d\r\n", user.age);
	network_client_printf(client, "weight: %d\r\n", user.weight);
	network_client_printf(client, "size: %d\r\n", user.size);
	network_client_printf(client, "gender: %d\r\n", user.gender);
	network_client_printf(client, "permission: %d\r\n", user.permission);
	return COMMANDS_OK;
}

static enum commands_status action_get_user_by_id(struct client *client, int argc, char **argv)
{
	struct TagUser user;

	if(!tag_database_user_get_by_id(client->database, (gint)atoi(argv[1]), &user))
		return COMMANDS_FAIL;
	network_client_printf(client, "name: %s\r\n", user.name);
	network_client_printf(client, "surname: %s\r\n", user.surname);
	network_client_printf(client, "nick: %s\r\n", user.nick);
	network_client_printf(client, "email: %s\r\n", user.email);
	network_client_printf(client, "age: %d\r\n", user.age);
	network_client_printf(client, "weight: %d\r\n", user.weight);
	network_client_printf(client, "size: %d\r\n", user.size);
	network_client_printf(client, "gender: %d\r\n", user.gender);
	network_client_printf(client, "permission: %d\r\n", user.permission);
	return COMMANDS_OK;
}

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

static enum commands_status action_insert_tag(struct client *client, int argc, char **argv)
{
	tag_database_tag_insert(client->database, argv[1], (gint)atoi(argv[2]), (gint)atoi(argv[3]));
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
