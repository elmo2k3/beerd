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
#include <glib/gprintf.h>
#include "misc.h"
#include "tag_database.h"

#define CREATE_TABLE_TAGS_QUERY "CREATE TABLE tags ( tag TEXT, user_id INTEGER,permission INTEGER)"
#define CREATE_TABLE_USERS_QUERY "CREATE TABLE users ( name TEXT, surname TEXT, nick TEXT, email TEXT, permission INTEGER)"
#define CREATE_TABLE_ACTIONS_QUERY "CREATE TABLE actions ( timestamp INTEGER, action_id INTEGER, action_value1 INTEGER, action_value2 INTEGER)"

#define SELECT_TAG_QUERY "SELECT * FROM tags WHERE tag=?"
#define SELECT_USER_QUERY "SELECT * FROM users WHERE rowid=?"

#define INSERT_ACTION_QUERY "INSERT INTO actions (timestamp, action_id, action_value1, action_value2) VALUES (?,?,?,?)"

static int createDatabaseLayout(struct TagDatabase *database);

extern struct TagDatabase *tag_database_new(char *filename)
{
    struct TagDatabase *database = g_new0(struct TagDatabase, 1);

	if(fileExists(filename))
	{
		int rc;
		rc = sqlite3_open(filename, &database->db);
		if(rc != SQLITE_OK)
		{
			fprintf(stderr,"could not open database: %s", filename);
			sqlite3_close(database->db);
            g_free(database);
			return NULL;
		}
	}
	else // create the database
	{
		int rc;
		rc = sqlite3_open(filename, &database->db);
		if(rc)
		{
			fprintf(stderr,"could not open database: %s", filename);
			sqlite3_close(database->db);
            g_free(database);
			return NULL;
		}
		if(createDatabaseLayout(database))
		{
			sqlite3_close(database->db);
            g_free(database);
			return NULL;
		}
	}
	return database;
}

static int createDatabaseLayout(struct TagDatabase *database)
{
	int rc;
	char *zErrMsg = 0;

	rc = sqlite3_exec(database->db, CREATE_TABLE_TAGS_QUERY,  0, 0, &zErrMsg);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"sqlerror: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return -1;
	}
	rc = sqlite3_exec(database->db, CREATE_TABLE_USERS_QUERY,  0, 0, &zErrMsg);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"sqlerror: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return -1;
	}
	rc = sqlite3_exec(database->db, CREATE_TABLE_ACTIONS_QUERY,  0, 0, &zErrMsg);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"sqlerror: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return -1;
	}
	return 0;
}

void tag_database_set_callback_auth_successfull(struct TagDatabase *database, void *callback)
{
    database->auth_successfull = callback;
}

void tag_database_set_callback_auth_failed(struct TagDatabase *database, void *callback)
{
    database->auth_failed = callback;
}

gint tag_database_tag_exists(struct TagDatabase *database, gchar *tagid)
{
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(database->db, SELECT_TAG_QUERY, 2048, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_TAG_QUERY\n");
		return 0;
	}
	sqlite3_bind_text(stmt, 1, tagid, -1, NULL);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if(rc == SQLITE_ROW)
		return 1;
	return 0;
}

static sqlite_int64 tag_database_get_user_id(struct TagDatabase *database, gchar *tagid)
{
	int rc;
	sqlite_int64 user_id;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(database->db, SELECT_TAG_QUERY, 2048, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_TAG_QUERY\n");
		return 0;
	}
	sqlite3_bind_text(stmt, 1, tagid, -1, NULL);
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		user_id = sqlite3_column_int64(stmt, 1);
		sqlite3_finalize(stmt);
		return user_id;
	}
	sqlite3_finalize(stmt);
	return 0;
}

struct TagUser *tag_database_user_get_by_tag(struct TagDatabase *database, gchar *tagid)
{
	int rc;
	sqlite3_stmt *stmt;
	sqlite3_int64 user_id;

	struct TagUser *user = g_new0(struct TagUser, 1);
	if(!user)
		return NULL;

	user_id = tag_database_get_user_id(database, tagid);
	if(!user_id)
	{
		g_free(user);
		return NULL;
	}

	rc = sqlite3_prepare_v2(database->db, SELECT_USER_QUERY, 2048, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_USER_QUERY\n");
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		user->id = (gint)user_id;
		g_strlcpy(user->name, (gchar*)sqlite3_column_text(stmt,0), sizeof(user->name));
		g_strlcpy(user->surname, (gchar*)sqlite3_column_text(stmt,1), sizeof(user->surname));
		g_strlcpy(user->nick, (gchar*)sqlite3_column_text(stmt,2), sizeof(user->nick));
		g_strlcpy(user->email, (gchar*)sqlite3_column_text(stmt,3), sizeof(user->email));
		user->permission = (gint)sqlite3_column_int64(stmt,4);
		sqlite3_finalize(stmt);
		return user;
	}
	sqlite3_finalize(stmt);
	g_free(user);
	return NULL;
}

gint tag_database_action_insert
(struct TagDatabase *database, time_t timestamp, gint action_id, gchar *value1, gchar *value2)
{
	int rc;
	sqlite3_stmt *stmt;
	
	rc = sqlite3_prepare_v2(database->db, INSERT_ACTION_QUERY, 2048, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error INSERT_ACTION_QUERY\n");
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, (sqlite3_int64)timestamp);
	sqlite3_bind_int64(stmt, 2, (sqlite3_int64)action_id);
	sqlite3_bind_text(stmt, 3, value1, -1, NULL);
	sqlite3_bind_text(stmt, 4, value2, -1, NULL);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return 1;
}
