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
#include "configfile.h"

#define CREATE_TABLE_TAGS_QUERY "CREATE TABLE tags ( tag TEXT, user_id INTEGER,permission INTEGER)"
#define CREATE_TABLE_USERS_QUERY "CREATE TABLE users ( name TEXT, surname TEXT, nick TEXT, email TEXT, age INTEGER, weight INTEGER, size INTEGER, gender INTEGER, permission INTEGER, password TEXT, pic BLOB, user_id INTEGER PRIMARY KEY)"
#define CREATE_TABLE_ACTIONS_QUERY "CREATE TABLE actions ( timestamp INTEGER, action_id INTEGER, action_value1 TEXT, action_value2 TEXT, action_value3 TEXT)"
#define CREATE_TABLE_LITERS_12H_QUERY "CREATE TABLE liters_12h ( user_nick TEXT, liters INTEGER)"
#define CREATE_TABLE_LITERS_OVERALL_QUERY "CREATE TABLE liters_overall ( user_nick TEXT, liters INTEGER)"
#define CREATE_ADMIN_USER "INSERT INTO users (nick, password, permission) VALUES ('admin','600982cf9c0c41e12df616d2a9a72d675345ced7',2)"

#define SELECT_TAG_QUERY "SELECT * FROM tags WHERE tag=?"
#define SELECT_USER_QUERY "SELECT * FROM users WHERE rowid=?"
#define SELECT_USER_BY_NICK "SELECT password, permission FROM users WHERE nick=?"
#define SELECT_ALL_USER_QUERY "SELECT rowid,* FROM users"
#define SELECT_ALL_TAGS_QUERY "SELECT rowid,* FROM tags"
#define SELECT_ALL_ACTIONS_QUERY "SELECT rowid,* FROM actions"
#define SELECT_USER_NUM "SELECT count(*) FROM users"
#define SELECT_TAG_NUM "SELECT count(*) FROM tags"
#define SELECT_ACTION_NUM "SELECT count(*) FROM actions"
#define SELECT_ACTIONS_LAST_DRAW "SELECT action_value1 FROM actions WHERE action_id=1 ORDER BY timestamp desc LIMIT 1"
#define SELECT_ALL_DRAWINGS "SELECT timestamp, action_value1 FROM actions WHERE action_id=0 AND timestamp >=?"
#define SELECT_LITERS_BY_TIME "SELECT sum(action_value3) FROM actions WHERE timestamp >? AND timestamp <?"

#define SELECT_ACTION_LAST_READ "SELECT timestamp,action_value1 FROM actions WHERE action_id=? order by timestamp desc LIMIT 1"

#define INSERT_ACTION_QUERY "INSERT INTO actions (timestamp, action_id, action_value1, action_value2, action_value3) VALUES (?,?,?,?,?)"
#define INSERT_USER_QUERY "INSERT INTO users (name,surname,nick,email,age,weight,size,gender,permission,password) VALUES (?,?,?,?,?,?,?,?,?,?)"
#define UPDATE_USER_QUERY "UPDATE users SET name=?, surname=?, nick=?, email=?, age=?, weight=?, size=?, gender=?, permission=?, password=? WHERE rowid=?"
#define UPDATE_USER_QUERY_WO_PW "UPDATE users SET name=?, surname=?, nick=?, email=?, age=?, weight=?, size=?, gender=?, permission=? WHERE rowid=?"
#define INSERT_TAG_QUERY "INSERT INTO tags (tag,user_id,permission) VALUES (?,?,?)"

static int createDatabaseLayout(struct TagDatabase *database);

