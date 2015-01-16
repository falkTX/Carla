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
#include <clxclient.h>
#include <X11/keysym.h>
#include <math.h>
#include "styles.h"
#include "spectwin.h"
#include "messages.h"


const char *Spectwin::_formats [9] = { "%1.0f",  "%2.1f",  "%3.2f",
                                       "%1.0fk", "%2.1fk", "%3.2fk",
                                       "%1.0fM", "%2.1fM", "%3.2fM" };


const char *Spectwin::_notes[12] = { "C", "C#,Db", "D", "D#,Eb", "E", "F",
                                     "F#,Gb", "G", "Ab,G#", "A", "Bb,A#", "B" };



Spectwin::Spectwin (X_window *parent, X_resman *xres, ITC_ctrl *audio) :
    X_window (parent, XPOS, YPOS, XDEF, YDEF, Colors.main_bg), _xs (XDEF), _ys (YDEF), _running (1),
    _audio (audio), _input (-1), _drag (0), _p_ind (-1), _ngx (0), _ngy (0), _fftplan (0), _fftlen (64)
{
    int      x, y;
    X_hints  H;

    _atoms [0] = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    _atoms [1] = XInternAtom (dpy (), "WM_TAKE_FOCUS", True);
    XSetWMProtocols (dpy (), win (), _atoms, 2);
    _wmpro = XInternAtom (dpy (), "WM_PROTOCOLS", True);

    H.position (XPOS, YPOS);
    H.minsize (XMIN, YMIN);
    H.maxsize (XMAX, YMAX);
    H.rname (xres->rname ());
    H.rclas (xres->rclas ());
    H.input (True);
    x_apply (&H); 

    x_add_events (ExposureMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask);

    _plotwin = new X_window (this, LMAR, TMAR, XDEF - LMAR - RMAR, YDEF - TMAR - BMAR, Colors.spect_bg);
    _plotmap = XCreatePixmap (dpy (), _plotwin->win (), XDEF - LMAR - RMAR, YDEF - TMAR - BMAR, disp ()->depth ());
    _plotgct = XCreateGC (dpy (), _plotmap, 0, NULL);
     XSetWindowBackgroundPixmap (dpy (), _plotwin->win (), _plotmap);
    _plotwin->x_map ();     

    x = XDEF - RMAR + 3;
    y = 20;
    _butt [IP1] = new X_tbutton (this, this, &Bst0, x     , y, "1", 0, IP1);
    _butt [IP2] = new X_tbutton (this, this, &Bst0, x + 19, y, "2", 0, IP2);
    _butt [IP3] = new X_tbutton (this, this, &Bst0, x + 38, y, "3", 0, IP3);
    _butt [IP4] = new X_tbutton (this, this, &Bst0, x + 57, y, "4", 0, IP4);
    y += Bst1.size.y + 2;
    _butt [IP5] = new X_tbutton (this, this, &Bst0, x     , y, "5", 0, IP5);
    _butt [IP6] = new X_tbutton (this, this, &Bst0, x + 19, y, "6", 0, IP6);
    _butt [IP7] = new X_tbutton (this, this, &Bst0, x + 38, y, "7", 0, IP7);
    _butt [IP8] = new X_tbutton (this, this, &Bst0, x + 57, y, "8", 0, IP8);
    y += Bst1.size.y + 25;

    Bst1.size.x = RMAR - 6;
    _butt [BANDW] = new X_tbutton (this, this, &Bst1, x, y, "Bandw",   0, BANDW);
    y += Bst1.size.y;  
    _butt [VIDAV] = new X_tbutton (this, this, &Bst1, x, y, "Vid.Av",  0, VIDAV);
    y += Bst1.size.y;
    _butt [PEAKH] = new X_tbutton (this, this, &Bst1, x, y, "Pk Hold", 0, PEAKH);
    y += Bst1.size.y;
    _butt [FREEZ] = new X_tbutton (this, this, &Bst1, x, y, "Freeze",  0, FREEZ);
    y += Bst1.size.y + 25;

    _butt [MCLR]  = new X_tbutton (this, this, &Bst1, x, y, "Clear", 0, MCLR);
    y += Bst1.size.y;
    _butt [MPEAK] = new X_tbutton (this, this, &Bst1, x, y, "Peak",  0, MPEAK);
    y += Bst1.size.y;
    _butt [MNSE]  = new X_tbutton (this, this, &Bst1, x, y, "Noise", 0, MNSE);
    y += Bst1.size.y + 25;

    _butt [FMIN]  = new X_tbutton (this, this, &Bst1, x, y, "Min",  0, FMIN);
    y += Bst1.size.y;
    _butt [FMAX]  = new X_tbutton (this, this, &Bst1, x, y, "Max",  0, FMAX);
    y += Bst1.size.y;
    _butt [FCENT] = new X_tbutton (this, this, &Bst1, x, y, "Cent", 0, FCENT);
    y += Bst1.size.y;
    _butt [FSPAN] = new X_tbutton (this, this, &Bst1, x, y, "Span", 0, FSPAN);
    y += Bst1.size.y + 25;

    _butt [AMAX]  = new X_tbutton (this, this, &Bst1, x, y, "Max", 0, AMAX);
    y += Bst1.size.y;
    _butt [ASPAN] = new X_tbutton (this, this, &Bst1, x, y, "Range", 0, ASPAN);
    y += Bst1.size.y + 25;

    _txt1 = new X_textip (this, this, &Tst1, x, y, RMAR - 6, 18, 16);
    _txt1->x_set_win_gravity (NorthEastGravity);
    _txt1->x_mapraised ();

    Bst0.size.x = 15;
    Bst0.size.y = 15;
    y += 22;
    _butt [BTDN] = new X_ibutton (this, this, &Bst0, x + RMAR - 38, y, disp ()->image1515 (X_display::IMG_LT), BTDN);
    _butt [BTUP] = new X_ibutton (this, this, &Bst0, x + RMAR - 23, y, disp ()->image1515 (X_display::IMG_RT), BTUP);

    Bst0.size.x = 17;
    Bst0.size.y = 17;
    y += 47;
    _butt [OP1] = new X_tbutton (this, this, &Bst0, x     , y, "1", 0, OP1);
    _butt [OP2] = new X_tbutton (this, this, &Bst0, x + 19, y, "2", 0, OP2);
    _butt [OP3] = new X_tbutton (this, this, &Bst0, x + 38, y, "3", 0, OP3);
    _butt [OP4] = new X_tbutton (this, this, &Bst0, x + 57, y, "4", 0, OP4);
    y += 19;
    _butt [OP5] = new X_tbutton (this, this, &Bst0, x     , y, "5", 0, OP5);
    _butt [OP6] = new X_tbutton (this, this, &Bst0, x + 19, y, "6", 0, OP6);
    _butt [OP7] = new X_tbutton (this, this, &Bst0, x + 38, y, "7", 0, OP7);
    _butt [OP8] = new X_tbutton (this, this, &Bst0, x + 57, y, "8", 0, OP8);

    Bst0.size.x = 27;
    Bst0.size.y = 17;
    y += 24;
    _butt [NSACT] = new X_tbutton (this, this, &Bst0, XDEF - 30, y, "On", 0, NSACT);
    y += Bst0.size.y + 4;
    _butt [NSLEV] = new X_tbutton (this, this, &Bst1, x, y, "Level", 0, NSLEV);

    y += Bst0.size.y + 8;
    _butt [SIACT] = new X_tbutton (this, this, &Bst0, XDEF - 30, y, "On", 0, SIACT);
    y += Bst0.size.y + 4;
    _butt [SILEV] = new X_tbutton (this, this, &Bst1, x, y, "Ampl", 0, SILEV);
    y += Bst1.size.y;
    _butt [SFREQ] = new X_tbutton (this, this, &Bst1, x, y, "Freq", 0, SFREQ);

    for (int i = 0; i < NBUTT; i++)
    {
       _butt [i]->x_set_win_gravity (NorthEastGravity);
       _butt [i]->x_map ();
    }

    _ipbuf = (float *) fftwf_malloc (BUF_LEN * sizeof (float));
    _fftbuf = (float *) fftwf_malloc (FFT_MAX * sizeof (float));
    _trbuf = (fftwf_complex *) fftwf_malloc ((FFT_MAX / 2 + 9) * sizeof (fftwf_complex));
    _power = new float [FFT_MAX + 1]; //!!!

    _spect = new Spectdata (disp()->xsize() - LMAR - RMAR);    
    _spect->_npix = XDEF - LMAR - RMAR;
    _spect->_bits = 0;
    _spect->_avcnt = 0;
    _spect->_avmax = 1000;

    set_input (0);

    _g_bits = 0;
    _a_ns = -20;
    _a_si = -20;
    _f_si = 1000.0;
}


