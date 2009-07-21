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
#include <stdio.h>
#include <glib.h>

#include "configfile.h"
#include "rfid_tag_reader.h"
#include "tag_database.h"
#include "network.h"

void tag_read(struct RfidTagReader *tag_reader, void *user_data)
{
	struct TagUser user;
	gint auth_successfull;
	time_t rawtime;
	struct TagDatabase *database = (struct TagDatabase*)user_data;
	
	time(&rawtime);
	tag_database_action_insert(database, rawtime, ACTION_TAG_READ, rfid_tag_reader_last_tag(tag_reader), NULL);
	printf("tag read    id = %s   ",rfid_tag_reader_last_tag(tag_reader));
	auth_successfull = tag_database_tag_exists(database, tag_reader->tagid);
	if(auth_successfull)
	{
		printf("auth successfull   ");
		if(tag_database_user_get_by_tag(database, tag_reader->tagid, &user))
		{
			printf("nick = %s", user.nick);
		}
		printf("\n");
	}
	else
		printf("auth not successfull\n");
}

int main(int argc, char *argv[])
{
	struct RfidTagReader *tag_reader;
	struct TagDatabase *database;
	struct NetworkServer *server;
    GMainLoop *loop;

	config_load("beerd.conf");

	database = tag_database_new(config.sqlite_file); 

	tag_reader = rfid_tag_reader_new(config.rfid_serial_port);
	if(tag_reader == NULL)
	{
		return -1;
	}
	rfid_tag_reader_set_callback(tag_reader, tag_read, database);

	server = network_server_new(database);
    loop = g_main_loop_new(NULL,FALSE);
    g_main_loop_run(loop);

    return 0;
}
