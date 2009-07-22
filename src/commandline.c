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
#include "commandline.h"


gint command_line_parse
(struct CmdOptions *options, int argc, char* argv[])
{
    GOptionContext *context;
    gboolean option_no_tagreader;
    gint ret;
    const GOptionEntry entries[] = {
        {"disable-tagreader", 0, 0, G_OPTION_ARG_NONE, &option_no_tagreader,
         NULL, NULL},
         {NULL}
    };

    // default values
    option_no_tagreader = FALSE;
    
    context = g_option_context_new(" - the Beernary Daemon");
    g_option_context_add_main_entries(context, entries, NULL);
    ret = g_option_context_parse(context, &argc, &argv, NULL);

    options->disable_tagreader = option_no_tagreader;

    return 1;
}


