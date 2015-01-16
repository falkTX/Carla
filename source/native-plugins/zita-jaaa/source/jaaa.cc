// -------------------------------------------------------------------------
//
//  Copyright (C) 2004-2013 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// -------------------------------------------------------------------------


#include <stdlib.h>
#include <stdio.h>
#include <clthreads.h>
#include <clxclient.h>
#include <sys/mman.h>
#include "messages.h"
#include "spectwin.h"
#include "audio.h"


#define NOPTS 10
#define CP (char *)


XrmOptionDescRec options [NOPTS] =
{
    {CP"-h",   CP".help",      XrmoptionNoArg,  CP"true"  },
    {CP"-J",   CP".jack",      XrmoptionNoArg,  CP"true"  },
    {CP"-s",   CP".server",    XrmoptionSepArg,  0        },
    {CP"-A",   CP".alsa",      XrmoptionNoArg,  CP"true"  },
    {CP"-d",   CP".device",    XrmoptionSepArg,  0        },
    {CP"-C",   CP".capture",   XrmoptionSepArg,  0        },
    {CP"-P",   CP".playback",  XrmoptionSepArg,  0        },
    {CP"-r",   CP".fsamp",     XrmoptionSepArg,  0        },
    {CP"-p",   CP".period",    XrmoptionSepArg,  0        },
    {CP"-n",   CP".nfrags",    XrmoptionSepArg,  0        }
};


static void help (void)
{
    fprintf (stderr, "\nJaaa-%s\n\n", VERSION);
    fprintf (stderr, "  (C) 2004-2010 Fons Adriaensen  <fons@kokkinizita.net>\n\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -h                 Display this text\n");
    fprintf (stderr, "  -name <name>       Jack and X11 name\n");
    fprintf (stderr, "  -J                 Use JACK, with options:\n");
    fprintf (stderr, "    -s <server>        Select Jack server\n");
    fprintf (stderr, "  -A                 Use ALSA, with options:\n");
    fprintf (stderr, "    -d <device>        Alsa device [hw:0]\n");
    fprintf (stderr, "    -C <device>        Capture device\n");
    fprintf (stderr, "    -P <device>        Playback device\n");
    fprintf (stderr, "    -r <rate>          Sample frequency [48000]\n");
    fprintf (stderr, "    -p <period>        Period size [1024]\n");
    fprintf (stderr, "    -n <nfrags>        Number of fragments [2]\n\n");
    fprintf (stderr, "  Either -J or -A is required.\n\n");
    exit (1);
}


int main (int argc, char *argv [])
{
    X_resman       xrm;
    X_display     *display;
    X_handler     *xhandler;
    X_rootwin     *rootwin;
    Spectwin      *mainwin;
    Audio         *driver;
    ITC_ctrl       itcc;

    // Initialse resource database
    xrm.init (&argc, argv, CP"jaaa", options, NOPTS);
    if (xrm.getb (".help", 0)) help ();
            
    // Open display
    display = new X_display (xrm.get (".display", 0));
    if (display->dpy () == 0)
    {
	fprintf (stderr, "Can't open display !\n");
        delete display;
	exit (1);
    }
    // Open audio interface
    driver = new Audio (&itcc, xrm.rname ());

    if (xrm.getb (".jack", 0))
    {
        driver->init_jack (xrm.get (".server", 0));
    }
    else if (xrm.getb (".alsa", 0))
    {
	const char *p, *adev, *capt, *play;
        int  fsamp, period, nfrags;

        p = xrm.get (".fsamp", 0);
        if (! p || sscanf (p, "%d", &fsamp) != 1) fsamp = 48000;
        p = xrm.get (".period", 0);
        if (! p || sscanf (p, "%d", &period) != 1) period = 1024;
        p = xrm.get (".nfrags", 0);
        if (! p || sscanf (p, "%d", &nfrags) != 1) nfrags = 2;
        adev = xrm.get (".device", "hw:0");
        capt = xrm.get (".capture", 0);
        play = xrm.get (".playback", 0);
	if (!capt && !play) capt = play = adev;
        driver->init_alsa (play, capt, fsamp, period, nfrags, 4, 4);
    }
    else help ();

    // Initialise resources and create windows
    init_styles (display, &xrm);
    rootwin = new X_rootwin (display);
    mainwin = new Spectwin (rootwin, &xrm, driver);

    // Create X handler
    xhandler = new X_handler (display, &itcc, EV_X11);
    xhandler->next_event ();
    XFlush (display->dpy ());

    // Try to lock memory
    if (mlockall (MCL_CURRENT | MCL_FUTURE))
    {
        fprintf (stderr, "Warning: memory lock failed.\n");
    }

    // Enter main loop
    while (mainwin->running ())
    {
	switch (itcc.get_event ())
	{
        case EV_TRIG:
            mainwin->handle_trig ();
	    rootwin->handle_event ();
            XFlush (display->dpy ());
            break;

        case EV_JACK:
            mainwin->handle_term ();
            break;

        case EV_MESG:
            mainwin->handle_mesg (itcc.get_message ());
	    rootwin->handle_event ();
            XFlush (display->dpy ());
            break;

	case EV_X11:
	    rootwin->handle_event ();
	    xhandler->next_event ();
            break;
	}
    }

    // Cleanup
    delete xhandler;
    delete driver;
    delete mainwin; 
    delete rootwin;
    delete display;

    return 0;
}