Spectwin::~Spectwin (void)
{
    if (_fftplan) fftwf_destroy_plan (_fftplan);
    delete _spect;
    delete[] _power;
    fftwf_free (_trbuf);
    fftwf_free (_fftbuf);
    fftwf_free (_ipbuf);
    XFreePixmap (dpy (), _plotmap);
    XFreeGC (dpy (), _plotgct);
}


void Spectwin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case Expose:
        expose ((XExposeEvent *) E);
        break;  

    case ButtonPress:
        bpress ((XButtonEvent *) E);
        break;  

    case MotionNotify:
        motion ((XPointerMovedEvent *) E);
        break;  

    case ButtonRelease:
        brelse ((XButtonEvent *) E);
        break;  

    case ConfigureNotify:
        resize ((XConfigureEvent *) E);
        break;

    case ClientMessage:
        message ((XClientMessageEvent *) E);
        break;

    }
}


void Spectwin::handle_callb (int k, X_window *W, _XEvent * /*E*/ )
{
    X_tbutton *B;
    int    i;
    char   c;

    switch (k)
    {
    case X_callback::TEXTIP | X_textip::BUT:
	_txt1->enable ();
       break;

    case X_callback::TEXTIP | X_textip::KEY:
	if (_txt1->key () == XK_Return)
	{
	    if (sscanf (_txt1->text (), "%f%c", &_p_val, &c) >= 1)
	    {
                if (c == 'k') _p_val *= 1e3;
                if (c == 'M') _p_val *= 1e6;
                switch (_p_ind)
                {
                case BANDW: set_bw (_p_val); break;
                case FMIN:  set_f0 (_p_val); break;
                case FMAX:  set_f1 (_p_val); break;
                case FCENT: set_fc (_p_val); break;
                case FSPAN: set_fs (_p_val); break;
                case AMAX:  set_a1 (_p_val); break;
                case ASPAN: set_ar (_p_val); break;
                case NSLEV: set_a_ns (_p_val); break;
                case SILEV: set_a_si (_p_val); break;
                case SFREQ: set_f_si (_p_val); break;
		}
            }
	    show_param ();
	}
        break;

    case BUTTON | X_button::PRESS:
	B = (X_tbutton *) W;
        switch (i = B->cbid ())
	{
	case IP1:
	case IP2:
	case IP3:
	case IP4:
	case IP5:
	case IP6:
	case IP7:
	case IP8:
            set_input (i - IP1);
            break;

	case BANDW:
        case MPEAK:
	case MNSE:
	case FMIN:
	case FMAX:
	case FCENT:
	case FSPAN:
        case AMAX:
        case ASPAN:
	case NSLEV:
        case SILEV:
        case SFREQ:
            set_param (i);
            break;

        case VIDAV:
            if (B->stat ())
     	    {
        	B->set_stat (0);
                _spect->_avcnt = 0;
	    }
            else 
     	    {
		B->set_stat (2); 
		_butt [PEAKH]->set_stat (0);
                _spect->_avcnt = 1;
                _spect->_bits &= ~Spectdata::PEAKH;
	    }
            break;

        case PEAKH:
            if (B->stat ())
     	    {
		B->set_stat (0);
                _spect->_bits &= ~Spectdata::PEAKH;
	    }
            else 
     	    {
		B->set_stat (2);
		_butt [VIDAV]->set_stat (0);
                _spect->_bits |=  Spectdata::PEAKH;	
                _spect->_avcnt = 0;
            }
            break;

        case FREEZ:
            if (B->stat ())
     	    {
		B->set_stat (0);
                _spect->_bits &= ~Spectdata::FREEZE;
	    }
            else 
     	    {
		B->set_stat (2);
                _spect->_bits |= Spectdata::FREEZE;
	    }
            break;

        case MCLR:
  	    clr_mark ();
            if (_spect->_bits & Spectdata::FREEZE) update ();
            break;

        case BTUP:
        case BTDN:
            mod_param (i == BTUP);
            break;

	case OP1:
	case OP2:
	case OP3:
	case OP4:
	case OP5:
	case OP6:
	case OP7:
	case OP8:
	case NSACT:
	case SIACT:
            set_output (i - OP1);
            break;

	}
        break;
    }
}


