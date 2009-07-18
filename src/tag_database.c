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
#include "misc.h"
#include "tag_database.h"

#define CREATE_TABLE_TAGS_QUERY "CREATE TABLE tags ( tag TEXT, user_id INTEGER,permission INTEGER)"
#define CREATE_TABLE_USERS_QUERY "CREATE TABLE users ( name TEXT, surname TEXT, nick TEXT, email TEXT )"
#define CREATE_TABLE_ACTIONS_QUERY "CREATE TABLE actions ( timestamp INTEGER, action_id INTEGER, action_value1 INTEGER, action_value2 INTEGER)"

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

