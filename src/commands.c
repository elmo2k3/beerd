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
    enum NetworkClientPermission permission; 
    enum commands_status (*action)(struct client *client, int argc, char **argv);
};

static enum commands_status action_commands(struct client *client, int argc, char **argv);
static enum commands_status action_last_tagid(struct client *client, int argc, char **argv);
static enum commands_status action_disconnect(struct client *client, int argc, char **argv);
static enum commands_status action_insert_user(struct client *client, int argc, char **argv);
static enum commands_status action_insert_user_with_tag(struct client *client, int argc, char **argv);
static enum commands_status action_insert_tag(struct client *client, int argc, char **argv);
static enum commands_status action_get_user_by_id(struct client *client, int argc, char **argv);
static enum commands_status action_get_user_by_tag(struct client *client, int argc, char **argv);
static enum commands_status action_auth(struct client *client, int argc, char **argv);
static enum commands_status action_get_all_users(struct client *client, int argc, char **argv);
static enum commands_status action_get_all_tags(struct client *client, int argc, char **argv);
static enum commands_status action_get_all_actions(struct client *client, int argc, char **argv);
static enum commands_status action_get_auth_string(struct client *client, int argc, char **argv);
static enum commands_status action_update_user(struct client *client, int argc, char **argv);
static enum commands_status action_get_all_tagliters(struct client *client, int argc, char **argv);

#define NUM_COMMANDS 15
static struct command commands[] = {
    {"commands", 0, 0,      NETWORK_CLIENT_PERMISSION_NONE, action_commands},
    {"last_tagid", 0, 0,    NETWORK_CLIENT_PERMISSION_READ, action_last_tagid},
    {"quit", 0, 0,          NETWORK_CLIENT_PERMISSION_NONE, action_disconnect},
    {"insert_user", 10, 10, NETWORK_CLIENT_PERMISSION_ADMIN, action_insert_user},
    {"insert_tag",3,3,      NETWORK_CLIENT_PERMISSION_ADMIN, action_insert_tag},
    {"get_user_by_id",1,1,  NETWORK_CLIENT_PERMISSION_READ, action_get_user_by_id},
    {"get_user_by_tag",1,1, NETWORK_CLIENT_PERMISSION_READ, action_get_user_by_tag},
    {"auth",2,2,            NETWORK_CLIENT_PERMISSION_NONE, action_auth},
    {"get_all_users",0,0,   NETWORK_CLIENT_PERMISSION_READ, action_get_all_users},
    {"get_all_tags",0,0,    NETWORK_CLIENT_PERMISSION_READ, action_get_all_tags},
    {"get_all_actions",0,0, NETWORK_CLIENT_PERMISSION_READ, action_get_all_actions},
    {"get_auth_string",0,0, NETWORK_CLIENT_PERMISSION_NONE, action_get_auth_string},
    {"update_user",11,11, NETWORK_CLIENT_PERMISSION_ADMIN, action_update_user},
    {"user_insert_with_tag",12,12, NETWORK_CLIENT_PERMISSION_ADMIN, action_insert_user_with_tag},
    {"get_all_tagliters",0,1, NETWORK_CLIENT_PERMISSION_READ, action_get_all_tagliters},
    };

static enum commands_status action_get_auth_string(struct client *client, int argc, char **argv)
{
    network_client_printf(client,"auth_string: %s\r\n",client->random_number);
    return COMMANDS_OK;
}

static void print_user(struct client *client, struct TagUser *user)
{
    network_client_printf(client, "id: %d\r\n", user->id);
    network_client_printf(client, "name: %s\r\n", user->name);
    network_client_printf(client, "surname: %s\r\n", user->surname);
    network_client_printf(client, "nick: %s\r\n", user->nick);
    network_client_printf(client, "email: %s\r\n", user->email);
    network_client_printf(client, "age: %d\r\n", user->age);
    network_client_printf(client, "weight: %d\r\n", user->weight);
    network_client_printf(client, "size: %d\r\n", user->size);
    network_client_printf(client, "gender: %d\r\n", user->gender);
    network_client_printf(client, "permission: %d\r\n", user->permission);
}

static enum commands_status action_get_all_tags(struct client *client, int argc, char **argv)
{
    struct Tag *tags;
    gint num;
    int i;