void Spectwin::message (XClientMessageEvent *E)
{
    if (E->message_type == _wmpro)
    {
	if (E->data.l [0] == (long) _atoms [0])
	{
            _running = false;
	}
	else if (E->data.l [0] == (long)_atoms [1])
	{
//	    _txt1->enable ();
	}
    }
}


void Spectwin::expose (XExposeEvent *E)
{
    if (E->count == 0) redraw ();
}


void Spectwin::resize (XConfigureEvent *E)
{
    _xs = E->width;
    _ys = E->height;
    if (_xs > XMAX) _xs = XMAX;
    if (_ys > YMAX) _ys = YMAX;
    _plotwin->x_resize (_xs - LMAR - RMAR, _ys - TMAR - BMAR); 
    XFreePixmap (dpy (), _plotmap);
    XFreeGC (dpy (), _plotgct);
    _plotmap = XCreatePixmap (dpy (), _plotwin->win (), _xs - LMAR - RMAR, _ys - TMAR - BMAR, disp ()->depth ());
    _plotgct = XCreateGC (dpy (), _plotmap, 0, NULL);
    XSetWindowBackgroundPixmap (dpy (), _plotwin->win (), _plotmap);
    _spect->_npix = _xs - LMAR - RMAR;
    _ngx = _ngy = 0;
    plot_clear ();
    redraw ();
}


void Spectwin::redraw (void)
{
    X_draw D (dpy (), win (), dgc (), xft ());

    D.setcolor (XftColors.main_fg);
    D.setfont (XftFonts.button);
    D.clearwin ();     

    D.move (_xs - RMAR + 2, 15);
    D.drawstring ("Input", -1);
    D.move (_xs - RMAR + 2, 75);
    D.drawstring ("Analyser", -1);
    D.move (_xs - RMAR + 2, 168);
    D.drawstring ("Markers", -1);
    D.move (_xs - RMAR + 2, 244);
    D.drawstring ("Frequency", -1);
    D.move (_xs - RMAR + 2, 338);
    D.drawstring ("Amplitude", -1);
    D.move (_xs - RMAR + 2, 465);
    D.drawstring ("Output", -1);
    D.move (_xs - RMAR + 2, 525);
    D.drawstring ("Noise", -1);
    D.move (_xs - RMAR + 2, 572);
    D.drawstring ("Sine", -1);

    plot_fscale ();
    plot_ascale ();
    update ();
}


void Spectwin::update (void)
{
    calc_spect (_spect); 
    plot_clear ();
    if (_spect->_bits & (Spectdata::YP_VAL | Spectdata::YM_VAL))
    {
        plot_spect (_spect);
        plot_grid ();
        plot_annot (_spect);
    }

    _plotwin->x_clear ();
}


void Spectwin::bpress (XButtonEvent *E)
{
    float f;

    if (E->subwindow)
    {
	if      (E->button == Button4) mod_param (true);
	else if (E->button == Button5) mod_param (false);
	else if (_butt [MPEAK]->stat ())
	{
            set_mark (calcfreq (E->x), false);
            if (_spect->_bits & Spectdata::FREEZE) update ();
	}
        else if (_butt [MNSE]->stat ())
	{
            set_mark (calcfreq (E->x), true);
            if (_spect->_bits & Spectdata::FREEZE) update ();
	}
        else
	{
   	    _xd = E->x;
            _yd = E->y;
	    _drag = true;
	    if (E->button == Button1)
	    {
		switch (_p_ind)
		{
		case FMAX:  set_param (FMIN);  break;
		case FSPAN: set_param (FCENT); break;
		case ASPAN: set_param (AMAX);  break;
		}
	    }
	    if (E->button == Button3)
	    {
		switch (_p_ind)
		{
		case FMIN:  set_param (FMAX) ; break;
		case FCENT: set_param (FSPAN); break;
		case AMAX:  set_param (ASPAN); break;
		}
	    }
	}
    }
    else if (E->y > _ys - BMAR)
    {
        f = calcfreq (E->x);
	if (E->button == Button1)
	{
	    if ((f > _f0) && (f < _f1)) set_fc (f);
	}
        else if (E->button == Button3) {}
    }
}


