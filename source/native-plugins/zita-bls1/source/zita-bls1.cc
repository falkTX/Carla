// ----------------------------------------------------------------------
//
//  Copyright (C) 2011 Fons Adriaensen <fons@linuxaudio.org>
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
// ----------------------------------------------------------------------


#include <stdlib.h>
#include <stdio.h>
#include <clthreads.h>
#include <sys/mman.h>
#include <signal.h>
#include "global.h"
#include "styles.h"
#include "jclient.h"
#include "mainwin.h"


#define NOPTS 4
#define CP (char *)


XrmOptionDescRec options [NOPTS] =
{
    { CP"-h", CP".help",     XrmoptionNoArg,   CP"true" },
    { CP"-g", CP".geometry", XrmoptionSepArg,  0        },
    { CP"-s", CP".server",   XrmoptionSepArg,  0        }
};



static Jclient  *jclient = 0;
static Mainwin  *mainwin = 0;


static void help (void)
{
    fprintf (stderr, "\n%s-%s\n\n", PROGNAME, VERSION);
    fprintf (stderr, "  (C) 2011 Fons Adriaensen  <fons@linuxaudio.org>\n\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -h              Display this text\n");
    fprintf (stderr, "  -g <geometry>   Window position\n");
    fprintf (stderr, "  -s <server>     Jack server name\n");
    exit (1);
}


static void sigint_handler (int)
{
    signal (SIGINT, SIG_IGN);
    mainwin->stop ();
}


int main (int ac, char *av [])
{
    X_resman       xresman;
    X_display     *display;
    X_handler     *handler;
    X_rootwin     *rootwin;
    int           ev, xp, yp, xs, ys;

    xresman.init (&ac, av, CP PROGNAME, options, NOPTS);
    if (xresman.getb (".help", 0)) help ();
            
    display = new X_display (xresman.get (".display", 0));
    if (display->dpy () == 0)
    {
	fprintf (stderr, "Can't open display.\n");
        delete display;
	return 1;
    }

    xp = yp = 100;
    xs = Mainwin::XSIZE + 4;
    ys = Mainwin::YSIZE + 30;
    xresman.geometry (".geometry", display->xsize (), display->ysize (), 1, xp, yp, xs, ys);
    if (styles_init (display, &xresman)) 
    {
	delete display;
	return 1;
    }

    jclient = new Jclient (xresman.rname (), xresman.get (".server", 0));
    rootwin = new X_rootwin (display);
    mainwin = new Mainwin (rootwin, &xresman, xp, yp, jclient);
    rootwin->handle_event ();
    handler = new X_handler (display, mainwin, EV_X11);
    handler->next_event ();
    XFlush (display->dpy ());
    ITC_ctrl::connect (jclient, EV_EXIT, mainwin, EV_EXIT);

    if (mlockall (MCL_CURRENT | MCL_FUTURE)) fprintf (stderr, "Warning: memory lock failed.\n");
    signal (SIGINT, sigint_handler); 
    do
    {
	ev = mainwin->process ();
	if (ev == EV_X11)
	{
	    rootwin->handle_event ();
	    handler->next_event ();
	}
    }
    while (ev != EV_EXIT);	

    delete jclient;
    delete handler;
    delete rootwin;
    delete display;
   
    return 0;
}



