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
	void (*action)(struct client *client);
};

static void action_last_tagid(struct client *client);

#define NUM_COMMANDS 1
static struct command commands[] = {
	{"last_tagid", 0, 0, action_last_tagid}
	};

static void action_last_tagid(struct client *client)
{
	time_t timestamp;
	gchar *last_tag;
	gsize bytes_written;

	last_tag = tag_database_tag_last_read(client->database, &timestamp);
	g_io_channel_write_chars(client->channel, "last_tagid: ", 12, &bytes_written, NULL);
	g_io_channel_write_chars(client->channel, last_tag, strlen(last_tag), &bytes_written, NULL);
	g_io_channel_write_chars(client->channel, "\r\n", 2, &bytes_written, NULL);
}

gboolean commands_process(struct client *client)
{
	int i;
	gboolean found = FALSE;
	gchar *pos;

	found = FALSE;
	for(i = 0;i < NUM_COMMANDS; i++)
	{
		pos = g_strstr_len(client->buf, -1, commands[i].cmd);
		if(pos == client->buf) // command must be at the beginning of the line
		{
			commands[i].action(client);
			found = TRUE;
		}
	}
	return found;
}