void Spectwin::motion (XPointerMovedEvent *E)
{
    if (! _drag) return;

    switch (_p_ind)
    {
    case FMIN:
    case FMAX:
    case FCENT:
    case FSPAN:
        while (E->x - _xd >  15) { mod_param (true);  _xd += 15; }
        while (E->x - _xd < -15) { mod_param (false); _xd -= 15; }
        break;

    case AMAX:
    case ASPAN:
        while (E->y - _yd >  15) { mod_param (false); _yd += 15; }
        while (E->y - _yd < -15) { mod_param (true);  _yd -= 15; }
        break;
    }
}


void Spectwin::brelse (XButtonEvent * /*E*/)
{
    _drag = false;
}


void Spectwin::set_fsamp (float fsamp, bool /*symm*/)
{
    _fsamp = fsamp;
    _fmin = 0;
    _fmax = 0.5 * fsamp;
    _spect->_f0 = _f0 = _fmin;
    _spect->_f1 = _f1 = _fmax;
    _fc = 0.5 * (_fmin + _fmax);
    _fm = 0.001 * fsamp;

    _bmin = 2 * fsamp / FFT_MAX;
    _bmax = 2 * fsamp / FFT_MIN;
    set_bw (_bmax / 8);

    _amin = -200;
    _amax =   10;
    _a0 = -100;
    _a1 =    0;

    set_param (BANDW);
    _ngx = 0;
    _ngy = 0;
    x_map ();
    XFlush (dpy ());
}


void Spectwin::set_input (int i)
{
    if (_input >= 0) _butt [_input + IP1]->set_stat (0);
    _butt [_input = i]->set_stat (2);
    _spect->_bits |= Spectdata::RESET;
    _audio->put_event (EV_MESG, new M_input (_input));
}


void Spectwin::set_output (int i)
{
    int on;

    on = _butt [i + OP1]->stat () ^ 2;
    if (on) _g_bits |=  (1 << i);
    else    _g_bits &= ~(1 << i);
    _butt [i + OP1]->set_stat (on);
    send_genp ();
}


void Spectwin::set_param (int i)
{
    if (_p_ind >= 0) _butt [_p_ind]->set_stat (0);
    _butt [_p_ind = i]->set_stat (1);
    _txt1->set_text ("");
    switch (i)
    {
    case BANDW: _p_val = _bw; break;
    case FMIN:  _p_val = _f0; break;
    case FMAX:  _p_val = _f1; break;
    case FCENT: _p_val = _fc; break;
    case FSPAN: _p_val = _f1 - _f0; break;
    case AMAX:  _p_val = _a1; break;    
    case ASPAN: _p_val = _a1 - _a0; break;    
    case NSLEV: _p_val = _a_ns; break;
    case SILEV: _p_val = _a_si; break;    
    case SFREQ: _p_val = _f_si; break;    
    default:    _p_val = 0;
    }
    show_param ();
}


void Spectwin::mod_param (bool inc)
{
    float df = inc ? _df : -_df;
    float da = inc ? _da : -_da;

    switch (_p_ind)
    {
    case SILEV:
	if (inc) set_a_si (_a_si + 0.1f);
	else     set_a_si (_a_si - 0.1f);
        break;
    case BANDW: set_bw ((inc ? 2.0 : 0.5) * _bw); break;
    case FMIN:  set_f0 (_f0 + df); break;
    case FMAX:  set_f1 (_f1 + df); break;
    case FCENT: set_fc (_fc + df); break;
    case FSPAN: set_fs (_f1 - _f0 + 2 * df); break;
    case AMAX:  set_a1 (_a1 + da); break;
    case ASPAN: set_ar (_a1 - _a0 + da); break;
    }
    show_param ();
}


void Spectwin::set_bw (float bw)
{
    if (bw > _bmax) bw = _bmax;
    if (bw < _bmin) bw = _bmin;
    _spect->_bw = _p_val = _bw = bw;
    _spect->_bits &= ~(Spectdata::YP_VAL | Spectdata::YM_VAL);
    alloc_fft (_spect);
}


void Spectwin::set_f0 (float f)
{
    if (f > _f1 - _fm) f = _f1 - _fm;
    else if (f < _fmin) f = _fmin;
    if (f != _f0)
    {
        _spect->_f0 = _p_val = _f0 = f;
        _fc = 0.5 * (_f0 + _f1);
        _ngx = 0;
	redraw ();
    }
}


void Spectwin::set_f1 (float f)
{
    if (f < _f0 + _fm) f = _f0 + _fm;
    else if (f > _fmax) f = _fmax;
    if (f != _f1)
    {
        _spect->_f1 = _p_val = _f1 = f;
        _fc = 0.5 * (_f0 + _f1);
        _ngx = 0;
	redraw ();
    }
}


void Spectwin::set_fc (float f)
{
    float d = _fc - _f0;
    _f0 += f - _fc;
    _f1 += f - _fc;
    if (_f0 < _fmin) { _f0 = _fmin; _f1 = _f0 + 2 * d; }
    if (_f1 > _fmax) { _f1 = _fmax; _f0 = _f1 - 2 * d; }
    _p_val = _fc = 0.5 * (_f0 + _f1);
    _spect->_f0 = _f0;
    _spect->_f1 = _f1;
    _ngx = 0;
    redraw ();
}


