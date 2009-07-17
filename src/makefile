LDFLAGS = `pkg-config --libs glib-2.0`
CFLAGS = `pkg-config --cflags glib-2.0` -g

.PHONY: clean

beerd: beerd.o configfile.o rfid_tag_reader.o

beerd.o: beerd.c beerd.h
configfile.o: configfile.c configfile.h
rfid_tag_reader.o: rfid_tag_reader.c rfid_tag_reader.h

clean:
	$(RM) *.o beerd