    num = tag_database_tags_get_all(client->database, &tags);
    for(i=0;i<num;i++)
    {
        network_client_printf(client,"rowid: %d\r\n",tags[i].rowid);
        network_client_printf(client,"tagid: %s\r\n",tags[i].tagid);
        network_client_printf(client,"userid: %d\r\n",tags[i].userid);
        network_client_printf(client,"permission: %d\r\n",tags[i].permission);
    }
    if(num)
        g_free(tags);
    return COMMANDS_OK;
}

static enum commands_status action_get_all_tagliters(struct client *client, int argc, char **argv)
{
    struct TagLiters *tagliters;
    gint num;
    int i;
    struct TagUser user;
    
    if(argc == 1)
    {
        num = tag_database_get_liters_per_tag(client->database, &tagliters, atoi(argv[1]));
    }
    else
    {
        num = tag_database_get_liters_per_tag(client->database, &tagliters, 0);
    }
    for(i=0;i<num;i++)
    {
        network_client_printf(client,"tagid: %s\r\n",tagliters[i].tagid);
        if(tag_database_user_get_by_tag(client->database, tagliters[i].tagid, &user))
            network_client_printf(client,"user: %s\r\n", user.nick);
        network_client_printf(client,"liters: %d\r\n",tagliters[i].liters);
    }
    if(num)
        g_free(tagliters);
    return COMMANDS_OK;
}

static enum commands_status action_get_all_actions(struct client *client, int argc, char **argv)
{
    struct TagAction *actions;
    gint num;
    int i;

    num = tag_database_actions_get_all(client->database, &actions);
    for(i=0;i<num;i++)
    {
        network_client_printf(client,"rowid: %d\r\n",actions[i].rowid);
        network_client_printf(client,"timestamp: %d\r\n",actions[i].timestamp);
        network_client_printf(client,"action_id: %d\r\n",actions[i].action_id);
        network_client_printf(client,"action_value1: %s\r\n",actions[i].action_value1);
        network_client_printf(client,"action_value2: %s\r\n",actions[i].action_value2);
    }
    if(num)
        g_free(actions);
    return COMMANDS_OK;
}

static enum commands_status action_get_all_users(struct client *client, int argc, char **argv)
{
    struct TagUser *users;
    gint num;
    int i;

    num = tag_database_user_get_all(client->database, &users);
    for(i=0;i<num;i++)
    {
        print_user(client,&users[i]);
    }
    if(num)
        g_free(users);
    return COMMANDS_OK;
}

static enum commands_status action_get_user_by_tag(struct client *client, int argc, char **argv)
{
    struct TagUser user;

    if(!tag_database_user_get_by_tag(client->database, (gchar*)argv[1], &user))
        return COMMANDS_FAIL;
    print_user(client, &user);
    return COMMANDS_OK;
}
static enum commands_status action_get_user_by_id(struct client *client, int argc, char **argv)
{
    struct TagUser user;

    if(!tag_database_user_get_by_id(client->database, (gint)atoi(argv[1]), &user))
        return COMMANDS_FAIL;
    print_user(client,&user);
    return COMMANDS_OK;
}

static enum commands_status action_auth(struct client *client, int argc, char **argv)
{
    client->permission = tag_database_user_get_permission
       (client->database, argv[1], argv[2], client->random_number);
    switch(client->permission)
    {
        case NETWORK_CLIENT_PERMISSION_NONE: 
                    network_client_printf(client,"permission: NONE\r\n"); break;
        case NETWORK_CLIENT_PERMISSION_READ: 
                    network_client_printf(client,"permission: READ\r\n"); break;
        case NETWORK_CLIENT_PERMISSION_ADMIN: 
                    network_client_printf(client,"permission: ADMIN\r\n"); break;
    }
    return COMMANDS_OK;
}


static enum commands_status action_insert_user(struct client *client, int argc, char **argv)
{
    struct TagUser user;
    GChecksum *checksum;