void Spectwin::set_fs (float f)
{
    if (f < 0.001 * (_fmax - _fmin)) f = 0.001 * (_fmax - _fmin);
    _f0 = _fc - 0.5 * f;   
    _f1 = _fc + 0.5 * f;   
    if (_f0 < _fmin) { _f0 = _fmin; _f1 = 2 * _fc - _f0; }
    if (_f1 > _fmax) { _f1 = _fmax; _f0 = 2 * _fc - _f1; }
    _p_val = _f1 - _f0;
    _spect->_f0 = _f0;
    _spect->_f1 = _f1;
    _ngx = 0;
    redraw ();
}


void Spectwin::set_a1 (float a)
{
    float d = _a1 - _a0;
    _a1 = a;
    _a0 = _a1 - d;
    if (_a1 > _amax) { _a1 = _amax; _a0 = _a1 - d; }
    if (_a0 < _amin) { _a0 = _amin; _a1 = _a0 + d; }
    _p_val = _a1;
    _ngy = 0;
    redraw ();
}


void Spectwin::set_ar (float a)
{
    _a0 = _a1 - a;
    if (_a0 < _amin) _a0 = _amin;
    if (_a0 > _a1 - 5) _a0 = _a1 - 5;
    _p_val = _a1 - _a0;
    _ngy = 0;
    redraw ();
}


void Spectwin::set_a_ns (float a)
{
    _a_ns = a;
    send_genp ();
}


void Spectwin::set_a_si (float a)
{
    _p_val = _a_si = a;
    send_genp ();
    show_param ();
}


void Spectwin::set_f_si (float a)
{
    _f_si = a;
    send_genp ();
}


void Spectwin::send_genp (void)
{
    M_genpar *X = new M_genpar ();
    X->_g_bits  = _g_bits;
    X->_a_noise = _a_ns;
    X->_a_sine  = _a_si;
    X->_f_sine  = _f_si;
    _audio->put_event (EV_MESG, X);
}


void Spectwin::set_mark (float f, bool nse)
{
    if (_spect->_bits & Spectdata::MK1_SET)
    {
        _spect->_mk2f = f;
        _spect->_bits |= Spectdata::MK2_SET;
        if (nse) _spect->_bits |=  Spectdata::MK2_NSE;
        else     _spect->_bits &= ~Spectdata::MK2_NSE;
    }
    else
    {
        _spect->_mk1f = f;
        _spect->_bits |= Spectdata::MK1_SET;
        if (nse) _spect->_bits |=  Spectdata::MK1_NSE;
        else     _spect->_bits &= ~Spectdata::MK1_NSE;
    }
}


void Spectwin::clr_mark (void)
{
    _spect->_bits &= ~(Spectdata::MK1_SET | Spectdata::MK2_SET);
}


float Spectwin::calcfreq (int x)
{
    return _f0 + (_f1 - _f0) * (x - LMAR) / (_xs - LMAR - RMAR);
}


void Spectwin::show_param (void)
{
    char  s [16];
    
    *s = 0;
    switch (_p_ind)
    {
    case BANDW:
    case FMIN:
    case FMAX:
    case FCENT:
    case FSPAN:
    case SFREQ:
	if      (_p_val < 1e3) sprintf (s, "%5.3f", _p_val);
        else if (_p_val < 1e6) sprintf (s, "%5.3fk", _p_val / 1e3);
        else                   sprintf (s, "%5.3fM", _p_val / 1e6);
        break;

    case AMAX:
    case ASPAN:
        sprintf (s, "%1.0f dB", _p_val);
        break;

    case NSLEV:
    case SILEV:
        sprintf (s, "%2.1f dB", _p_val);
        break; 
    }
    _txt1->set_text (s);
    _txt1->clear_modified ();
}


void Spectwin::plot_fscale (void)
{
    int   i, j, k0, k1, x;
    float f, g, xr;
    char  s [16]; 
    X_draw D (dpy (), win (), dgc (), xft ());

    D.setcolor (XftColors.spect_sc);
    D.setfont (XftFonts.scales);
    D.setfunc (GXcopy);
 
    xr = _xs - LMAR - RMAR - 1;
    if (! _ngx)
    {
	f = fabs (_f0);
	g = fabs (_f1);
	if (g > f) f = g;
	if      (f > 2e6f) i = 6;
	else if (f > 2e3f) i = 3;
	else               i = 0;
	_funit = powf (10.0f, i);
	f = 80 * (_f1 - _f0) / xr;
	f /= _funit;
	if      (f < 0.01f) _df = 0.01f;
	else if (f < 0.02f) _df = 0.02f;
	else if (f < 0.05f) _df = 0.05f;
	else if (f < 0.1f)  _df = 0.1f;
	else if (f < 0.2f)  _df = 0.2f;
	else if (f < 0.5f)  _df = 0.5f;
	else if (f < 1)     _df = 1;
	else if (f < 2)     _df = 2;
	else if (f < 5)     _df = 5;
	else if (f < 10)    _df = 10;
	else if (f < 20)    _df = 20;
	else if (f < 50)    _df = 50;
	else if (f < 100)   _df = 100;
	else if (f < 200)   _df = 200;
        else                _df = 500; 

	if (_df < 1.0f) i++;
	if (_df < 0.1f) i++;
	_fform = _formats [i];
	_df *= 0.5f * _funit;
    }

    k0 = (int)(ceilf (_f0 / _df));
    k1 = (int)(floorf (_f1 / _df));

    j = 0;
    for (i = k0; i <= k1; i++)
    {
	f = i * _df;
        x = (int)((f - _f0) / (_f1 - _f0) * xr + 0.5f);  
        sprintf (s, _fform, f / _funit);
        D.move (x + LMAR, _ys - BMAR);
        D.rdraw (0, 5);
        if (! (i & 1))
	{
            if (! _ngx) _grx [j++] = x;
            D.rmove (0, 10);        
            D.drawstring (s, 0);
	}
    }
    if (! _ngx) _ngx = j;
}


