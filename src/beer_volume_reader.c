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
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>

#include "configfile.h"
#include "beer_volume_reader.h"

void beer_volume_reader_set_callback(struct BeerVolumeReader *tag_reader, void *callback, void *user_data)
{
	tag_reader->callback = callback;
	tag_reader->user_data = user_data;
}

static gboolean serialReceive
(GIOChannel *channel, GIOCondition *condition, struct BeerVolumeReader *beer_volume_reader)
{
    gchar buf[2048];
    gsize bytes_read;
    gint i;
    g_io_channel_read_chars(channel, buf, sizeof(buf), &bytes_read, NULL);
    buf[bytes_read] = '\0';

    for(i=0; i < bytes_read; i++)
    {
        if(buf[i] > 47 && buf[i] < 91) // we want to parse hex data only
        {
			beer_volume_reader->buf[beer_volume_reader->buf_position++] = buf[i];
        }
		if(buf[i] == '\n')
		{
			char *divider;
			divider = strtok(beer_volume_reader->buf, ";");
			if(divider)
				beer_volume_reader->last_barrel = atoi (divider);
			divider = strtok(NULL, beer_volume_reader->buf);
			if(divider)
				beer_volume_reader->last_overall = atoi (divider);
			beer_volume_reader->callback(beer_volume_reader, 
				beer_volume_reader->user_data);
			beer_volume_reader->buf_position = 0;
		}
    }
    return TRUE;
}

struct BeerVolumeReader *beer_volume_reader_new(char *serial_device)
{
    int fd;
	struct termios newtio;
	/* open the device */
	fd = open(serial_device, O_RDONLY | O_NOCTTY | O_NDELAY );
	if (fd <0) 
    {
		return NULL;
	}

	memset(&newtio, 0, sizeof(newtio)); /* clear struct for new port settings */
	newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = (ICANON);
	tcflush(fd, TCIFLUSH);
	if(tcsetattr(fd,TCSANOW,&newtio) < 0)
	{
//		return NULL;
    }

	struct BeerVolumeReader *beer_volume_reader_to_return = g_new0(struct BeerVolumeReader, 1);

	beer_volume_reader_to_return->buf_position = 0;
    GIOChannel *serial_device_chan = g_io_channel_unix_new(fd);
    g_io_add_watch(serial_device_chan, G_IO_IN, 
		(GIOFunc)serialReceive, beer_volume_reader_to_return);
	g_io_add_watch(serial_device_chan, G_IO_ERR, (GIOFunc)exit, NULL);
	beer_volume_reader_to_return->channel = serial_device_chan;
    g_io_channel_unref(serial_device_chan);

	return beer_volume_reader_to_return;
}

void beer_volume_reader_control_valve(struct BeerVolumeReader *beer_reader, const char open)
{

	g_io_channel_write_chars(beer_reader->channel, &open, sizeof(open), NULL, NULL);
}
