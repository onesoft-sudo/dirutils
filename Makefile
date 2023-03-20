# Makefile to build dirutils.
#
# Copyright (C) 2023 OSN Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>. 

DIRSTATS_OBJECTS = src/dirstats.o src/utils.o
DIRWATCH_OBJECTS = src/dirwatch.o src/utils.o 
CC = gcc
CFLAGS = -g
CPPFLAGS = -DUSE_COLORS -DHAVE_SYS_INOTIFY_H -DHAVE_SYS_STAT_H

all: dirwatch dirstats

dirstats: $(DIRSTATS_OBJECTS)	
	$(CC) -g $(DIRSTATS_OBJECTS) -o dirstats

dirwatch: $(DIRWATCH_OBJECTS)	
	$(CC) -g $(DIRWATCH_OBJECTS) -o dirwatch

clean:
	rm -fr *.exe dirstats src/*.o
