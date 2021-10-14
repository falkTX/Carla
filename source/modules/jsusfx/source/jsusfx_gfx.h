/*
 * Copyright 2018 Pascal Gauthier, Marcel Smit
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * *distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#pragma once

#include "WDL/eel2/ns-eel.h"

class JsusFx;

struct JsusFxGfx {
	EEL_F *m_gfx_r;
	EEL_F *m_gfx_g;
	EEL_F *m_gfx_b;
	EEL_F *m_gfx_a;
	
	EEL_F *m_gfx_w;
	EEL_F *m_gfx_h;
	
	EEL_F *m_gfx_x;
	EEL_F *m_gfx_y;
	
	EEL_F *m_gfx_mode;
	EEL_F *m_gfx_clear;
	EEL_F *m_gfx_texth;
	EEL_F *m_gfx_dest;
	
	EEL_F *m_mouse_x;
	EEL_F *m_mouse_y;
	EEL_F *m_mouse_cap;
	EEL_F *m_mouse_wheel;
	
	virtual ~JsusFxGfx() { }
	
	void init(NSEEL_VMCTX vm);
	
	virtual void setup(const int w, const int h) { }
	
	virtual void beginDraw() { }
	virtual void endDraw() { }
	
	virtual void gfx_line(int np, EEL_F ** params) { }
	virtual void gfx_rect(int np, EEL_F ** params) { }
	virtual void gfx_circle(EEL_F x, EEL_F y, EEL_F radius, bool fill, bool aa) { }
	virtual void gfx_triangle(EEL_F ** params, int np) { }
	
	virtual void gfx_lineto(EEL_F xpos, EEL_F ypos, EEL_F useaa) { }
	virtual void gfx_rectto(EEL_F xpos, EEL_F ypos) { }
	virtual void gfx_arc(int np, EEL_F ** params) { }
	virtual void gfx_set(int np, EEL_F ** params) { }
	
	virtual void gfx_roundrect(int np, EEL_F ** params) { }
	
	virtual void gfx_grad_or_muladd_rect(int mod, int np, EEL_F ** params) { }
	
	virtual void gfx_drawnumber(EEL_F n, int nd) { }
	virtual void gfx_drawchar(EEL_F n) { }
	virtual void gfx_drawstr(void * opaque, EEL_F ** parms, int np, int mode) { } // mode=1 for format, 2 for purely measure no format, 3 for measure char
	
	virtual void gfx_setpixel(EEL_F r, EEL_F g, EEL_F b) { }
	virtual void gfx_getpixel(EEL_F * r, EEL_F * g, EEL_F * b) { }

	virtual EEL_F gfx_loadimg(JsusFx & jsusFx, int img, EEL_F loadFrom) { return 0.f; }
	virtual void gfx_getimgdim(EEL_F img, EEL_F * w, EEL_F * h) { }
	virtual EEL_F gfx_setimgdim(int img, EEL_F * w, EEL_F * h) { return 0.f; }
	
	virtual EEL_F gfx_setfont(int np, EEL_F ** parms) { return 0.f; }
	virtual EEL_F gfx_getfont(int np, EEL_F ** parms) { return 0.f; }
	
	virtual EEL_F gfx_showmenu(EEL_F ** parms, int nparms) { return 0.f; }
	virtual EEL_F gfx_setcursor(EEL_F ** parms, int nparms) { return 0.f; }
	
	virtual void gfx_blurto(EEL_F x, EEL_F y) { }

	virtual void gfx_blit(EEL_F img, EEL_F scale, EEL_F rotate) { }
	virtual void gfx_blitext(EEL_F img, EEL_F * coords, EEL_F angle) { }
	virtual void gfx_blitext2(int mp, EEL_F ** params, int blitmode) { } // 0=blit, 1=deltablit
	
	virtual int gfx_getchar(int p) { return 0; }
};

// the logging based GFX implementation below is used to see if the API works, and to see which calls a script is making. it's use doesn't extend beyong this basic debugging facility

#define GFXLOG printf("%s called!\n", __FUNCTION__)

struct JsusFxGfx_Log : JsusFxGfx {
	virtual void gfx_line(int np, EEL_F ** params) override { GFXLOG; }
	virtual void gfx_rect(int np, EEL_F ** params) override { GFXLOG; }
	virtual void gfx_circle(EEL_F x, EEL_F y, EEL_F radius, bool fill, bool aa) override { GFXLOG; }
	virtual void gfx_triangle(EEL_F ** params, int np) override { GFXLOG; }
	
	virtual void gfx_lineto(EEL_F xpos, EEL_F ypos, EEL_F useaa) override { GFXLOG; }
	virtual void gfx_rectto(EEL_F xpos, EEL_F ypos) override { GFXLOG; }
	virtual void gfx_arc(int np, EEL_F ** params) override { GFXLOG; }
	virtual void gfx_set(int np, EEL_F ** params) override { GFXLOG; }
	
	virtual void gfx_roundrect(int np, EEL_F ** params) override { GFXLOG; }
	
	virtual void gfx_grad_or_muladd_rect(int mod, int np, EEL_F ** params) override { GFXLOG; }
	
	virtual void gfx_drawnumber(EEL_F n, int nd) override { GFXLOG; }
	virtual void gfx_drawchar(EEL_F n) override { GFXLOG; }
	virtual void gfx_drawstr(void * opaque, EEL_F ** parms, int np, int mode) override { GFXLOG; }
	
	virtual void gfx_setpixel(EEL_F r, EEL_F g, EEL_F b) override { GFXLOG; }
	virtual void gfx_getpixel(EEL_F * r, EEL_F * g, EEL_F * b) override { GFXLOG; }

	virtual EEL_F gfx_loadimg(JsusFx & jsusFx, int img, EEL_F loadFrom) override { GFXLOG; return -1.f; }
	virtual void gfx_getimgdim(EEL_F img, EEL_F * w, EEL_F * h) override { GFXLOG; }
	virtual EEL_F gfx_setimgdim(int img, EEL_F * w, EEL_F * h) override { GFXLOG; return 0.f; }
	
	virtual EEL_F gfx_setfont(int np, EEL_F ** parms) override { GFXLOG; return 0.f; }
	virtual EEL_F gfx_getfont(int np, EEL_F ** parms) override { GFXLOG; return 0.f; }
	
	virtual EEL_F gfx_showmenu(EEL_F ** parms, int nparms) override { GFXLOG; return 0.f; }
	virtual EEL_F gfx_setcursor(EEL_F ** parms, int nparms) override { GFXLOG; return 0.f; }
	
	virtual void gfx_blurto(EEL_F x, EEL_F y) override { GFXLOG; }

	virtual void gfx_blit(EEL_F img, EEL_F scale, EEL_F rotate) override { GFXLOG; }
	virtual void gfx_blitext(EEL_F img, EEL_F * coords, EEL_F angle) override { GFXLOG; }
	virtual void gfx_blitext2(int mp, EEL_F ** params, int blitmode) override { GFXLOG; }
};

#undef GFXLOG
