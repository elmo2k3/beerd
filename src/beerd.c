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
#include <pthread.h>

#include "configfile.h"
#include "rfid_tag_reader.h"
#include "tag_database.h"
#include "network.h"
#include "commandline.h"
#include "led_routines.h"

pthread_t led_thread;

void tag_read(struct RfidTagReader *tag_reader, void *user_data)
{
    struct TagUser user;
    gint auth_successfull;
    time_t rawtime;
    struct TagDatabase *database = (struct TagDatabase*)user_data;
    
    time(&rawtime);
    tag_database_action_insert(database, rawtime, ACTION_TAG_READ, rfid_tag_reader_last_tag(tag_reader), NULL);
    g_debug("tag read    id = %s   ",rfid_tag_reader_last_tag(tag_reader));
    auth_successfull = tag_database_tag_exists(database, tag_reader->tagid);
    if(auth_successfull)
    {
        if(tag_database_user_get_by_tag(database, tag_reader->tagid, &user))
        {
            g_debug("tag known: nick = %s", user.nick);
        }
    }
    else
    {
        g_debug("tag unknown");
    }
}

static void logfunc
(const gchar *log_domain,GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
}

int main(int argc, char *argv[])
{
    struct RfidTagReader *tag_reader;
    struct TagDatabase *database;
    struct NetworkServer *server;
    GMainLoop *loop;
    struct CmdOptions options;
    int i;

    command_line_parse(&options, argc, argv);
    config_load("beerd.conf");

    database = tag_database_new(config.sqlite_file); 

    if(!options.disable_tagreader)
    {
        for(i=0;i < config.num_rfid_readers; i++)
        {
            tag_reader = rfid_tag_reader_new(config.rfid_serial_port[i]);
            if(tag_reader)
                rfid_tag_reader_set_callback(tag_reader, tag_read, database);
            else
                fprintf(stderr,"Error opening serial device for tagreader\n");
        }
    }

    server = network_server_new(database);
    pthread_create(&led_thread,NULL,(void*)&ledMatrixThread,NULL);
    loop = g_main_loop_new(NULL,FALSE);
//    g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, logfunc, NULL);
    g_main_loop_run(loop);

    return 0;
}
