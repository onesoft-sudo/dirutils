LSDIR_OBJECTS = src/dirstats.o src/utils.o
CC = gcc
CFLAGS = -g
CPPFLAGS = -DUSE_COLORS

dirstats: $(LSDIR_OBJECTS)	
	$(CC) -g $(LSDIR_OBJECTS) -o dirstats

clean:
	rm -fr *.exe src/*.o