void Spectwin::plot_ascale (void)
{
    int   i, j, k0, k1, y;
    float a, yr;
    char  s [16]; 
    X_draw D (dpy (), win (), dgc (), xft ());

    D.setfont (XftFonts.scales);
    D.setcolor (XftColors.spect_sc);
    D.setfunc (GXcopy);
 
    yr = _ys - TMAR - BMAR - 1;
    if (! _ngy)
    {
	a = 60 * (_a1 - _a0) / yr;
	if      (a < 1)  _da = 1;
	else if (a < 2)  _da = 2;
	else if (a < 5)  _da = 5;
	else if (a < 10) _da = 10;
	else             _da = 20;
    }

    k0 = (int)(ceil (_a0 / _da));
    k1 = (int)(floor (_a1 / _da));
    j = 0;
    for (i = k0; i <= k1; i++)
    {
	a = i * _da;
        y = (int)(yr - (a - _a0) / (_a1 - _a0) * yr + 0.5);  
        sprintf (s, "%1.0f", a);
        D.move (LMAR, y + TMAR);
        D.rdraw (-5, 0);
        if (! _ngy) _gry [j++] = y;
        D.rmove (-3, 4);        
        D.drawstring (s, 1);
    }
    if (! _ngy) _ngy = j;
}


void Spectwin::plot_clear (void)
{
    X_draw D (dpy (), _plotmap, _plotgct, 0);

    D.setcolor (Colors.spect_bg);
    D.fillrect (0, 0, _xs, _ys);
}


void Spectwin::plot_grid (void)
{
    int i;

    X_draw D (dpy (), _plotmap, _plotgct, 0);

    D.setcolor (Colors.spect_gr ^ Colors.spect_bg);
    D.setfunc (GXxor); 
    for (i = 0; i < _ngx; i++)
    {
	D.move (_grx [i], 0);
        D.rdraw (0, _ys);
    }
    for (i = 0; i < _ngy; i++)
    {
	D.move (0, _gry [i]);
        D.rdraw (_xs, 0);
    }
}


void Spectwin::plot_spect (Spectdata *Z)
{
    int i, n, x, y;
    XPoint P [XMAX - LMAR - RMAR];
    float  sx, ry, sy, v; 

    X_draw D (dpy (), _plotmap, _plotgct, 0);
    D.setline (0);
    D.setfunc (GXcopy);
    ry = _ys - TMAR - BMAR - 1;
    sy = ry / (_a1 - _a0);

    if (Z->_bits & Spectdata::YM_VAL)
    {
        D.setcolor (Colors.spect_pkA);
        for (i = n = 0; i < Z->_npix; i++)
        {
  	    if (Z->_ym [i] >= 0)
	    {
		v = 10 * log10f (Z->_ym [i] + 1e-30f);
	        P [n].x = i;
	        P [n++].y = (int)(ry - (v - _a0) * sy + 0.5f);  
	    }
	}
        D.drawlines (n, P);
    }

    if (Z->_bits & Spectdata::YP_VAL)
    {
        D.setcolor (Colors.spect_trA);
        for (i = n = 0; i < Z->_npix; i++)
        {
  	    if (Z->_yp [i] >= 0)
	    {
		v = 10 * log10f (Z->_yp [i] + 1e-30f);
	        P [n].x = i;
	        P [n++].y = (int)(ry - (v - _a0) * sy + 0.5f);  
	    }
	}
        D.drawlines (n, P);
    }

    sx =  (_xs - LMAR - RMAR - 1) / (Z->_f1 - Z->_f0);
    D.setcolor (Colors.spect_mk);

    if (Z->_bits & Spectdata::MK1_SET)
    {
	v = 10 * log10f (Z->_mk1p + 1e-30f);
        y = (int)(ry - (v - _a0) * sy + 0.5f);
        x = (int)((Z->_mk1f - Z->_f0) * sx + 0.5f);
        D.drawrect (x - 3, y - 3, x + 3, y + 3);
        D.drawrect (x - 5, y - 5, x + 5, y + 5);
    }
    if (Z->_bits & Spectdata::MK2_SET)
    {
	v = 10 * log10f (Z->_mk2p + 1e-30f);
        y = (int)(ry - (v - _a0) * sy + 0.5f);
        x = (int)((Z->_mk2f - Z->_f0) * sx + 0.5);
        D.drawrect (x - 3, y - 3, x + 3, y + 3);
        D.drawrect (x - 5, y - 5, x + 5, y + 5);
    }
}


void Spectwin::print_note (char *s, float f)
{
    int n;
    
    f = fmodf (12 * log2 (f / 440.0f) + 129, 12.0f);
    n = (int)(floorf (f + 0.5f)); 
    f -= n;
    if (n == 12) n = 0;
    sprintf (s, "  %5s (%+2.0lf)", _notes [n], 100.0f * f);  
}


