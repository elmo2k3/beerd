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
#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <glib.h>
#include "tag_database.h"

#define GREETING "Welcome to the Beernary Daemon 0.001\r\nType commands to get a list of commands\r\n\r\n"
#define CMD_SUCCESSFULL "OK\r\n"
#define CMD_FAIL "FAIL (drink more beer)\r\n"

#define MAX_CMD_LENGTH 1024

struct NetworkServer
{
	int fd;
	guint listen_watch;
	struct TagDatabase *database;
};

struct client
{
	guint num;
	GIOChannel *channel;
	guint source_id;
	gchar buf[MAX_CMD_LENGTH];
	gint buf_position;
	struct TagDatabase *database;
};

extern struct NetworkServer *network_server_new(struct TagDatabase *database);
extern gboolean network_client_printf(struct client *client, char *format, ...);
extern void network_client_disconnect(struct client *client);

#endif

