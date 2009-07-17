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
#ifndef __RFID_TAG_READER_H__
#define __RFID_TAG_READER_H__

#include <glib.h>

struct RfidTagReader
{
	gchar tagid[11];
	gchar last_tagid[11];
	guint tagposition;
	guint serial_port_watcher;
	gchar error_string[1024];
	gboolean timeout_active;
};

extern struct RfidTagReader *rfid_tag_reader_new(char *serial_device);

#endif
