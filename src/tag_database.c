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
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "misc.h"
#include "tag_database.h"

#define CREATE_TABLE_TAGS_QUERY "CREATE TABLE tags ( tag TEXT, user_id INTEGER,permission INTEGER)"
#define CREATE_TABLE_USERS_QUERY "CREATE TABLE users ( name TEXT, surname TEXT, nick TEXT, email TEXT, age INTEGER, weight INTEGER, size INTEGER, gender INTEGER, permission INTEGER, password TEXT, pic BLOB)"
#define CREATE_TABLE_ACTIONS_QUERY "CREATE TABLE actions ( timestamp INTEGER, action_id INTEGER, action_value1 INTEGER, action_value2 INTEGER)"

#define SELECT_TAG_QUERY "SELECT * FROM tags WHERE tag=?"
#define SELECT_USER_QUERY "SELECT * FROM users WHERE rowid=?"
#define SELECT_USER_BY_NICK "SELECT password, permission FROM users WHERE nick=?"
#define SELECT_ALL_USER_QUERY "SELECT rowid,* FROM users"
#define SELECT_USER_NUM "SELECT count(*) FROM users"

#define SELECT_ACTION_LAST_READ "SELECT timestamp,action_value1 FROM actions WHERE action_id=? order by timestamp desc LIMIT 1"

#define INSERT_ACTION_QUERY "INSERT INTO actions (timestamp, action_id, action_value1, action_value2) VALUES (?,?,?,?)"
#define INSERT_USER_QUERY "INSERT INTO users (name,surname,nick,email,age,weight,size,gender,permission,password) VALUES (?,?,?,?,?,?,?,?,?,?)"
#define INSERT_TAG_QUERY "INSERT INTO tags (tag,user_id,permission) VALUES (?,?,?)"

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

	rc = sqlite3_prepare_v2(database->db, SELECT_TAG_QUERY, 1024, &stmt, NULL);
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

	rc = sqlite3_prepare_v2(database->db, SELECT_TAG_QUERY, 1024, &stmt, NULL);
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

gint tag_database_user_get_by_tag
(struct TagDatabase *database, gchar *tagid, struct TagUser *user)
{
	int rc;
	sqlite3_stmt *stmt;
	sqlite3_int64 user_id;

	user_id = tag_database_get_user_id(database, tagid);
	if(!user_id)
	{
		return FALSE;
	}

	rc = sqlite3_prepare_v2(database->db, SELECT_USER_QUERY, 1024, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_USER_QUERY\n");
		return FALSE;
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
		user->age = (gint)sqlite3_column_int64(stmt,4);
		user->weight = (gint)sqlite3_column_int64(stmt,5); 
		user->size = (gint)sqlite3_column_int64(stmt,6); 
		user->gender = (gint)sqlite3_column_int64(stmt,7); 
		user->permission = (gint)sqlite3_column_int64(stmt,8);
		sqlite3_finalize(stmt);
		return TRUE;
	}
	sqlite3_finalize(stmt);
	return FALSE;
}

gint tag_database_user_get_by_id
(struct TagDatabase *database, gint user_id, struct TagUser *user)
{
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(database->db, SELECT_USER_QUERY, 1024, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_USER_QUERY\n");
		return FALSE;
	}
	sqlite3_bind_int64(stmt, 1, (sqlite_int64)user_id);
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		user->id = (gint)user_id;
		g_strlcpy(user->name, (gchar*)sqlite3_column_text(stmt,0), sizeof(user->name));
		g_strlcpy(user->surname, (gchar*)sqlite3_column_text(stmt,1), sizeof(user->surname));
		g_strlcpy(user->nick, (gchar*)sqlite3_column_text(stmt,2), sizeof(user->nick));
		g_strlcpy(user->email, (gchar*)sqlite3_column_text(stmt,3), sizeof(user->email));
		user->age = (gint)sqlite3_column_int64(stmt,4);
		user->weight = (gint)sqlite3_column_int64(stmt,5); 
		user->size = (gint)sqlite3_column_int64(stmt,6); 
		user->gender = (gint)sqlite3_column_int64(stmt,7); 
		user->permission = (gint)sqlite3_column_int64(stmt,8);
		sqlite3_finalize(stmt);
		return TRUE;
	}
	sqlite3_finalize(stmt);
	return FALSE;
}

gint tag_database_action_insert
(struct TagDatabase *database, time_t timestamp, gint action_id, gchar *value1, gchar *value2)
{
	int rc;
	sqlite3_stmt *stmt;
	
	rc = sqlite3_prepare_v2(database->db, INSERT_ACTION_QUERY, 1024, &stmt, NULL);
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

gchar *tag_database_tag_last_read
(struct TagDatabase *database, time_t *timestamp)
{
	int rc;
	sqlite3_stmt *stmt;
	gchar *ret;
	
	rc = sqlite3_prepare_v2(database->db, SELECT_ACTION_LAST_READ, 1024, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_ACTION_LAST_READ\n");
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, (sqlite3_int64)ACTION_TAG_READ);
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		*timestamp = sqlite3_column_int64(stmt,0);
		ret = g_strdup((gchar*)sqlite3_column_text(stmt,1));
		sqlite3_finalize(stmt);
	}
	else
	{
		sqlite3_finalize(stmt);
		return NULL;
	}
	return ret;
}

