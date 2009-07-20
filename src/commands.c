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
#include "network.h"
#include "commands.h"
#include "tag_database.h"

struct command
{
	gchar *cmd;
	gint min_params;
	gint max_params;
	enum commands_status (*action)(struct client *client);
};

static enum commands_status action_commands(struct client *client);
static enum commands_status action_last_tagid(struct client *client);
static enum commands_status action_disconnect(struct client *client);

#define NUM_COMMANDS 3
static struct command commands[] = {
	{"commands", 0, 0, action_commands},
	{"last_tagid", 0, 0, action_last_tagid},
	{"quit", 0, 0, action_disconnect}
	};

static enum commands_status action_disconnect(struct client *client)
{
	network_client_disconnect(client);
	return COMMANDS_DISCONNECT;
}

static enum commands_status action_commands(struct client *client)
{
	int i;
	network_client_printf(client,"Available commands:\r\n");
	for(i=1; i< NUM_COMMANDS; i++)
	{
		network_client_printf(client,"%s\r\n",commands[i].cmd);
	}
	return COMMANDS_OK;
}

static enum commands_status action_last_tagid(struct client *client)
{
	time_t timestamp;
	gchar *last_tag;
	gchar date[256];
	struct tm ptm;

	last_tag = tag_database_tag_last_read(client->database, &timestamp);
	localtime_r(&timestamp, &ptm);
	strftime(date, sizeof(date), "%c", &ptm);
	
	network_client_printf(client, "time_last_tagid: %s\r\n",date);
	network_client_printf(client, "last_tagid: %s\r\n",last_tag);
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

	ret = COMMANDS_FAIL;
	for(i = 0;i < NUM_COMMANDS; i++)
	{
		pos = g_strstr_len(client->buf, -1, commands[i].cmd);
		if(pos == client->buf) // command must be at the beginning of the line
		{
			ret = commands[i].action(client);
		}
	}
	return ret;
}