    checksum = g_checksum_new(G_CHECKSUM_SHA1);
    g_checksum_update(checksum, (guchar*)argv[10], strlen(argv[10]));
    g_strlcpy(user.name, argv[1], sizeof(user.name));
    g_strlcpy(user.surname, argv[2], sizeof(user.surname));
    g_strlcpy(user.nick, argv[3], sizeof(user.nick));
    g_strlcpy(user.email, argv[4], sizeof(user.email));
    user.age = atoi(argv[5]);
    user.weight = atoi(argv[6]);
    user.size = atoi(argv[7]);
    user.gender = atoi(argv[8]);
    user.permission = atoi(argv[9]);
    g_strlcpy(user.password, g_checksum_get_string(checksum), sizeof(user.password));
    g_checksum_free(checksum);
    
    if(!tag_database_user_insert(client->database, &user))
        return COMMANDS_FAIL;
    return COMMANDS_OK;
}

static enum commands_status action_insert_user_with_tag(struct client *client, int argc, char **argv)
{
    if(action_insert_user(client, argc, argv) != COMMANDS_OK)
        return COMMANDS_FAIL;
    if(!tag_database_tag_insert(client->database, argv[11], 
        (gint)sqlite3_last_insert_rowid(client->database->db), (gint)atoi(argv[12])))
        return COMMANDS_FAIL;
    return COMMANDS_OK;
}

static enum commands_status action_update_user(struct client *client, int argc, char **argv)
{
    struct TagUser user;
    GChecksum *checksum;

    if(strlen(argv[11]))
    {
        checksum = g_checksum_new(G_CHECKSUM_SHA1);
        g_checksum_update(checksum, (guchar*)argv[11], strlen(argv[11]));
        g_strlcpy(user.password, g_checksum_get_string(checksum), sizeof(user.password));
        g_checksum_free(checksum);
    }
    else
        user.password[0] = '\0';

    user.id = atoi(argv[1]);
    g_strlcpy(user.name, argv[2], sizeof(user.name));
    g_strlcpy(user.surname, argv[3], sizeof(user.surname));
    g_strlcpy(user.nick, argv[4], sizeof(user.nick));
    g_strlcpy(user.email, argv[5], sizeof(user.email));
    user.age = atoi(argv[6]);
    user.weight = atoi(argv[7]);
    user.size = atoi(argv[8]);
    user.gender = atoi(argv[9]);
    user.permission = atoi(argv[10]);
    if(!tag_database_user_update(client->database, &user))
        return COMMANDS_FAIL;
    return COMMANDS_OK;
}

static enum commands_status action_insert_tag(struct client *client, int argc, char **argv)
{
    if(!tag_database_tag_insert(client->database, argv[1], (gint)atoi(argv[2]), (gint)atoi(argv[3])))
        return COMMANDS_FAIL;
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
    struct TagUser user; 

    last_tag = tag_database_tag_last_read(client->database, &timestamp);
    localtime_r(&timestamp, &ptm);
    strftime(date, sizeof(date), "%c", &ptm);
    
    network_client_printf(client, "time_last_tagid: %s\r\n",date);
    network_client_printf(client, "last_tagid: %s\r\n",last_tag);
    if(tag_database_user_get_by_tag(client->database, last_tag, &user))
        network_client_printf(client, "user_id: %d\r\n", user.id);
    else
        network_client_printf(client, "user_id: 0\r\n");

    return COMMANDS_OK;
}

enum commands_status commands_process(struct client *client, gchar *cmdline)
{
    int i;
    gchar *pos;
    gint ret;
    char *argv[1024] = { NULL };
    int argc;
    
    if(strcmp(cmdline,"last_tagid\n"))
        g_debug("processing command from client %d %s: %s", client->num,client->addr_string, cmdline);
    
    ret = COMMANDS_FAIL;
    for(i = 0;i < NUM_COMMANDS; i++)
    {
        pos = g_strstr_len(cmdline, -1, commands[i].cmd);
        if(pos == cmdline) // command must be at the beginning of the line
        {
            network_client_printf(client,"command: %s\r\n",commands[i].cmd);
            argc = buffer2array(cmdline, argv, 1024) -1 ;
            if(argc < commands[i].min_params || argc > commands[i].max_params)
                return COMMANDS_FAIL;
            if(client->permission >= commands[i].permission)
                ret = commands[i].action(client, argc, argv);
            else
                ret = COMMANDS_DENIED;
        }
    }
    return ret;
}
