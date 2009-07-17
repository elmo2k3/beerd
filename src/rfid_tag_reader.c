#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "beerd.h"

static gchar tagid[11];
static gint tagposition = 0;

static gboolean serialReceive(GIOChannel *channel)
{
    gchar buf[2048];
    gsize bytes_read;
    gint i;
    g_io_channel_read_chars(channel, buf, sizeof(buf), &bytes_read, NULL);
    buf[bytes_read] = '\0';

    for(i=0; i < bytes_read; i++)
    {
        if(buf[i] > 47 && buf[i] < 91) // we only want to parse hex data
        {
            tagid[tagposition++] = buf[i];
            if(tagposition >= 10)
            {
                tagposition = 0;
                tagid[10] = '\0';
                fprintf(stdout,"tagid =  %s\n",tagid);
            }
        }
    }
    return TRUE;
}

gint rfid_tag_reader_init(char *serial_device)
{
    int fd;
	struct termios newtio;
	/* open the device */
	fd = open(serial_device, O_RDONLY | O_NOCTTY | O_NDELAY );
	if (fd <0) 
    {
		return -1;
	}

	memset(&newtio, 0, sizeof(newtio)); /* clear struct for new port settings */
	newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = (ICANON);
	tcflush(fd, TCIFLUSH);
	if(tcsetattr(fd,TCSANOW,&newtio) < 0)
		return -1;
    
    GIOChannel *serial_device_chan = g_io_channel_unix_new(fd);
    guint serial_watch = g_io_add_watch(serial_device_chan, G_IO_IN, 
		(GIOFunc)serialReceive, GINT_TO_POINTER(fd));
    g_io_channel_unref(serial_device_chan);
	beerd_state.rfid_serial_port_watcher = serial_watch;
	return 0;
}
