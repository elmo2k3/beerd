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

#include "configfile.h"
#include "rfid_tag_reader.h"

void tag_read(struct RfidTagReader *tag_reader)
{
	printf("tag read: %s\n",tag_reader);
}

int main(int argc, char *argv[])
{
	struct RfidTagReader *tag_reader;
	config_load("beerd.conf");
	tag_reader = rfid_tag_reader_new(config.rfid_serial_port);
	rfid_tag_reader_set_callback(tag_reader, tag_read);

	if(tag_reader == NULL)
	{
		g_fprintf(stderr,"Error creating rfid_tag_reader: %s\n",
			tag_reader->error_string);
		return -1;
	}
    GMainLoop *loop = g_main_loop_new(NULL,FALSE);
    g_main_loop_run(loop);
    return 0;
}
