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
#ifndef __BEER_VOLUME_READER_H__
#define __BEER_VOLUME_READER_H__

/** @file beer_volume_reader.h
 * Connection to Mic's Beernary Counter Board via RS232
 */

#include <glib.h>
#include "tag_database.h"

/**
 *	struct for one rfid tag reader
 */
struct BeerVolumeReader
{
	char buf[1024];
	GIOChannel *channel;	
	int buf_position;
	int last_barrel;
	int last_overall;
	void (*callback)(void*,void*); /** function to call after got volume */
	void *user_data; /** data to pass to the function that is called */
};

extern struct BeerVolumeReader *beer_volume_reader_new(char *serial_device);
extern void beer_volume_reader_set_callback(struct BeerVolumeReader *beer_reader, void *callback, void *user_data);
extern void beer_volume_reader_control_valve(struct BeerVolumeReader *beer_reader, const char open);

#endif