gint tag_database_user_insert
(struct TagDatabase *database, struct TagUser *user)
{
	int rc;
	sqlite3_stmt *stmt;
	
	rc = sqlite3_prepare_v2(database->db, INSERT_USER_QUERY, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error INSERT_USER_QUERY\n");
		return 0;
	}
	sqlite3_bind_text(stmt, 1, user->name, -1, NULL);
	sqlite3_bind_text(stmt, 2, user->surname, -1, NULL);
	sqlite3_bind_text(stmt, 3, user->nick, -1, NULL);
	sqlite3_bind_text(stmt, 4, user->email, -1, NULL);
	sqlite3_bind_int64(stmt, 5, (sqlite3_int64)user->age);
	sqlite3_bind_int64(stmt, 6, (sqlite3_int64)user->weight);
	sqlite3_bind_int64(stmt, 7, (sqlite3_int64)user->size);
	sqlite3_bind_int64(stmt, 8, (sqlite3_int64)user->gender);
	sqlite3_bind_int64(stmt, 9, (sqlite3_int64)user->permission);
	sqlite3_bind_text(stmt, 10, user->password, -1, NULL);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return 1;
}

gint tag_database_tag_insert
(struct TagDatabase *database, gchar *tagid, gint userid, gint permission)
{
	int rc;
	sqlite3_stmt *stmt;
	
	rc = sqlite3_prepare_v2(database->db, INSERT_TAG_QUERY, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error INSERT_TAG_QUERY\n");
		return 0;
	}
	sqlite3_bind_text(stmt, 1, tagid, -1, NULL);
	sqlite3_bind_int64(stmt, 2, (sqlite3_int64)userid);
	sqlite3_bind_int64(stmt, 3, (sqlite3_int64)permission);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return 1;
}

extern gint tag_database_user_get_permission
(struct TagDatabase *database, gchar *nick, gchar *password, time_t rawtime)
{
	int rc;
	sqlite3_stmt *stmt;
	gint permission;
	gchar buffer[512];
	
	/* some checks for plausability */
	if(!rawtime || rawtime < 1248201327 || rawtime > 1910889327)
		return 0;

	rc = sqlite3_prepare_v2(database->db, SELECT_USER_BY_NICK, 1024, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_USER_BY_NICK\n");
		return 0;
	}
	sqlite3_bind_text(stmt, 1, nick, -1, NULL);
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		GChecksum *checksum = g_checksum_new(G_CHECKSUM_SHA256);
		
		g_sprintf(buffer,"%s%ld",sqlite3_column_text(stmt,0), rawtime);
		g_checksum_update(checksum, (guchar*)buffer, strlen(buffer));
		if(g_strcmp0(g_checksum_get_string(checksum), password))
		{
			permission = 0;
		}
		else
			permission = (gint)sqlite3_column_int64(stmt,1);

		g_checksum_free(checksum);
	}
	else
		permission = 0;
	sqlite3_finalize(stmt);
	return permission;
}

gint tag_database_user_get_all(struct TagDatabase *database, struct TagUser **users)
{
	int rc;
	sqlite3_stmt *stmt;
	gint num = 0;
	struct TagUser *user;

	rc = sqlite3_prepare_v2(database->db, SELECT_USER_NUM, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_USER_NUM\n");
		return 0;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		num = (gint)sqlite3_column_int64(stmt,0);
		if(num)
		{
			g_debug("allocating %d structs",num);
			user = g_new0(struct TagUser, num);
		}
	}
	sqlite3_finalize(stmt);
	if(!num)
		return 0;

	rc = sqlite3_prepare_v2(database->db, SELECT_ALL_USER_QUERY, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_ALL_USER_QUERY\n");
		return 0;
	}
	rc = sqlite3_step(stmt);
	num = 0;
	while(rc == SQLITE_ROW)
	{
		user[num].id = (gint)sqlite3_column_int64(stmt,0);
		g_strlcpy(user[num].name, (gchar*)sqlite3_column_text(stmt,1), sizeof(user[num].name));
		g_strlcpy(user[num].surname, (gchar*)sqlite3_column_text(stmt,2), sizeof(user[num].surname));
		g_strlcpy(user[num].nick, (gchar*)sqlite3_column_text(stmt,3), sizeof(user[num].nick));
		g_strlcpy(user[num].email, (gchar*)sqlite3_column_text(stmt,4), sizeof(user[num].email));
		user[num].age = (gint)sqlite3_column_int64(stmt,5);
		user[num].weight = (gint)sqlite3_column_int64(stmt,6); 
		user[num].size = (gint)sqlite3_column_int64(stmt,7); 
		user[num].gender = (gint)sqlite3_column_int64(stmt,8); 
		user[num].permission = (gint)sqlite3_column_int64(stmt,9);
		rc = sqlite3_step(stmt);
		num++;
	}
	sqlite3_finalize(stmt);
	*users = user;
	return num;
}

