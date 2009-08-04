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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <poll.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "network.h"
#include "configfile.h"
#include "commands.h"

static GList *clients;
static guint num_clients;

static gboolean listen_in_event
(GIOChannel *source, GIOCondition condition, gpointer server);

static gboolean client_in_event
(GIOChannel *source, GIOCondition condition, gpointer server);


static gboolean client_in_event
(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    struct client *client = user_data;
    GIOStatus status;
    gint ret;
    gchar *line;
    gsize terminator_pos;
    gsize bytes_read;
  
    if(condition != G_IO_IN)
    {
        network_client_disconnect(client);
        return FALSE;
    }
    status = g_io_channel_read_line(source, &line, &bytes_read, &terminator_pos, NULL);
    if(status != G_IO_STATUS_NORMAL)
    {
        network_client_disconnect(client);
    }
    else
    {
        if((ret = commands_process(client,line)) == COMMANDS_OK)
        {
            if(g_io_channel_write_chars(client->channel, CMD_SUCCESSFULL, sizeof(CMD_SUCCESSFULL), NULL, NULL) != G_IO_STATUS_NORMAL)
                network_client_disconnect(client);
            
        }
        else if (ret == COMMANDS_FAIL)
        {
            if(g_io_channel_write_chars(client->channel, CMD_FAIL, sizeof(CMD_FAIL), NULL, NULL) != G_IO_STATUS_NORMAL)
                network_client_disconnect(client);
        }
        else if (ret == COMMANDS_DENIED)
        {
            if(g_io_channel_write_chars(client->channel, CMD_DENIED, sizeof(CMD_DENIED), NULL, NULL) != G_IO_STATUS_NORMAL)
                network_client_disconnect(client);
        }
        else if (ret == COMMANDS_DISCONNECT)
        {
            g_free(line);
            return TRUE;
        }
        struct pollfd pollfd = { client->fd, POLLHUP, POLLHUP };
        if(!poll(&pollfd, 1, 1))
            g_io_channel_flush(client->channel, NULL);
        else
        {
            g_io_channel_shutdown(client->channel, FALSE, NULL);
            g_debug("poll error");
        }
    }
    g_free(line);
    return TRUE;
}

void network_client_disconnect(struct client *client)
{
    clients = g_list_remove(clients, client);
    num_clients--;
    if(client->source_id)
    {
        g_source_remove(client->source_id);
        client->source_id = 0;
    }
    if(client->channel)
    {
        g_io_channel_unref(client->channel);
        client->channel = NULL;
        g_debug("client %d disconnected",client->num);
        g_free(client);
    }
}


static gboolean listen_in_event
(GIOChannel *source, GIOCondition condition, gpointer data)
{
    struct NetworkServer *server = (struct NetworkServer*)data;
    int fd;
    struct sockaddr_storage sa;
    socklen_t sa_length = sizeof(sa);
    int flags;
    struct client *client;

    fd = accept(server->fd, (struct sockaddr*)&sa, &sa_length);
    if(fd >= 0)
    {
        /* set nonblocking. copy&paste from mpd */
        while ((flags = fcntl(fd, F_GETFL)) < 0 && errno == EINTR);
        flags |= O_NONBLOCK;
        while ((fcntl(fd, F_SETFL, flags)) < 0 && errno == EINTR);
        
        if(num_clients >= config.max_clients)
        {
            close(fd);
            return TRUE;
        }
        g_debug("client %d connected",num_clients);    
        client = g_new0(struct client, 1);
        clients = g_list_prepend(clients, client);
        client->num = num_clients;
        num_clients++;
        client->database = server->database;
        client->fd = fd;
        client->permission = 0;
        client->channel = g_io_channel_unix_new(fd);
        g_sprintf(client->random_number,"%d",g_random_int());
        g_io_channel_set_close_on_unref(client->channel, TRUE);
        g_io_channel_set_encoding(client->channel, NULL, NULL);
//        g_io_channel_set_buffered(client->channel, FALSE);
        write(fd, GREETING, sizeof(GREETING)-1);
        client->source_id = g_io_add_watch(client->channel, G_IO_IN|G_IO_ERR|G_IO_HUP,
                            client_in_event, client);
    }
    return TRUE;
}

        
struct NetworkServer *network_server_new(struct TagDatabase *database)
{
    struct sockaddr_in serveraddr;
    int y = 1;
    struct NetworkServer *server;
    GIOChannel *channel;

    server = g_new0(struct NetworkServer, 1);

    server->database = database;
    server->fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( server->fd < 0 )
    {
        g_fprintf(stderr,"Could not create socket\n");
        g_free(server);
        return NULL;
    }
    
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(config.server_port);

    setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));

    if(bind(server->fd,(struct sockaddr*)&serveraddr, sizeof( serveraddr)) < 0)
    {
        g_fprintf(stderr,"Could not start tcp server\n");
        g_free(server);
        return NULL;
    }

    if(listen(server->fd, 5) == -1)
    {
        g_fprintf(stderr,"Could not create listener\n");
        g_free(server);
        return NULL;
    }
    
    channel = g_io_channel_unix_new(server->fd);
    server->listen_watch = g_io_add_watch(channel, G_IO_IN, listen_in_event, server);
    g_io_channel_unref(channel);

    return server;
}

gboolean network_client_printf(struct client *client, char *format, ...)
{
    va_list args, tmp;
    int length;
    gchar *buffer;

    va_start(args, format);
    va_copy(tmp, args);
    length = vsnprintf(NULL, 0, format, tmp);
    va_end(tmp);

    if(length <= 0)
        return FALSE;

    buffer = g_malloc(length + 1);
    vsnprintf(buffer, length + 1, format, args);
    if(g_io_channel_write_chars(client->channel, buffer, length, NULL, NULL) != G_IO_STATUS_NORMAL)
    {
        network_client_disconnect(client);
        g_free(buffer);
        va_end(args);
        return FALSE;
    }
    g_free(buffer);
    va_end(args);
    return TRUE;
}
