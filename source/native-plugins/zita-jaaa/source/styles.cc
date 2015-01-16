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


#include "styles.h"


struct colors      Colors;
struct fonts       Fonts;
struct xft_colors  XftColors_jaaa;
struct xft_fonts   XftFonts_jaaa;

X_button_style  Bst0, Bst1, BstA, BstB, BstM;
X_textln_style  Tst0, Tst1;


void init_styles (X_display *disp, X_resman *xrm)
{
    Colors.black = disp->blackpixel ();
    Colors.white = disp->whitepixel ();
    Colors.main_bg   = disp->alloc_color (xrm->get (".color.main.bg",   "#e4e0d4"),   Colors.white);
    Colors.main_ds   = disp->alloc_color (xrm->get (".color.main.ds",   "gray20"),    Colors.black);
    Colors.main_ls   = disp->alloc_color (xrm->get (".color.main.ls",   "white"),     Colors.white);
    Colors.text_ed   = disp->alloc_color (xrm->get (".color.text.ed",   "yellow"),    Colors.white);
    Colors.text_ca   = disp->alloc_color (xrm->get (".color.text.ca",   "black"),     Colors.black);
    Colors.spect_bg  = disp->alloc_color (xrm->get (".color.spect.bg",  "white"),     Colors.white);
    Colors.spect_gr  = disp->alloc_color (xrm->get (".color.spect.gr",  "gray80"),    Colors.white);
    Colors.spect_mk  = disp->alloc_color (xrm->get (".color.spect.mk",  "red"),       Colors.black);
    Colors.spect_trA = disp->alloc_color (xrm->get (".color.spect.trA", "blue"),      Colors.black);
    Colors.spect_pkA = disp->alloc_color (xrm->get (".color.spect.pkA", "#A0A0FF"),   Colors.black);
    Colors.spect_trB = disp->alloc_color (xrm->get (".color.spect.trB", "blue"),      Colors.black);
    Colors.spect_pkB = disp->alloc_color (xrm->get (".color.spect.pkB", "#A0A0FF"),   Colors.black);
    Colors.spect_trM = disp->alloc_color (xrm->get (".color.spect.trM", "darkgreen"), Colors.white);
    Colors.butt_bg0  = disp->alloc_color (xrm->get (".color.butt.bg0",  "#d0c8c0"),   Colors.white);
    Colors.butt_bg1  = disp->alloc_color (xrm->get (".color.butt.bg1",  "yellow"),    Colors.white);
    Colors.butt_bg2  = disp->alloc_color (xrm->get (".color.butt.bg2",  "green"),     Colors.white);
    Colors.butt_bgA  = disp->alloc_color (xrm->get (".color.butt.bgA",  "red"),       Colors.black);
    Colors.butt_bgB  = disp->alloc_color (xrm->get (".color.butt.bgB",  "blue"),      Colors.black);
    Colors.butt_bgM  = disp->alloc_color (xrm->get (".color.butt.bgM",  "green"),     Colors.white);

    XftColors_jaaa.white    = disp->alloc_xftcolor ("white", 0);
    XftColors_jaaa.black    = disp->alloc_xftcolor ("black", 0);
    XftColors_jaaa.main_fg  = disp->alloc_xftcolor (xrm->get (".color.main.fg",   "black"), XftColors_jaaa.black);
    XftColors_jaaa.text_fg  = disp->alloc_xftcolor (xrm->get (".color.text.fg",   "black"), XftColors_jaaa.black);
    XftColors_jaaa.spect_fg = disp->alloc_xftcolor (xrm->get (".color.spect.fg",  "blue"),  XftColors_jaaa.black);
    XftColors_jaaa.spect_sc = disp->alloc_xftcolor (xrm->get (".color.spect.sc",  "black"), XftColors_jaaa.black);
    XftColors_jaaa.spect_an = disp->alloc_xftcolor (xrm->get (".color.spect.an",  "red"),   XftColors_jaaa.black);
    XftColors_jaaa.butt_fg0 = disp->alloc_xftcolor (xrm->get (".color.butt.fg0",  "black"), XftColors_jaaa.black);
    XftColors_jaaa.butt_fg1 = disp->alloc_xftcolor (xrm->get (".color.butt.fg1",  "black"), XftColors_jaaa.black);
    XftColors_jaaa.butt_fg2 = disp->alloc_xftcolor (xrm->get (".color.butt.fg2",  "black"), XftColors_jaaa.black);
    XftColors_jaaa.butt_fgA = disp->alloc_xftcolor (xrm->get (".color.butt.fgA",  "white"), XftColors_jaaa.white);
    XftColors_jaaa.butt_fgB = disp->alloc_xftcolor (xrm->get (".color.butt.fgB",  "white"), XftColors_jaaa.white);
    XftColors_jaaa.butt_fgM = disp->alloc_xftcolor (xrm->get (".color.butt.fgM",  "black"), XftColors_jaaa.black);

    XftFonts_jaaa.about1 = disp->alloc_xftfont (xrm->get (".font.about1", "times:pixelsize=24"));
    XftFonts_jaaa.about2 = disp->alloc_xftfont (xrm->get (".font.about2", "times:pixelsize=14"));
    XftFonts_jaaa.button = disp->alloc_xftfont (xrm->get (".font.button", "luxi:pixelsize=11"));
    XftFonts_jaaa.labels = disp->alloc_xftfont (xrm->get (".font.labels", "luxi:pixelsize=11"));
    XftFonts_jaaa.scales = disp->alloc_xftfont (xrm->get (".font.scales", "luxi:pixelsize=10"));

    Bst0.font = XftFonts_jaaa.button;
    Bst0.color.bg [0] = Colors.butt_bg0;
    Bst0.color.fg [0] = XftColors_jaaa.butt_fg0;
    Bst0.color.bg [1] = Colors.butt_bg1;
    Bst0.color.fg [1] = XftColors_jaaa.butt_fg1;
    Bst0.color.bg [2] = Colors.butt_bg2;
    Bst0.color.fg [2] = XftColors_jaaa.butt_fg2;
    Bst0.color.shadow.bgnd = Colors.main_bg;
    Bst0.color.shadow.lite = Colors.main_ls;
    Bst0.color.shadow.dark = Colors.main_ds;
    Bst0.size.x = 17;
    Bst0.size.y = 17;
    Bst0.type = X_button_style::RAISED;

    Bst1.font = XftFonts_jaaa.button;
    Bst1.color.bg [0] = Colors.butt_bg0;
    Bst1.color.fg [0] = XftColors_jaaa.butt_fg0;
    Bst1.color.bg [1] = Colors.butt_bg1;
    Bst1.color.fg [1] = XftColors_jaaa.butt_fg1;
    Bst1.color.bg [2] = Colors.butt_bg2;
    Bst1.color.fg [2] = XftColors_jaaa.butt_fg2;
    Bst1.color.shadow.bgnd = Colors.main_bg;
    Bst1.color.shadow.lite = Colors.main_ls;
    Bst1.color.shadow.dark = Colors.main_ds;
    Bst1.size.x = 17;
    Bst1.size.y = 17;
    Bst1.type = X_button_style::RAISED;// | X_button_style::LED;

    BstA = Bst0;
    BstA.color.bg [1] = Colors.butt_bgA;
    BstA.color.fg [1] = XftColors_jaaa.butt_fgA;

    BstB = Bst0;
    BstB.color.bg [1] = Colors.butt_bgB;
    BstB.color.fg [1] = XftColors_jaaa.butt_fgB;

    BstM = Bst0;
    BstM.color.bg [1] = Colors.butt_bgM;
    BstM.color.fg [1] = XftColors_jaaa.butt_fgM;

    Tst0.font = XftFonts_jaaa.labels;
    Tst0.color.normal.bgnd = Colors.white;
    Tst0.color.normal.text = XftColors_jaaa.text_fg;
    Tst0.color.shadow.lite = Colors.main_ls;
    Tst0.color.shadow.dark = Colors.main_ds;
    Tst0.color.shadow.bgnd = Colors.main_bg;

    Tst1.font = XftFonts_jaaa.labels;
    Tst1.color.normal.bgnd = Colors.main_bg;
    Tst1.color.normal.text = XftColors_jaaa.main_fg;
    Tst1.color.focus.bgnd  = Colors.text_ed;
    Tst1.color.focus.text  = XftColors_jaaa.main_fg;
    Tst1.color.focus.line  = Colors.text_ca;
    Tst1.color.shadow.lite = Colors.main_ls;
    Tst1.color.shadow.dark = Colors.main_ds;
    Tst1.color.shadow.bgnd = Colors.main_bg;
}