extern struct TagDatabase *tag_database_new(char *filename)
{
    struct TagDatabase *database = g_new0(struct TagDatabase, 1);

	if(config.use_sqlite)
	{
		database->sqlite_usable = 1;
		if(fileExists(filename))
		{
			int rc;
			rc = sqlite3_open(filename, &database->db);
			if(rc != SQLITE_OK)
			{
				fprintf(stderr,"could not open database: %s", filename);
				sqlite3_close(database->db);
				database->sqlite_usable = 0;
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
				database->sqlite_usable = 0;
			}
			if(createDatabaseLayout(database))
			{
				sqlite3_close(database->db);
				database->sqlite_usable = 0;
			}
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
	rc = sqlite3_exec(database->db, CREATE_ADMIN_USER,  0, 0, &zErrMsg);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"sqlerror: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return -1;
	}
	rc = sqlite3_exec(database->db, CREATE_TABLE_LITERS_12H_QUERY,  0, 0, &zErrMsg);
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

	rc = sqlite3_prepare_v2(database->db, SELECT_TAG_QUERY, -1, &stmt, NULL);
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

	rc = sqlite3_prepare_v2(database->db, SELECT_TAG_QUERY, -1, &stmt, NULL);
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

	rc = sqlite3_prepare_v2(database->db, SELECT_USER_QUERY, -1, &stmt, NULL);
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

	rc = sqlite3_prepare_v2(database->db, SELECT_USER_QUERY, -1, &stmt, NULL);
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
	int differential_beer = 0;
	
	if(action_id == ACTION_BEER_DRAWN)
	{
		g_debug("selecting last draw action");	
		rc = sqlite3_prepare_v2(database->db, SELECT_ACTIONS_LAST_DRAW, -1, &stmt, NULL);
		if(rc != SQLITE_OK)
		{
			g_debug(database->error_string,"sql error SELECT_ACTIONS_LAST_DRAW\n");
			return 0;
		}
		rc = sqlite3_step(stmt);
		if(rc == SQLITE_ROW)
		{
			g_debug("actual: %d old: %d", atoi(value1), (int)sqlite3_column_int64(stmt, 0));
			differential_beer = atoi(value1) - (int)sqlite3_column_int64(stmt,0);
			sqlite3_finalize(stmt);
			g_debug("differential beer = %d",differential_beer);
		}
		else
		{
			sqlite3_finalize(stmt);
		}
	}

	
	rc = sqlite3_prepare_v2(database->db, INSERT_ACTION_QUERY, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error INSERT_ACTION_QUERY\n");
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, (sqlite3_int64)timestamp);
	sqlite3_bind_int64(stmt, 2, (sqlite3_int64)action_id);
	sqlite3_bind_text(stmt, 3, value1, -1, NULL);
	sqlite3_bind_text(stmt, 4, value2, -1, NULL);
	if(action_id == ACTION_BEER_DRAWN)
	{
		char buf[128];
		if(differential_beer <0)
			differential_beer = 0;
		sprintf(buf,"%d",differential_beer);
		sqlite3_bind_text(stmt, 5, buf, -1, NULL);
	}
	else
	{
		sqlite3_bind_text(stmt, 5, "", -1, NULL);
	}

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
	
	rc = sqlite3_prepare_v2(database->db, SELECT_ACTION_LAST_READ, -1, &stmt, NULL);
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

static gint tag_database_nick_exists
(struct TagDatabase *database, gchar *nick)
{
	int rc;
	sqlite3_stmt *stmt;
	
	rc = sqlite3_prepare_v2(database->db, SELECT_USER_BY_NICK, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_USER_BY_NICK\n");
		return 0;
	}
	sqlite3_bind_text(stmt, 1, nick, -1, NULL);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if(rc == SQLITE_ROW)
		return 1;
	return 0;
}


gint tag_database_user_insert
(struct TagDatabase *database, struct TagUser *user)
{
	int rc;
	sqlite3_stmt *stmt;

	if(tag_database_nick_exists(database, user->nick))
		return 0;

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

gint tag_database_user_update
(struct TagDatabase *database, struct TagUser *user)
{
	int rc;
	sqlite3_stmt *stmt;
	
//	if(tag_database_nick_exists(database, user->nick))
//		return 0;
	
	if(strlen(user->password))
		rc = sqlite3_prepare_v2(database->db, UPDATE_USER_QUERY, -1, &stmt, NULL);
	else
		rc = sqlite3_prepare_v2(database->db, UPDATE_USER_QUERY_WO_PW, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error UPDATE_USER_QUERY\n");
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
	if(strlen(user->password))
	{
		sqlite3_bind_text(stmt, 10, user->password, -1, NULL);
		sqlite3_bind_int64(stmt, 11, (sqlite3_int64)user->id);
	}
	else
		sqlite3_bind_int64(stmt, 10, (sqlite3_int64)user->id);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	
	return 1;
}

gint tag_database_tag_insert
(struct TagDatabase *database, gchar *tagid, gint userid, gint permission)
{
	int rc;
	sqlite3_stmt *stmt;

	if(tag_database_tag_exists(database, tagid))
		return 0;

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
(struct TagDatabase *database, gchar *nick, gchar *password, gchar *auth_string)
{
	int rc;
	sqlite3_stmt *stmt;
	gint permission;
	gchar buffer[512];
	
	rc = sqlite3_prepare_v2(database->db, SELECT_USER_BY_NICK, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_USER_BY_NICK\n");
		return 0;
	}
	sqlite3_bind_text(stmt, 1, nick, -1, NULL);
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		GChecksum *checksum = g_checksum_new(G_CHECKSUM_SHA1);
		
		g_sprintf(buffer,"%s%s",sqlite3_column_text(stmt,0), auth_string);
		g_checksum_update(checksum, (guchar*)buffer, strlen(buffer));
		if(g_strcmp0(g_checksum_get_string(checksum), password) ||
			!g_strcmp0((const char*)sqlite3_column_text(stmt,0),"da39a3ee5e6b4b0d3255bfef95601890afd80709"))
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

gint tag_database_tags_get_all(struct TagDatabase *database, struct Tag **tags)
{
	int rc;
	sqlite3_stmt *stmt;
	gint num = 0;
	struct Tag *tag;

	rc = sqlite3_prepare_v2(database->db, SELECT_TAG_NUM, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_TAG_NUM\n");
		return 0;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		num = (gint)sqlite3_column_int64(stmt,0);
		if(num)
		{
			g_debug("allocating %d structs",num);
			tag = g_new0(struct Tag, num);
		}
	}
	sqlite3_finalize(stmt);
	if(!num)
		return 0;

	rc = sqlite3_prepare_v2(database->db, SELECT_ALL_TAGS_QUERY, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_ALL_TAGS_QUERY\n");
		return 0;
	}
	rc = sqlite3_step(stmt);
	num = 0;
	while(rc == SQLITE_ROW)
	{
		tag[num].rowid = (gint)sqlite3_column_int64(stmt,0);
		g_strlcpy(tag[num].tagid, (gchar*)sqlite3_column_text(stmt,1), sizeof(tag[num].tagid));
		tag[num].userid = (gint)sqlite3_column_int64(stmt,2);
		tag[num].permission = (gint)sqlite3_column_int64(stmt,3);
		rc = sqlite3_step(stmt);
		num++;
	}
	sqlite3_finalize(stmt);
	*tags = tag;
	return num;
}

extern gint tag_database_actions_get_all(struct TagDatabase *database, struct TagAction **actions)
{
	int rc;
	sqlite3_stmt *stmt;
	gint num = 0;
	struct TagAction *action;

	rc = sqlite3_prepare_v2(database->db, SELECT_ACTION_NUM, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_ACTION_NUM\n");
		return 0;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		num = (gint)sqlite3_column_int64(stmt,0);
		if(num)
		{
			g_debug("allocating %d structs",num);
			action = g_new0(struct TagAction, num);
		}
	}
	sqlite3_finalize(stmt);
	if(!num)
		return 0;

	rc = sqlite3_prepare_v2(database->db, SELECT_ALL_ACTIONS_QUERY, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_ALL_ACTION_QUERY\n");
		return 0;
	}
	rc = sqlite3_step(stmt);
	num = 0;
	while(rc == SQLITE_ROW)
	{
		action[num].rowid = (gint)sqlite3_column_int64(stmt,0);
		action[num].timestamp = (time_t)sqlite3_column_int64(stmt,1);
		action[num].action_id = (gint)sqlite3_column_int64(stmt,2);
		if(sqlite3_column_text(stmt,3))
		{
			g_strlcpy(action[num].action_value1, 
				(gchar*)sqlite3_column_text(stmt,3), sizeof(action[num].action_value1));
		}
		if(sqlite3_column_text(stmt,4))
		{
			g_strlcpy(action[num].action_value2, 
				(gchar*)sqlite3_column_text(stmt,4), sizeof(action[num].action_value2));
		}
		rc = sqlite3_step(stmt);
		num++;
	}
	sqlite3_finalize(stmt);
	*actions = action;
	return num;
}

gint tag_database_get_liters_per_tag
(struct TagDatabase *database, struct TagLiters **liters, time_t timestamp)
{
	struct _drawing
	{
		time_t timestamp;
		gchar tagid[20];
	}*p_drawing, *p_drawing_last;
	
	int rc;
	sqlite3_stmt *stmt;
	GList *drawing_list = NULL;
	p_drawing = NULL;
	p_drawing_last = NULL;
	int num_tags;
	struct TagLiters *liter;
	int p;
	
	rc = sqlite3_prepare_v2(database->db, SELECT_TAG_NUM, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_TAG_NUM\n");
		return 0;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		num_tags = (gint)sqlite3_column_int64(stmt,0);
		if(num_tags)
		{
			liter = g_new0(struct TagLiters, num_tags);
		}
		else
			liter = NULL;
	}
	sqlite3_finalize(stmt);

	rc = sqlite3_prepare_v2(database->db, SELECT_ALL_DRAWINGS, -1, &stmt, NULL);
	if(rc != SQLITE_OK)
	{
		g_sprintf(database->error_string,"sql error SELECT_ALL_DRAWINGS\n");
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, timestamp);
	while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		p_drawing = g_new0(struct _drawing, 1);
		p_drawing->timestamp = (gint)sqlite3_column_int64(stmt,0);
		g_strlcpy(p_drawing->tagid, (gchar*)sqlite3_column_text(stmt,1), 
			sizeof(p_drawing->tagid));
		drawing_list = g_list_prepend( drawing_list, p_drawing);
	}
	drawing_list = g_list_reverse(drawing_list);
	sqlite3_finalize(stmt);

	for(p=0; p < g_list_length(drawing_list); p++)
	{
		p_drawing = g_list_nth_data(drawing_list,p);
		if(p_drawing_last)
		{
			sqlite3_prepare_v2(database->db, SELECT_LITERS_BY_TIME, -1, &stmt, NULL);
			sqlite3_bind_int64(stmt, 1, p_drawing_last->timestamp);
			sqlite3_bind_int64(stmt, 2, p_drawing->timestamp);
			if(sqlite3_step(stmt) == SQLITE_ROW)
			{
				int i;
				gint liters;
				liters = (gint)sqlite3_column_int64(stmt,0);
//				g_debug("got %d ml for %s",liters*10, p_drawing_last->tagid);
				for(i=0;i<num_tags;i++)
				{
					if(liter[i].tagid[0])
					{
						if(!g_strcmp0(p_drawing_last->tagid, liter[i].tagid))
						{
							liter[i].liters += liters;
							break;
						}
					}
					else
					{
						g_strlcpy(liter[i].tagid, p_drawing_last->tagid, 
							sizeof(liter[i].tagid));
						liter[i].liters = liters;
						break;
					}
				}
			}
			sqlite3_finalize(stmt);
		}
		p_drawing_last = p_drawing;
	}
	*liters = liter;
	g_list_free(drawing_list);
	return num_tags;
}

gboolean tag_database_update_liters(struct TagDatabase *database)
{
	int rc;
	sqlite3_stmt *stmt;
	
	struct TagLiters *tagliters;
	struct TagUser temp_user;
	int num;
	int i;
	
	rc = sqlite3_prepare_v2(database->db, 
		"DELETE FROM liters_12h", -1, &stmt, NULL);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	rc = sqlite3_prepare_v2(database->db, 
		"DELETE FROM liters_overall", -1, &stmt, NULL);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	num = tag_database_get_liters_per_tag(database, &tagliters, time(NULL)-43200);
	for(i=0;i<num;i++)
	{
		if(tag_database_user_get_by_tag(database, tagliters[i].tagid, &temp_user))
		{
			rc = sqlite3_prepare_v2(database->db, 
				"SELECT liters FROM liters_12h WHERE user_nick=?", -1, &stmt, NULL);
			if(rc != SQLITE_OK)
			{
				g_sprintf(database->error_string,"sql error SELECT_ALL_DRAWINGS\n");
				return 0;
			}
			sqlite3_bind_text(stmt, 1, temp_user.nick, -1, NULL);
			rc = sqlite3_step(stmt);
			if(rc == SQLITE_ROW) // user exists yet
			{
				tagliters[i].liters += sqlite3_column_int64(stmt, 1);
				sqlite3_finalize(stmt);
				rc = sqlite3_prepare_v2(database->db,
					"UPDATE liters_12h SET liters=? WHERE user_nick=?", -1, &stmt, NULL);
				if(rc != SQLITE_OK)
				{
					g_sprintf(database->error_string,"sql error SELECT_ALL_DRAWINGS\n");
					return 0;
				}
				sqlite3_bind_int64(stmt, 1, tagliters[i].liters);
				sqlite3_bind_text(stmt, 2, temp_user.nick, -1, NULL);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
			else
			{
				sqlite3_finalize(stmt);
				rc = sqlite3_prepare_v2(database->db,
					"INSERT INTO liters_12h (user_nick,liters) VALUES(?,?)", -1, &stmt, NULL);
				if(rc != SQLITE_OK)
				{
					g_sprintf(database->error_string,"sql error SELECT_ALL_DRAWINGS\n");
					return 0;
				}
				sqlite3_bind_text(stmt, 1, temp_user.nick, -1, NULL);
				sqlite3_bind_int64(stmt, 2, tagliters[i].liters);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}
	if(num)
		g_free(tagliters);
	
	num = tag_database_get_liters_per_tag(database, &tagliters, 0);
	for(i=0;i<num;i++)
	{
		if(tag_database_user_get_by_tag(database, tagliters[i].tagid, &temp_user))
		{
			rc = sqlite3_prepare_v2(database->db, 
				"SELECT liters FROM liters_overall WHERE user_nick=?", -1, &stmt, NULL);
			if(rc != SQLITE_OK)
			{
				g_sprintf(database->error_string,"sql error SELECT_ALL_DRAWINGS\n");
					g_debug("sql error SELECT_ALL_DRAWINGS\n");
				return 0;
			}
			sqlite3_bind_text(stmt, 1, temp_user.nick, -1, NULL);
			rc = sqlite3_step(stmt);
			if(rc == SQLITE_ROW) // user exists yet
			{
				int old_liters;
				old_liters = (int)sqlite3_column_int64(stmt, 0);
				tagliters[i].liters += old_liters;
				sqlite3_finalize(stmt);
				rc = sqlite3_prepare_v2(database->db,
					"UPDATE liters_overall SET liters=? WHERE user_nick=?", -1, &stmt, NULL);
				if(rc != SQLITE_OK)
				{
					g_sprintf(database->error_string,"sql error SELECT_ALL_DRAWINGS\n");
					g_debug("sql error SELECT_ALL_DRAWINGS\n");
					return 0;
				}
				sqlite3_bind_int64(stmt, 1, tagliters[i].liters);
				sqlite3_bind_text(stmt, 2, temp_user.nick, -1, NULL);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
			else
			{
				sqlite3_finalize(stmt);
				rc = sqlite3_prepare_v2(database->db,
					"INSERT INTO liters_overall (user_nick,liters) VALUES(?,?)", -1, &stmt, NULL);
				if(rc != SQLITE_OK)
				{
					g_sprintf(database->error_string,"sql error SELECT_ALL_DRAWINGS\n");
					g_debug("sql error SELECT_ALL_DRAWINGS\n");
					return 0;
				}
				sqlite3_bind_text(stmt, 1, temp_user.nick, -1, NULL);
				sqlite3_bind_int64(stmt, 2, tagliters[i].liters);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}
	if(num)
		g_free(tagliters);
	return TRUE;
}


