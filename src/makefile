LDFLAGS = `pkg-config --libs glib-2.0 sqlite3`
CFLAGS = `pkg-config --cflags glib-2.0 sqlite3` -g -Wall

.PHONY: clean

beerd: beerd.o configfile.o rfid_tag_reader.o tag_database.o misc.o network.o commands.o

beerd.o: beerd.c beerd.h
configfile.o: configfile.c configfile.h
rfid_tag_reader.o: rfid_tag_reader.c rfid_tag_reader.h
tag_database.o: tag_database.c tag_database.h
misc.o: misc.c misc.h
network.o: network.c network.h
commands.o: commands.c commands.h

clean:
	$(RM) *.o beerd
