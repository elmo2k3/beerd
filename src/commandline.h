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

/*!
 * \file	commandline.h
 * \brief	parser for the command line options
 * \author	Bjoern Biesenbach <bjoern at bjoern-b dot de>
*/

#ifndef __COMMANDLINE_H__
#define __COMMANDLINE_H__

/*!
 * struct holding options parsed from the commandline
 */
struct CmdOptions
{
    gboolean disable_tagreader; /**< tagreader disabled */
};

/**
 * \param options is a #CmdOptions
 * \param argc number of arguments
 * \param argv arguments to parse
 * \return 0 on failure, 1 on success
 */
extern gint command_line_parse
(struct CmdOptions *options, int argc, char* argv[]);

#endif

