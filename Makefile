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

LSDIR_OBJECTS = src/dirstats.o src/utils.o
CC = gcc
CFLAGS = -g
CPPFLAGS = -DUSE_COLORS

dirstats: $(LSDIR_OBJECTS)	
	$(CC) -g $(LSDIR_OBJECTS) -o dirstats

clean:
	rm -fr *.exe src/*.o
