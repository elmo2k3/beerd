#include <stdio.h>
#include <glib.h>

#include "configfile.h"
#include "rfid_tag_reader.h"


int main(int argc, char *argv[])
{
	config_load("beerd.conf");
	if(rfid_tag_reader_init(config.rfid_serial_port))
	{
		g_fprintf(stderr,"Could not open serial port for rfid tag reader: %s\n",
			config.rfid_serial_port);
		return -1;
	}
    GMainLoop *loop = g_main_loop_new(NULL,FALSE);
    g_main_loop_run(loop);
    return 0;
}