void Spectwin::plot_annot (Spectdata *Z)
{
    char  s [64];
    float v1, v2;
    int   k;

    X_draw D (dpy (), _plotmap, _plotgct, xft ());

    D.setcolor (XftColors.spect_an);
    D.setfont (XftFonts.labels);
    D.setfunc (GXcopy);

    sprintf (s, "BW = %4.2lf Hz = %5.2lf dBHz, VA = %d, Ptot = %5.2lf", 
             Z->_bw, 
             10 * log10 (Z->_bw),
             Z->_avcnt,
             10 * log10 (_ptot));
    D.move (10, 15);
    D.drawstring (s, -1);        

    if (Z->_bits & Spectdata::MK1_SET)
    {
      if (Z->_bits & Spectdata::MK1_NSE)
      {
  	 v1 = 10 * log10 (Z->_mk1p * _fftlen / (2.02 * _fsamp));
         sprintf (s, "Mk1 = %8.1lf Hz, %7.2lf dB/Hz", Z->_mk1f, v1);
      }
      else 
      {
  	 v1 = 10 * log10 (Z->_mk1p);
         k = sprintf (s, "Mk1 = %8.1lf Hz, %7.2lf dB", Z->_mk1f, v1);
         print_note (s + k, Z->_mk1f);
      }
      D.move (10, 33);
      D.drawstring (s, -1);

      if (Z->_bits & Spectdata::MK2_SET)
      {
        if (Z->_bits & Spectdata::MK2_NSE)
        {
    	  v2 = 10 * log10 (Z->_mk2p * _fftlen / (2.02 * _fsamp));
          sprintf (s, "Mk2 = %8.1lf Hz, %7.2lf dB/Hz", Z->_mk2f, v2);
          D.move (10, 51);
          D.drawstring (s, -1);
          if (Z->_bits & Spectdata::MK1_NSE)
          {
  	     sprintf (s, "Del = %8.1lf Hz, %7.2lf dB", Z->_mk2f - Z->_mk1f, v2 - v1);
          }
          else 
          {
  	     sprintf (s, "Del = %8.1lf Hz, %7.2lf dB/Hz", Z->_mk2f - Z->_mk1f, v2 - v1);
          }
          D.move (10, 69);
          D.drawstring (s, -1);
        }
        else 
        {
          v2 = 10 * log10 (Z->_mk2p);
          k = sprintf (s, "Mk2 = %8.1lf Hz, %7.2lf dB", Z->_mk2f, v2);
          print_note (s + k, Z->_mk2f);
          D.move (10, 51);
          D.drawstring (s, -1);
          if (Z->_bits & Spectdata::MK1_NSE)
          {
  	     sprintf (s, "Del = %8.1lf Hz, %7.2lf dBHz", Z->_mk2f - Z->_mk1f, v2 - v1);
          }
          else 
          {
  	     sprintf (s, "Del = %8.1lf Hz, %7.2lf dB, (%5.3lf)", Z->_mk2f - Z->_mk1f, v2 - v1, Z->_mk2f / Z->_mk1f);
          }
          D.move (10, 69);
          D.drawstring (s, -1);
        }
      }
    }
}


void Spectwin::alloc_fft (Spectdata *S)
{
    int    k;
    float  b;

    b = 1.41f * _fsamp / S->_bw;;
    k = FFT_MIN;
    while ((k < FFT_MAX) && (k < b)) k *= 2; 
    S->_bw = 2.00 * _fsamp / k;
    if (_fftlen != k)
    {
	if (_fftplan) fftwf_destroy_plan (_fftplan);
        S->_bits |= Spectdata::RESET;
        _fftplan = fftwf_plan_dft_r2c_1d (_fftlen = k, _fftbuf, _trbuf + 4, FFTW_ESTIMATE);
        _ipmod = k / (2 * INP_LEN);
        if (_ipmod < 1) _ipmod = 1;
    }
}


void Spectwin::handle_mesg (ITC_mesg *M)
{
    char s [256];

    if (M->type () == M_JINFO)
    {
	M_jinfo *Z = (M_jinfo *) M;
        set_fsamp ((float)(Z->_fsamp), false);
        sprintf (s, "%s-%s  [%s]", PROGNAME, VERSION, Z->_jname);
        x_set_title (s);
        _ipcnt = 0;
        _audio->put_event (EV_MESG, new M_buffp (_ipbuf, INP_MAX, INP_LEN));     
    }
    M->recover ();
}


void Spectwin::handle_trig ()
{
    int i, h, k;
    float  a, b, p, s, *pow;

    k = ++_ipcnt * INP_LEN;
    if (k == INP_MAX) _ipcnt = 0;
    if (_ipcnt % _ipmod == 0)
    {
        if (_spect->_bits & Spectdata::FREEZE) return;
        if (k < _fftlen) 
	{
	    memcpy (_ipbuf + INP_MAX, _ipbuf, k * sizeof (float));
            k += INP_MAX;
	}
	memcpy (_fftbuf, _ipbuf + k - _fftlen, _fftlen * sizeof (float));
        fftwf_execute_dft_r2c (_fftplan, _fftbuf, _trbuf + 4);
        k = _fftlen / 2;

        for (i = 1; i <= 4; i++)
	{
	    _trbuf [4 - i][0] =  _trbuf [4 + i][0];
	    _trbuf [4 - i][1] = -_trbuf [4 + i][1];
	    _trbuf [4 + k + i][0] =  _trbuf [4 + k - i][0];
	    _trbuf [4 + k + i][1] = -_trbuf [4 + k - i][1];
	}

	if (_spect->_bits & Spectdata::RESET)
	{
	    _spect->_bits ^= Spectdata::RESET;
	    memset (_power, 0, (FFT_MAX + 1) * sizeof (float));
	    if (_spect->_avcnt) _spect->_avcnt = 1;
	}
        k = _spect->_avcnt; 
        h = _spect->_bits & Spectdata::PEAKH;
        a = 1.0f;
        if (k)
	{
            a = 1.0f / k;
            if (k < _spect->_avmax) _spect->_avcnt = k + 1;
	} 
        b = 4.0f / ((float)_fftlen * (float)_fftlen);
        s = 0;
        pow = _power;
        for (i = 0; i < _fftlen / 2; i++)
	{
	    p = b * conv0 (_trbuf + 4 + i);
            s += p;
            if (k) *pow += a * (p - *pow);
	    else if (h) { if (p > *pow) *pow = p; }
            else   *pow = p;
            pow++;
	    p = b * conv1 (_trbuf + 4 + i);
            s += p;
            if (k) *pow += a * (p - *pow);
	    else if (h) { if (p > *pow) *pow = p; }
            else   *pow = p;
            pow++;
	}
        p = b * conv0 (_trbuf + 4 + i);
        s += p;
        if (k) *pow += a * (p - *pow);
        else if (h) { if (p > *pow) *pow = p; }
        else   *pow = p;
        _ptot += 0.1f * (0.25f * s - _ptot) + 1e-20f;
	update ();
    }
}


