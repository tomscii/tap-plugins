#   Copyright (C) 2004-2009 Tom Szilagyi
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#   $Id: Makefile,v 1.19 2014/02/14 18:59:14 tszilagyi Exp $


#####################################################################
# PLEASE CHANGE THIS to your preferred installation location!
#
# Change this if you want to install somewhere else. In particular
# you may wish to remove the middle "local/" part of the path.

INSTALL_PLUGINS_DIR	=	/usr/local/lib/ladspa/
INSTALL_LRDF_DIR	=	/usr/local/share/ladspa/rdf/

# NO EDITING below this line is required
# if all you want to do is install and use the plugins.
#####################################################################



# GENERAL

OS := $(shell uname -s)

CC      = gcc
CFLAGS  = -I. -O3 -Wall -fomit-frame-pointer -funroll-loops -ffast-math -c -fPIC -DPIC
ifeq ($(OS),Darwin)
LDFLAGS = -nostartfiles -shared -Wl,-install_name,symbolic -lc -lm
else
LDFLAGS = -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt
endif
MODULES = $(wildcard *.c)

all: $(MODULES:%.c=%.so)

# RULES TO BUILD PLUGINS FROM C CODE

tap_reverb.o: tap_reverb.h tap_reverb_presets.h
tap_dynamics_m.o: tap_dynamics_presets.h
tap_dynamics_st.o: tap_dynamics_presets.h

%.o: %.c tap_utils.h ladspa.h
	$(CC) $(CFLAGS) $< -o $@

%.so: %.o
	$(CC) -o $@ $< $(LDFLAGS)

# OTHER TARGETS

install: all
	-mkdir -p          $(INSTALL_PLUGINS_DIR)
	cp *.so            $(INSTALL_PLUGINS_DIR)
	-mkdir -p          $(INSTALL_LRDF_DIR)
	cp tap-plugins.rdf $(INSTALL_LRDF_DIR)
	cp tap_reverb.rdf  $(INSTALL_LRDF_DIR)

clean:
	-rm -f *.so *.o *~

.PHONY: all install clean
