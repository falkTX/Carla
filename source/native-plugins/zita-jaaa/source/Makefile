#  --------------------------------------------------------------------------
#
#  Copyright (C) 2004-2013 Fons Adriaensen <fons@linuxaudio.org>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#  --------------------------------------------------------------------------


PREFIX = /usr/local
SUFFIX := $(shell uname -m | sed -e 's/^unknown/$//' -e 's/^i.86/$//' -e 's/^x86_64/$/64/')
LIBDIR = lib$(SUFFIX)
VERSION = 0.8.4
CPPFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -I/usr/include/freetype2
CXXFLAGS += -O2 -Wall -MMD -MP
CXXFLAGS += -march=native
LDFLAGS += -L/usr/X11R6/$(LIBDIR)
LDLIBS += -lzita-alsa-pcmi -lclthreads -lclxclient -lpthread -lfftw3f -ljack -lasound -lpthread -lXft -lX11 -lrt


JAAA_O = jaaa.o styles.o spectwin.o audio.o rngen.o
jaaa:	$(JAAA_O)
	g++ $(LDFLAGS) -o $@ $(JAAA_O) $(LDLIBS)

$(JAAA_O):
-include $(JAAA_O:%.o=%.d)


install:	jaaa
	install -Dm 755 jaaa $(DESTDIR)$(PREFIX)/bin/jaaa

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/jaaa

clean:
	/bin/rm -f *~ *.o *.a *.d *.so jaaa

