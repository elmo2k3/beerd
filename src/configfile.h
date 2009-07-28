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
#ifndef __CONFIGFILE_H__
#define __CONFIGFILE_H__

/** @file configfile.h
 * Config file routines
 */

#define CONFIG_DEFAULT_RFID_SERIAL_PORT "/dev/ttyUSB0"

// how many seconds to wait, before the same tag is recognized again?
#define CONFIG_DEFAULT_RFID_TIMEOUT 5
#define CONFIG_DEFAULT_SQLITE_FILE "beerd.sqlite3"
#define CONFIG_DEFAULT_PORT 5335
#define CONFIG_DEFAULT_MAX_CLIENTS 10
#define CONFIG_DEFAULT_USE_SQLITE 1
#define CONFIG_DEFAULT_USE_MYSQL 0
#define CONFIG_DEFAULT_MYSQL_HOST "localhost"
#define CONFIG_DEFAULT_MYSQL_DB "beer"
#define CONFIG_DEFAULT_MYSQL_USER "beer"
#define CONFIG_DEFAULT_MYSQL_PASSWORD ""


/*!
 * struct holding values parsed from configfile
 */
struct _config
{
	char rfid_serial_port[128]; /**< serial device for rfid tag reader */
	int rfid_timeout;			/**< timeout in seconds for logging the same tag */
	char sqlite_file[128];		/**< location of the sqlite database file */
	int server_port;			/**< port the server will listen on tcp */
	int max_clients;			/**< max num of simultanous clients */
	int use_sqlite;
	int use_mysql;
	char mysql_host[128];
	char mysql_database[128];
	char mysql_user[128];
	char mysql_password[128];
}config;

/**
 * \param conf is a #_config
 * \return 0 on failure, 1 on success
 */
extern int config_load(char *conf);
extern int config_save(char *conf);

/* Default values */

#endif

