#   Copyright (C) 2004 Tom Szilagyi
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
#   $Id: Makefile,v 1.4 2004/02/19 20:30:58 tszilagyi Exp $


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

CC		=	gcc

PLUGINS		=	tap_autopan.so \
			tap_deesser.so \
			tap_eq.so \
			tap_pitch.so \
			tap_reverb.so \
			tap_rotspeak.so \
			tap_limiter.so \
			tap_echo.so \
			tap_tremolo.so \
			tap_vibrato.so

all: $(PLUGINS)

# RULES TO BUILD PLUGINS FROM C CODE

tap_tremolo.so: tap_tremolo.c tap_utils.h ladspa.h
	$(CC) -I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC tap_tremolo.c -o tap_tremolo.o
	$(CC) -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt -o tap_tremolo.so tap_tremolo.o

tap_eq.so: tap_eq.c tap_utils.h ladspa.h
	$(CC) -I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC tap_eq.c -o tap_eq.o
	$(CC) -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt -o tap_eq.so tap_eq.o

tap_echo.so: tap_echo.c tap_utils.h ladspa.h
	$(CC) -I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC tap_echo.c -o tap_echo.o
	$(CC) -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt -o tap_echo.so tap_echo.o

tap_reverb.so: tap_reverb.c tap_reverb.h tap_reverb_presets.h tap_utils.h ladspa.h
	$(CC) -I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC tap_reverb.c -o tap_reverb.o
	$(CC) -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt -o tap_reverb.so tap_reverb.o

tap_limiter.so: tap_limiter.c tap_utils.h ladspa.h
	$(CC) -I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC tap_limiter.c -o tap_limiter.o
	$(CC) -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt -o tap_limiter.so tap_limiter.o

tap_autopan.so: tap_autopan.c tap_utils.h ladspa.h
	$(CC) -I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC tap_autopan.c -o tap_autopan.o
	$(CC) -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt -o tap_autopan.so tap_autopan.o

tap_deesser.so: tap_deesser.c tap_utils.h ladspa.h
	$(CC) -I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC tap_deesser.c -o tap_deesser.o
	$(CC) -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt -o tap_deesser.so tap_deesser.o

tap_vibrato.so: tap_vibrato.c tap_utils.h ladspa.h
	$(CC) -I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC tap_vibrato.c -o tap_vibrato.o
	$(CC) -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt -o tap_vibrato.so tap_vibrato.o

tap_rotspeak.so: tap_rotspeak.c tap_utils.h ladspa.h
	$(CC) -I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC tap_rotspeak.c -o tap_rotspeak.o
	$(CC) -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt -o tap_rotspeak.so tap_rotspeak.o

tap_pitch.so: tap_pitch.c tap_utils.h ladspa.h
	$(CC) -I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC tap_pitch.c -o tap_pitch.o
	$(CC) -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt -o tap_pitch.so tap_pitch.o


# OTHER TARGETS

install: targets
	-mkdir -p		$(INSTALL_PLUGINS_DIR)
	cp *.so 		$(INSTALL_PLUGINS_DIR)
	-mkdir -p		$(INSTALL_LRDF_DIR)
	cp tap-plugins.rdf 	$(INSTALL_LRDF_DIR)

targets:	$(PLUGINS)

always:	

clean:
	-rm -f `find . -name "*.so"`
	-rm -f `find . -name "*.o"`
	-rm -f `find .. -name "*~"`

