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

#ifndef __TAG_DATABASE_H__
#define __TAG_DATABASE_H__

/** @file tag_database.h
 * Sqlite database handling
 */

#define _HAVE_MYSQL

#include <stdio.h>
#include <glib.h>
#include <sqlite3.h>

#ifdef _HAVE_MYSQL
#include <mysql/mysql.h>
#endif

#include "rfid_tag_reader.h"

enum action
{
    ACTION_TAG_READ,
    ACTION_BEER_DRAWN
};

struct TagDatabase
{
#ifdef _HAVE_MYSQL
    MYSQL *mysql;
#endif
    sqlite3 *db;
    gboolean sqlite_usable;
    gboolean mysql_usable;
    gchar error_string[1024];
    void (*auth_successfull)(void*);
    void (*auth_failed)(void*);
};

struct TagUser
{
    gint id;
    gchar name[128];
    gchar surname[128];
    gchar nick[128];
    gchar email[128];
    gchar password[128];
    gint age;
    gint weight;
    gint size;
    gint gender;
    gint permission;
};

struct Tag
{
    gint rowid;
    gchar tagid[11];
    gint userid;
    gint permission;
};

struct TagAction
{
    gint rowid;
    time_t timestamp;
    gint action_id;
    gchar action_value1[128];
    gchar action_value2[128];
};

struct TagLiters
{
    gchar tagid[20];
    int liters;
};

extern struct TagDatabase *tag_database_new
(char *filename);

extern void tag_database_set_callback_auth_successfull
(struct TagDatabase *database, void *callback);

extern void tag_database_set_callback_auth_failed
(struct TagDatabase *database, void *callback);


extern gint tag_database_tag_exists
(struct TagDatabase *database, gchar *tagid);

extern gint tag_database_user_get_by_tag
(struct TagDatabase *database, gchar *tagid, struct TagUser *user);

extern gint tag_database_user_insert
(struct TagDatabase *database, struct TagUser *user);

extern gint tag_database_user_update
(struct TagDatabase *database, struct TagUser *user);

extern gint tag_database_tag_insert
(struct TagDatabase *database, gchar *tagid, gint user_id, gint permission);

extern gint tag_database_action_insert
(struct TagDatabase *database, time_t timestamp, gint action_id, gchar *value1, gchar *value2);

extern gchar *tag_database_tag_last_read
(struct TagDatabase *database, time_t *timestamp);

extern gint tag_database_tag_insert
(struct TagDatabase *database, gchar *tagid, gint userid, gint permission);

extern gint tag_database_user_get_by_id
(struct TagDatabase *database, gint user_id, struct TagUser *user);

extern gint tag_database_user_get_permission
(struct TagDatabase *database, gchar *nick, gchar *password, gchar *random_number);

extern gint tag_database_user_get_all(struct TagDatabase *database, struct TagUser **users);
extern gint tag_database_tags_get_all(struct TagDatabase *database, struct Tag **tags);
extern gint tag_database_actions_get_all(struct TagDatabase *database, struct TagAction **actions);

extern gint tag_database_get_liters_per_tag
(struct TagDatabase *database, struct TagLiters **liters, time_t timestamp);

#endif

