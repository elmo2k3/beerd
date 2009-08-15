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
#include "beer_volume_reader.h"
#include "tag_database.h"
#include "network.h"
#include "commandline.h"
#include "led_routines.h"

pthread_t led_thread;

char last_message[1024];

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
            char buf[1024];
            struct TagLiters *tagliters;
            struct TagUser temp_user;
            int liters = 0;
            int num;
            int i;
        
            num = tag_database_get_liters_per_tag(database, &tagliters, time(NULL)-43200);
            for(i=0;i<num;i++)
            {
                if(tag_database_user_get_by_tag(database, tagliters[i].tagid, &temp_user))
                {
                    if(!g_strcmp0(user.nick, temp_user.nick))
                    {
                        liters += tagliters[i].liters;
                    }
                }
            }
            if(num)
                g_free(tagliters);

            g_debug("tag known: nick = %s", user.nick);
            sprintf(buf,"\b~~~~~\a %s \rdraws a b33r \a %s \rdrank \a%2.2f\bL\r today",
                user.nick, user.nick, (float)liters/100.0);
            if(g_strcmp0(last_message, buf))
                ledPushToStack(buf, 1, 1);
            g_strlcpy(last_message, buf, 1024);
            beer_volume_reader_control_valve(tag_reader->beer_volume_reader, VALVE_OPEN);
            if(tag_reader->beer_volume_reader)
            {
                if(tag_reader->beer_volume_reader->timeout_source)
                    g_source_remove(tag_reader->beer_volume_reader->timeout_source);
                tag_reader->beer_volume_reader->timeout_source = 
                    g_timeout_add_seconds(30, (GSourceFunc)beer_volume_reader_close_valve,
                    tag_reader->beer_volume_reader);
            }
        }

    }
    else
    {
        g_debug("tag unknown");
        char buf[1024];
        sprintf(buf,"\r~~~UNAUTHORIZED TAG SCANNED!!~~~");
        ledPushToStack(buf, 1, 2);
    }
}

void volume_read(struct BeerVolumeReader *beer_volume_reader, void *user_data)
{
    time_t rawtime;
    char last_barrel[20], last_overall[20];
    struct TagDatabase *database = (struct TagDatabase*)user_data;
    
    time(&rawtime);
    sprintf(last_barrel,"%d",beer_volume_reader->last_barrel);
    sprintf(last_overall,"%d",beer_volume_reader->last_overall);
    tag_database_action_insert(database, rawtime, ACTION_BEER_DRAWN, last_barrel, 
        last_overall);
    
    g_debug("volume read: barrel = %d   overall = %d",
        beer_volume_reader->last_barrel, beer_volume_reader->last_overall);
    beer_volume_reader_control_valve(beer_volume_reader, VALVE_CLOSE);
    
    if(g_source_remove(beer_volume_reader->timeout_source))
        g_debug("removed timeout source");
    else
        g_debug("not removed timeout source");
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
    struct BeerVolumeReader *volume_reader;
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

    volume_reader = beer_volume_reader_new(config.beer_volume_reader);
    if(volume_reader)
    {
        beer_volume_reader_set_callback(volume_reader, volume_read, database);
        tag_reader->beer_volume_reader = volume_reader;
        beer_volume_reader_control_valve(volume_reader, VALVE_CLOSE);
    }
    else
    {
        fprintf(stderr,"could not connect to valve reader\n");
        return -1;
    }

    server = network_server_new(database);
    pthread_create(&led_thread,NULL,(void*)&ledMatrixThread,NULL);

//    tag_database_update_liters(database);
    g_timeout_add_seconds(30, (GSourceFunc)tag_database_update_liters, database);

    loop = g_main_loop_new(NULL,FALSE);
//    g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, logfunc, NULL);
    g_main_loop_run(loop);

    return 0;
}