float Spectwin::conv0 (fftwf_complex *v)
{
    float x, y;
    x =  v [0][0]
        - 0.677014f * (v [-1][0] + v [1][0])
        + 0.195602f * (v [-2][0] + v [2][0])
        - 0.019420f * (v [-3][0] + v [3][0])
        + 0.000741f * (v [-4][0] + v [4][0]);
    y =  v [0][1]
        - 0.677014f * (v [-1][1] + v [1][1])
        + 0.195602f * (v [-2][1] + v [2][1])
        - 0.019420f * (v [-3][1] + v [3][1])
        + 0.000741f * (v [-4][1] + v [4][1]);
    return x * x + y * y;
}


float Spectwin::conv1 (fftwf_complex *v)
{
    float x, y;
    x =   0.908040f * (v [ 0][0] - v [1][0])
	- 0.409037f * (v [-1][0] - v [2][0])
	+ 0.071556f * (v [-2][0] - v [3][0])
	- 0.004085f * (v [-3][0] - v [4][0]);
    y =   0.908040f * (v [ 0][1] - v [1][1])
	- 0.409037f * (v [-1][1] - v [2][1])
	+ 0.071556f * (v [-2][1] - v [3][1])
	- 0.004085f * (v [-3][1] - v [4][1]);
    return x * x + y * y;
}


void Spectwin::calc_spect (Spectdata *S)
{
    int   i, j, k;
    float dc, dd, f0, fd, fc, p, pp, pm;

    dc = _fsamp / (2 * _fftlen);

    dd = (S->_f1 - S->_f0) / (S->_npix - 1);
    S->_bits |= Spectdata::YP_VAL;
    if (dd > dc) S->_bits |=  Spectdata::YM_VAL;
    else         S->_bits &= ~Spectdata::YM_VAL;
    f0 = S->_f0 - 0.5 * dd;
    j = (int)(ceil (f0 / dc)); 
    if (j < 0) j = 0;

    for (i = 0; i < S->_npix; i++)
    {
        fd = f0 + (i + 1) * dd;
        fc = j * dc; 
        pp = pm = 0;
        for (k = 0; fc < fd; j++, fc += dc)
        {
            if (j > _fftlen) break;
            p = _power [j];
            if (p > pp) pp = p; 
            pm += p;
            k++;
        }
        if (k)
        {
            S->_yp [i] = pp;
            S->_ym [i] = pm / k;
        }
        else
        {
            S->_yp [i] = -1;
            S->_ym [i] = -1;
        }
    }

    if (S->_bits & Spectdata::MK1_SET)
    {
        if (S->_bits & Spectdata::MK1_NSE) calc_noise (&(S->_mk1f), &(S->_mk1p));
        else calc_peak (&(S->_mk1f), &(S->_mk1p), dd / dc);

    }    
    if (S->_bits & Spectdata::MK2_SET)
    {
        if (S->_bits & Spectdata::MK2_NSE) calc_noise (&(S->_mk2f), &(S->_mk2p));
        else calc_peak (&(S->_mk2f), &(S->_mk2p), dd / dc);
    }    
}


void Spectwin::calc_noise (float *f, float *p)
{
    int    i, j;
    float  z;
  
    j = (int)(floor ((*f) * 2 * _fftlen / _fsamp + 0.5));
    if (j < 10) j = 10;
    if (j > _fftlen - 10) j = _fftlen - 10;
    z = 0;
    for (i = -10; i <= 10; i++) z += _power [i + j];
    *p =  z / 21.0;
}


void Spectwin::calc_peak (float *f, float *p, float r)
{
    int    i, j, k, n;
    float  a, b, c, yl, yr, z;
  
    j = (int)(floor ((*f) * 2 * _fftlen / _fsamp + 0.5));
    n = (int)(10 * r);
    if (n < 2) n = 2;
    if (j < n) j = n;
    if (j > _fftlen - n) j = _fftlen - n;
    z = -1;
    k = 0;
    for (i = -n; i <= n; i++)
    {
      if (_power [j + i] > z)
      {
        k = i;
        z = _power [j + i];
      }
    }
    if (abs (k) == n)
    {
      *p = _power [j];
      return;
    }
    k += j;

    a  = sqrt (_power [k]);
    yl = sqrt (_power [k - 1]);   
    yr = sqrt (_power [k + 1]);   
    b = 0.5 * (yr - yl);
    c = 0.5 * (yr + yl) - a;

    *f = 0.5 * _fsamp * (k - 0.5 * b / c) / _fftlen;
    a -= 0.25 * b * b / c;
    *p = a * a;
}

