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
#include <glib.h>
#include <string.h>
#include <stdio.h>

int fileExists(const char *filename)
{
	FILE *fp = fopen(filename,"r");
	if(fp)
	{
		fclose(fp);
		return 1;
	}
	else
		return 0;
}

/* the following function is a "copy&paste" from mpd */
int buffer2array(char *buffer, char *array[], const int max)
{
	int i = 0;
	char *c = buffer;

	while (*c != '\0' && i < max) {
		if (*c == '\"') {
			array[i++] = ++c;
			while (*c != '\0') {
				if (*c == '\"') {
					*(c++) = '\0';
					break;
				}
				else if (*(c++) == '\\' && *c != '\0') {
					memmove(c - 1, c, strlen(c) + 1);
				}
			}
		} else {
			c = g_strchug(c);
			if (*c == '\0')
				return i;

			array[i++] = c++;

			while (!g_ascii_isspace(*c) && *c != '\0')
				++c;
		}
		if (*c == '\0')
			return i;
		*(c++) = '\0';

		c = g_strchug(c);
	}
	return i;
}
