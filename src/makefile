LDFLAGS = `pkg-config --libs glib-2.0 sqlite3` -lmysqlclient
CFLAGS = `pkg-config --cflags glib-2.0 sqlite3` -g -Wall

.PHONY: clean

beerd: beerd.o configfile.o rfid_tag_reader.o tag_database.o misc.o network.o commands.o commandline.o led_routines.o beer_volume_reader.o

beerd.o: beerd.c beerd.h
configfile.o: configfile.c configfile.h
rfid_tag_reader.o: rfid_tag_reader.c rfid_tag_reader.h
tag_database.o: tag_database.c tag_database.h
misc.o: misc.c misc.h
network.o: network.c network.h
commands.o: commands.c commands.h
commandline.o: commandline.c commandline.h
led_routines.o: led_routines.c led_routines.h
beer_volume_reader.o: beer_volume_reader.c beer_volume_reader.h

clean:
	$(RM) *.o beerd
