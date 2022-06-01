//
// This file is based in part on modified source code from `WDL/eel2/eel_lice.h`.
// The zlib license from the WDL applies to this source file.
//
//------------------------------------------------------------------------------
//
// Copyright (C) 2021 and later Jean Pierre Cimalando
// Copyright (C) 2005 and later Cockos Incorporated
//
//
// Portions copyright other contributors, see each source file for more information
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// SPDX-License-Identifier: Zlib
//

#pragma once
#include "ysfx_api_eel.hpp"
#include "WDL/wdlstring.h"
#include "WDL/wdlcstring.h"
#include "WDL/wdlutf8.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

// help clangd to figure things out
#if defined(__CLANGD__)
#   include "ysfx_api_gfx.cpp"
#endif

#define EEL_LICE_GET_CONTEXT(opaque) ((opaque) ? ((ysfx_t *)(opaque))->gfx.state->lice.get() : nullptr)

//------------------------------------------------------------------------------

#define LICE_FUNCTION_VALID(x) (sizeof(int) > 0)

static HDC LICE__GetDC(LICE_IBitmap *bm)
{
  return bm->getDC();
}
static int LICE__GetWidth(LICE_IBitmap *bm)
{
  return bm->getWidth();
}
static int LICE__GetHeight(LICE_IBitmap *bm)
{
  return bm->getHeight();
}
static void LICE__Destroy(LICE_IBitmap *bm)
{
  delete bm;
}
static void LICE__SetFromHFont(LICE_IFont * ifont, HFONT font, int flags)
{
  if (ifont) ifont->SetFromHFont(font,flags);
}
static LICE_pixel LICE__SetTextColor(LICE_IFont* ifont, LICE_pixel color)
{
  if (ifont) return ifont->SetTextColor(color);
  return 0;
}
static void LICE__SetTextCombineMode(LICE_IFont* ifont, int mode, float alpha)
{
  if (ifont) ifont->SetCombineMode(mode, alpha);
}
static int LICE__DrawText(LICE_IFont* ifont, LICE_IBitmap *bm, const char *str, int strcnt, RECT *rect, UINT dtFlags)
{
  if (ifont) return ifont->DrawText(bm, str, strcnt, rect, dtFlags);
  return 0;
}


static LICE_IFont *LICE_CreateFont()
{
  return new LICE_CachedFont();
}
static void LICE__DestroyFont(LICE_IFont *bm)
{
  delete bm;
}
static bool LICE__resize(LICE_IBitmap *bm, int w, int h)
{
  return bm->resize(w,h);
}

static LICE_IBitmap *__LICE_CreateBitmap(int mode, int w, int h)
{
  if (mode==1) return new LICE_SysBitmap(w,h);
  return new LICE_MemBitmap(w,h);
}

class eel_lice_state
{
public:

  eel_lice_state(NSEEL_VMCTX vm, void *ctx, int image_slots, int font_slots);
  ~eel_lice_state();

  LICE_IBitmap *m_framebuffer, *m_framebuffer_extra;
  int m_framebuffer_dirty;
  WDL_TypedBuf<LICE_IBitmap *> m_gfx_images;
  struct gfxFontStruct {
    LICE_IFont *font;
    char last_fontname[128];
    char actual_fontname[128];
    int last_fontsize;
    int last_fontflag;

    int use_fonth;
  }; 
  WDL_TypedBuf<gfxFontStruct> m_gfx_fonts;
  enum {
    EELFONT_FLAG_BOLD = (1<<24),
    EELFONT_FLAG_ITALIC = (2<<24),
    EELFONT_FLAG_UNDERLINE = (4<<24),
    EELFONT_FLAG_MASK = EELFONT_FLAG_BOLD|EELFONT_FLAG_ITALIC|EELFONT_FLAG_UNDERLINE
  };

  int m_gfx_font_active; // -1 for default, otherwise index into gfx_fonts (NOTE: this differs from the exposed API, which defines 0 as default, 1-n)
  LICE_IFont *GetActiveFont() { return m_gfx_font_active>=0&&m_gfx_font_active<m_gfx_fonts.GetSize() && m_gfx_fonts.Get()[m_gfx_font_active].use_fonth ? m_gfx_fonts.Get()[m_gfx_font_active].font : NULL; }

  LICE_IBitmap *GetImageForIndex(EEL_F idx, const char *callername) 
  { 
    if (idx>-2.0)
    {
      if (idx < 0.0) return m_framebuffer;

      const int a = (int)idx;
      if (a >= 0 && a < m_gfx_images.GetSize()) return m_gfx_images.Get()[a];
    }
    return NULL;
  };

  void SetImageDirty(LICE_IBitmap *bm)
  {
    if (bm == m_framebuffer && !m_framebuffer_dirty)
    {
      if (m_gfx_clear && *m_gfx_clear > -1.0)
      {
        const int a=(int)*m_gfx_clear;
        if (LICE_FUNCTION_VALID(LICE_Clear)) LICE_Clear(m_framebuffer,LICE_RGBA((a&0xff),((a>>8)&0xff),((a>>16)&0xff),0));
      }
      m_framebuffer_dirty=1;
    }
  }

  // R, G, B, A, w, h, x, y, mode(1=add,0=copy)
  EEL_F *m_gfx_r, *m_gfx_g, *m_gfx_b, *m_gfx_w, *m_gfx_h, *m_gfx_a, *m_gfx_x, *m_gfx_y, *m_gfx_mode, *m_gfx_clear, *m_gfx_texth,*m_gfx_dest, *m_gfx_a2;
  EEL_F *m_mouse_x, *m_mouse_y, *m_mouse_cap, *m_mouse_wheel, *m_mouse_hwheel;
  EEL_F *m_gfx_ext_retina;

  NSEEL_VMCTX m_vmref;
  void *m_user_ctx;

  void gfx_lineto(EEL_F xpos, EEL_F ypos, EEL_F aaflag);
  void gfx_rectto(EEL_F xpos, EEL_F ypos);
  void gfx_line(int np, EEL_F **parms);
  void gfx_rect(int np, EEL_F **parms);
  void gfx_roundrect(int np, EEL_F **parms);
  void gfx_arc(int np, EEL_F **parms);
  void gfx_set(int np, EEL_F **parms);
  void gfx_grad_or_muladd_rect(int mode, int np, EEL_F **parms);
  void gfx_setpixel(EEL_F r, EEL_F g, EEL_F b);
  void gfx_getpixel(EEL_F *r, EEL_F *g, EEL_F *b);
  void gfx_drawnumber(EEL_F n, EEL_F ndigits);
  void gfx_drawchar(EEL_F ch);
  void gfx_getimgdim(EEL_F img, EEL_F *w, EEL_F *h);
  EEL_F gfx_setimgdim(int img, EEL_F *w, EEL_F *h);
  void gfx_blurto(EEL_F x, EEL_F y);
  void gfx_blitext(EEL_F img, EEL_F *coords, EEL_F angle);
  void gfx_blitext2(int np, EEL_F **parms, int mode); // 0=blit, 1=deltablit
  void gfx_transformblit(EEL_F **parms, int div_w, int div_h, EEL_F *tab); // parms[0]=src, 1-4=x,y,w,h
  void gfx_circle(float x, float y, float r, bool fill, bool aaflag);
  void gfx_triangle(EEL_F** parms, int nparms);  
  void gfx_drawstr(void *opaque, EEL_F **parms, int nparms, int formatmode); // formatmode=1 for format, 2 for purely measure no format, 3 for measure char
  EEL_F gfx_loadimg(void *opaque, int img, EEL_F loadFrom);
  EEL_F gfx_setfont(void *opaque, int np, EEL_F **parms);
  EEL_F gfx_getfont(void *opaque, int np, EEL_F **parms);

  LICE_pixel getCurColor();
  int getCurMode();
  int getCurModeForBlit(bool isFBsrc);

  // these have to be **parms because of the hack for getting string from parm index
  EEL_F gfx_setcursor(void* opaque, EEL_F** parms, int nparms);
};

eel_lice_state::eel_lice_state(NSEEL_VMCTX vm, void *ctx, int image_slots, int font_slots)
{
  m_user_ctx=ctx;
  m_vmref= vm;
  m_gfx_font_active=-1;
  m_gfx_fonts.Resize(font_slots);
  memset(m_gfx_fonts.Get(),0,m_gfx_fonts.GetSize()*sizeof(m_gfx_fonts.Get()[0]));

  m_gfx_images.Resize(image_slots);
  memset(m_gfx_images.Get(),0,m_gfx_images.GetSize()*sizeof(m_gfx_images.Get()[0]));
  m_framebuffer=m_framebuffer_extra=0;
  m_framebuffer_dirty=0;

  m_gfx_r = NSEEL_VM_regvar(vm,"gfx_r");
  m_gfx_g = NSEEL_VM_regvar(vm,"gfx_g");
  m_gfx_b = NSEEL_VM_regvar(vm,"gfx_b");
  m_gfx_a = NSEEL_VM_regvar(vm,"gfx_a");
  m_gfx_a2 = NSEEL_VM_regvar(vm,"gfx_a2");

  m_gfx_w = NSEEL_VM_regvar(vm,"gfx_w");
  m_gfx_h = NSEEL_VM_regvar(vm,"gfx_h");
  m_gfx_x = NSEEL_VM_regvar(vm,"gfx_x");
  m_gfx_y = NSEEL_VM_regvar(vm,"gfx_y");
  m_gfx_mode = NSEEL_VM_regvar(vm,"gfx_mode");
  m_gfx_clear = NSEEL_VM_regvar(vm,"gfx_clear");
  m_gfx_texth = NSEEL_VM_regvar(vm,"gfx_texth");
  m_gfx_dest = NSEEL_VM_regvar(vm,"gfx_dest");
  m_gfx_ext_retina = NSEEL_VM_regvar(vm,"gfx_ext_retina");

  m_mouse_x = NSEEL_VM_regvar(vm,"mouse_x");
  m_mouse_y = NSEEL_VM_regvar(vm,"mouse_y");
  m_mouse_cap = NSEEL_VM_regvar(vm,"mouse_cap");
  m_mouse_wheel=NSEEL_VM_regvar(vm,"mouse_wheel");
  m_mouse_hwheel=NSEEL_VM_regvar(vm,"mouse_hwheel");

  if (m_gfx_texth) *m_gfx_texth=8;
}
eel_lice_state::~eel_lice_state()
{
  if (LICE_FUNCTION_VALID(LICE__Destroy)) 
  {
    LICE__Destroy(m_framebuffer_extra);
    LICE__Destroy(m_framebuffer);
    int x;
    for (x=0;x<m_gfx_images.GetSize();x++)
    {
      LICE__Destroy(m_gfx_images.Get()[x]);
    }
  }
  if (LICE_FUNCTION_VALID(LICE__DestroyFont))
  {
    int x;
    for (x=0;x<m_gfx_fonts.GetSize();x++)
    {
      if (m_gfx_fonts.Get()[x].font) LICE__DestroyFont(m_gfx_fonts.Get()[x].font);
    }
  }
}

int eel_lice_state::getCurMode()
{
  const int gmode = (int) (*m_gfx_mode);
  const int sm=(gmode>>4)&0xf;
  if (sm > LICE_BLIT_MODE_COPY && sm <= LICE_BLIT_MODE_HSVADJ) return sm;

  return (gmode&1) ? LICE_BLIT_MODE_ADD : LICE_BLIT_MODE_COPY;
}
int eel_lice_state::getCurModeForBlit(bool isFBsrc)
{
  const int gmode = (int) (*m_gfx_mode);
 
  const int sm=(gmode>>4)&0xf;

  int mode;
  if (sm > LICE_BLIT_MODE_COPY && sm <= LICE_BLIT_MODE_HSVADJ) mode=sm;
  else mode=((gmode&1) ? LICE_BLIT_MODE_ADD : LICE_BLIT_MODE_COPY);


  if (!isFBsrc && !(gmode&2)) mode|=LICE_BLIT_USE_ALPHA;
  if (!(gmode&4)) mode|=LICE_BLIT_FILTER_BILINEAR;
 
  return mode;
}
LICE_pixel eel_lice_state::getCurColor()
{
  int red=(int) (*m_gfx_r*255.0);
  int green=(int) (*m_gfx_g*255.0);
  int blue=(int) (*m_gfx_b*255.0);
  int a2=(int) (*m_gfx_a2*255.0);
  if (red<0) red=0;else if (red>255)red=255;
  if (green<0) green=0;else if (green>255)green=255;
  if (blue<0) blue=0; else if (blue>255) blue=255;
  if (a2<0) a2=0; else if (a2>255) a2=255;
  return LICE_RGBA(red,green,blue,a2);
}


static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_lineto(void *opaque, EEL_F *xpos, EEL_F *ypos, EEL_F *useaa)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_lineto(*xpos, *ypos, *useaa);
  return xpos;
}
static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_lineto2(void *opaque, EEL_F *xpos, EEL_F *ypos)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_lineto(*xpos, *ypos, 1.0f);
  return xpos;
}

static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_rectto(void *opaque, EEL_F *xpos, EEL_F *ypos)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_rectto(*xpos, *ypos);
  return xpos;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_line(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_line((int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_rect(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_rect((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_roundrect(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_roundrect((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_arc(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_arc((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_set(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_set((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_gradrect(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_grad_or_muladd_rect(0,(int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_muladdrect(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_grad_or_muladd_rect(1,(int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_deltablit(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_blitext2((int)np,parms,1);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_transformblit(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) 
  {
#ifndef EEL_LICE_NO_RAM
    const int divw = (int) (parms[5][0]+0.5);
    const int divh = (int) (parms[6][0]+0.5);
    if (divw < 1 || divh < 1) return 0.0;
    const int sz = divw*divh*2;

#ifdef EEL_LICE_RAMFUNC
    EEL_F *d = EEL_LICE_RAMFUNC(opaque,7,sz);
    if (!d) return 0.0;
#else
    EEL_F **blocks = ctx->m_vmref ? ((compileContext*)ctx->m_vmref)->ram_state->blocks : 0;
    if (!blocks || np < 8) return 0.0;

    const int addr1= (int) (parms[7][0]+0.5);
    EEL_F *d=__NSEEL_RAMAlloc(blocks,addr1);
    if (sz>NSEEL_RAM_ITEMSPERBLOCK)
    {
      int x;
      for(x=NSEEL_RAM_ITEMSPERBLOCK;x<sz-1;x+=NSEEL_RAM_ITEMSPERBLOCK)
        if (__NSEEL_RAMAlloc(blocks,addr1+x) != d+x) return 0.0;
    }
    EEL_F *end=__NSEEL_RAMAlloc(blocks,addr1+sz-1);
    if (end != d+sz-1) return 0.0; // buffer not contiguous
#endif

    ctx->gfx_transformblit(parms,divw,divh,d);
#endif
  }
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_circle(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  bool aa = true, fill = false;
  if (np>3) fill = parms[3][0] > 0.5;
  if (np>4) aa = parms[4][0] > 0.5;
  if (ctx) ctx->gfx_circle((float)parms[0][0], (float)parms[1][0], (float)parms[2][0], fill, aa);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_triangle(void* opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_triangle(parms, (int)np);
  return 0.0;
}

static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_drawnumber(void *opaque, EEL_F *n, EEL_F *nd)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_drawnumber(*n, *nd);
  return n;
}

static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_drawchar(void *opaque, EEL_F *n)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_drawchar(*n);
  return n;
}

static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_measurestr(void *opaque, EEL_F *str, EEL_F *xOut, EEL_F *yOut)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) 
  {
    EEL_F *p[3]={str,xOut,yOut};
    ctx->gfx_drawstr(opaque,p,3,2);
  }
  return str;
}
static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_measurechar(void *opaque, EEL_F *str, EEL_F *xOut, EEL_F *yOut)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) 
  {
    EEL_F *p[3]={str,xOut,yOut};
    ctx->gfx_drawstr(opaque,p,3,3);
  }
  return str;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_drawstr(void *opaque, INT_PTR nparms, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_drawstr(opaque,parms,(int)nparms,0);
  return parms[0][0];
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_printf(void *opaque, INT_PTR nparms, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx && nparms>0) 
  {
    EEL_F v= **parms;
    ctx->gfx_drawstr(opaque,parms,(int)nparms,1);
    return v;
  }
  return 0.0;
}

static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_setpixel(void *opaque, EEL_F *r, EEL_F *g, EEL_F *b)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_setpixel(*r, *g, *b);
  return r;
}

static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_getpixel(void *opaque, EEL_F *r, EEL_F *g, EEL_F *b)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_getpixel(r, g, b);
  return r;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_setfont(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) return ctx->gfx_setfont(opaque,(int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_getfont(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) 
  {
    const int idx=ctx->m_gfx_font_active;
    if (idx>=0 && idx < ctx->m_gfx_fonts.GetSize())
    {
      eel_lice_state::gfxFontStruct* f=ctx->m_gfx_fonts.Get()+idx;

      EEL_STRING_MUTEXLOCK_SCOPE
    
#ifdef NOT_EEL_STRING_UPDATE_STRING
      NOT_EEL_STRING_UPDATE_STRING(parms[0][0],f->actual_fontname);
#else
      WDL_FastString *fs=NULL;
      EEL_STRING_GET_FOR_WRITE(parms[0][0],&fs);
      if (fs) fs->Set(f->actual_fontname);
#endif
    }
    return idx;
  }
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_blit2(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx && np>=3) 
  {
    ctx->gfx_blitext2((int)np,parms,0);
    return *(parms[0]);
  }
  return 0.0;
}

static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_blitext(void *opaque, EEL_F *img, EEL_F *coordidx, EEL_F *rotate)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) 
  {
#ifndef EEL_LICE_NO_RAM
#ifdef EEL_LICE_RAMFUNC
    EEL_F *buf = EEL_LICE_RAMFUNC(opaque,1,10);
    if (!buf) return img;
#else
    EEL_F fc = *coordidx;
    if (fc < -0.5 || fc >= NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK) return img;
    int a=(int)fc;
    if (a<0) return img;
        
    EEL_F buf[10];
    int x;
    EEL_F **blocks = ctx->m_vmref ? ((compileContext*)ctx->m_vmref)->ram_state->blocks : 0;
    if (!blocks) return img;
    for (x = 0;x < 10; x ++)
    {
      EEL_F *d=__NSEEL_RAMAlloc(blocks,a++);
      if (!d || d==&nseel_ramalloc_onfail) return img;
      buf[x]=*d;
    }
#endif
    // read megabuf
    ctx->gfx_blitext(*img,buf,*rotate);
#endif
  }
  return img;
}


static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_blurto(void *opaque, EEL_F *x, EEL_F *y)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_blurto(*x,*y);
  return x;
}

static EEL_F * NSEEL_CGEN_CALL ysfx_api_gfx_getimgdim(void *opaque, EEL_F *img, EEL_F *w, EEL_F *h)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_getimgdim(*img,w,h);
  return img;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_loadimg(void *opaque, EEL_F *img, EEL_F *fr)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) return ctx->gfx_loadimg(opaque,(int)*img,*fr);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_setimgdim(void *opaque, EEL_F *img, EEL_F *w, EEL_F *h)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) return ctx->gfx_setimgdim((int)*img,w,h);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_getsyscol(void* ctxe, INT_PTR np, EEL_F **parms)
{
  return (EEL_F)LICE_RGBA_FROMNATIVE(GetSysColor(COLOR_3DFACE));
}

void eel_lice_state::gfx_lineto(EEL_F xpos, EEL_F ypos, EEL_F aaflag)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_lineto");
  if (!dest) return;

  int x1=(int)floor(xpos),y1=(int)floor(ypos),x2=(int)floor(*m_gfx_x), y2=(int)floor(*m_gfx_y);
  if (LICE_FUNCTION_VALID(LICE__GetWidth) && LICE_FUNCTION_VALID(LICE__GetHeight) && LICE_FUNCTION_VALID(LICE_Line) && 
      LICE_FUNCTION_VALID(LICE_ClipLine) && 
      LICE_ClipLine(&x1,&y1,&x2,&y2,0,0,LICE__GetWidth(dest),LICE__GetHeight(dest))) 
  {
    SetImageDirty(dest);
    LICE_Line(dest,x1,y1,x2,y2,getCurColor(),(float) *m_gfx_a,getCurMode(),aaflag > 0.5);
  }
  *m_gfx_x = xpos;
  *m_gfx_y = ypos;
  
}

void eel_lice_state::gfx_circle(float x, float y, float r, bool fill, bool aaflag)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_circle");
  if (!dest) return;

  if (LICE_FUNCTION_VALID(LICE_Circle) && LICE_FUNCTION_VALID(LICE_FillCircle))
  {
    SetImageDirty(dest);
    if(fill)
      LICE_FillCircle(dest, x, y, r, getCurColor(), (float) *m_gfx_a, getCurMode(), aaflag);
    else
      LICE_Circle(dest, x, y, r, getCurColor(), (float) *m_gfx_a, getCurMode(), aaflag);
  }
}

void eel_lice_state::gfx_triangle(EEL_F** parms, int np)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest, "gfx_triangle");
  if (np >= 6)
  {
    np &= ~1;
    SetImageDirty(dest);
    if (np == 6)
    {        
      if (!LICE_FUNCTION_VALID(LICE_FillTriangle)) return;

      LICE_FillTriangle(dest, (int)parms[0][0], (int)parms[1][0], (int)parms[2][0], (int)parms[3][0], 
          (int)parms[4][0], (int)parms[5][0], getCurColor(), (float)*m_gfx_a, getCurMode());
    }
    else
    {
      if (!LICE_FUNCTION_VALID(LICE_FillConvexPolygon)) return;

      const int maxpt = 512;
      const int n = wdl_min(np/2, maxpt);
      int i, rdi=0;
      int x[maxpt], y[maxpt];
      for (i=0; i < n; i++)
      {
        x[i]=(int)parms[rdi++][0];
        y[i]=(int)parms[rdi++][0];
      }

      LICE_FillConvexPolygon(dest, x, y, n, getCurColor(), (float)*m_gfx_a, getCurMode());
    }
  }
}

void eel_lice_state::gfx_rectto(EEL_F xpos, EEL_F ypos)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_rectto");
  if (!dest) return;

  EEL_F x1=xpos,y1=ypos,x2=*m_gfx_x, y2=*m_gfx_y;
  if (x2<x1) { x1=x2; x2=xpos; }
  if (y2<y1) { y1=y2; y2=ypos; }

  if (LICE_FUNCTION_VALID(LICE_FillRect) && x2-x1 > 0.5 && y2-y1 > 0.5)
  {
    SetImageDirty(dest);
    LICE_FillRect(dest,(int)x1,(int)y1,(int)(x2-x1),(int)(y2-y1),getCurColor(),(float)*m_gfx_a,getCurMode());
  }
  *m_gfx_x = xpos;
  *m_gfx_y = ypos;
}


void eel_lice_state::gfx_line(int np, EEL_F **parms)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_line");
  if (!dest) return;

  int x1=(int)floor(parms[0][0]),y1=(int)floor(parms[1][0]),x2=(int)floor(parms[2][0]), y2=(int)floor(parms[3][0]);
  if (LICE_FUNCTION_VALID(LICE__GetWidth) && 
      LICE_FUNCTION_VALID(LICE__GetHeight) && 
      LICE_FUNCTION_VALID(LICE_Line) && 
      LICE_FUNCTION_VALID(LICE_ClipLine) && LICE_ClipLine(&x1,&y1,&x2,&y2,0,0,LICE__GetWidth(dest),LICE__GetHeight(dest))) 
  {
    SetImageDirty(dest);
    LICE_Line(dest,x1,y1,x2,y2,getCurColor(),(float)*m_gfx_a,getCurMode(),np< 5 || parms[4][0] > 0.5);
  } 
}

void eel_lice_state::gfx_rect(int np, EEL_F **parms)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_rect");
  if (!dest) return;

  int x1=(int)floor(parms[0][0]),y1=(int)floor(parms[1][0]),w=(int)floor(parms[2][0]),h=(int)floor(parms[3][0]);  
  int filled=(np < 5 || parms[4][0] > 0.5);

  if (LICE_FUNCTION_VALID(LICE_FillRect) && LICE_FUNCTION_VALID(LICE_DrawRect) && w>0 && h>0)
  {
    SetImageDirty(dest);
    if (filled) LICE_FillRect(dest,x1,y1,w,h,getCurColor(),(float)*m_gfx_a,getCurMode());
    else LICE_DrawRect(dest, x1, y1, w-1, h-1, getCurColor(), (float)*m_gfx_a, getCurMode());
  }
}

void eel_lice_state::gfx_roundrect(int np, EEL_F **parms)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_roundrect");
  if (!dest) return;

  const bool aa = np <= 5 || parms[5][0]>0.5;

  if (LICE_FUNCTION_VALID(LICE_RoundRect) && parms[2][0]>0 && parms[3][0]>0)
  {
    SetImageDirty(dest);
    LICE_RoundRect(dest, (float)parms[0][0], (float)parms[1][0], (float)parms[2][0], (float)parms[3][0], (int)parms[4][0], getCurColor(), (float)*m_gfx_a, getCurMode(), aa);
  }
}

void eel_lice_state::gfx_arc(int np, EEL_F **parms)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_arc");
  if (!dest) return;

  const bool aa = np <= 5 || parms[5][0]>0.5;

  if (LICE_FUNCTION_VALID(LICE_Arc))
  {
    SetImageDirty(dest);
    LICE_Arc(dest, (float)parms[0][0], (float)parms[1][0], (float)parms[2][0], (float)parms[3][0], (float)parms[4][0], getCurColor(), (float)*m_gfx_a, getCurMode(), aa);
  }
}

void eel_lice_state::gfx_grad_or_muladd_rect(int whichmode, int np, EEL_F **parms)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,whichmode==0?"gfx_gradrect":"gfx_muladdrect");
  if (!dest) return;

  const int x1=(int)floor(parms[0][0]),y1=(int)floor(parms[1][0]),w=(int)floor(parms[2][0]), h=(int)floor(parms[3][0]);

  if (w>0 && h>0)
  {
    SetImageDirty(dest);
    if (whichmode==0 && LICE_FUNCTION_VALID(LICE_GradRect) && np > 7)
    {
      LICE_GradRect(dest,x1,y1,w,h,(float)parms[4][0],(float)parms[5][0],(float)parms[6][0],(float)parms[7][0],
                                   np > 8 ? (float)parms[8][0]:0.0f, np > 9 ? (float)parms[9][0]:0.0f,  np > 10 ? (float)parms[10][0]:0.0f, np > 11 ? (float)parms[11][0]:0.0f,  
                                   np > 12 ? (float)parms[12][0]:0.0f, np > 13 ? (float)parms[13][0]:0.0f,  np > 14 ? (float)parms[14][0]:0.0f, np > 15 ? (float)parms[15][0]:0.0f,  
                                   getCurMode());
    }
    else if (whichmode==1 && LICE_FUNCTION_VALID(LICE_MultiplyAddRect) && np > 6)
    {
      const double sc = 255.0;
      LICE_MultiplyAddRect(dest,x1,y1,w,h,(float)parms[4][0],(float)parms[5][0],(float)parms[6][0],np>7 ? (float)parms[7][0]:1.0f,
        (float)(np > 8 ? sc*parms[8][0]:0.0), (float)(np > 9 ? sc*parms[9][0]:0.0),  (float)(np > 10 ? sc*parms[10][0]:0.0), (float)(np > 11 ? sc*parms[11][0]:0.0));
    }
  }
}



void eel_lice_state::gfx_setpixel(EEL_F r, EEL_F g, EEL_F b)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_setpixel");
  if (!dest) return;

  int red=(int) (r*255.0);
  int green=(int) (g*255.0);
  int blue=(int) (b*255.0);
  if (red<0) red=0;else if (red>255)red=255;
  if (green<0) green=0;else if (green>255)green=255;
  if (blue<0) blue=0; else if (blue>255) blue=255;

  if (LICE_FUNCTION_VALID(LICE_PutPixel)) 
  {
    SetImageDirty(dest);
    LICE_PutPixel(dest,(int)*m_gfx_x, (int)*m_gfx_y,LICE_RGBA(red,green,blue,255), (float)*m_gfx_a,getCurMode());
  }
}

void eel_lice_state::gfx_getimgdim(EEL_F img, EEL_F *w, EEL_F *h)
{
  *w=*h=0;
#ifdef DYNAMIC_LICE
  if (!LICE__GetWidth || !LICE__GetHeight) return;
#endif

  LICE_IBitmap *bm=GetImageForIndex(img,"gfx_getimgdim"); 
  if (bm)
  {
    *w=LICE__GetWidth(bm);
    *h=LICE__GetHeight(bm);
  }
}

EEL_F eel_lice_state::gfx_loadimg(void *opaque, int img, EEL_F loadFrom)
{
#ifdef DYNAMIC_LICE
  if (!__LICE_LoadImage || !LICE__Destroy) return 0.0;
#endif

  if (img >= 0 && img < m_gfx_images.GetSize()) 
  {
    WDL_FastString fs;
    bool ok = EEL_LICE_GET_FILENAME_FOR_STRING(loadFrom,&fs,0);

    if (ok && fs.GetLength())
    {
      LICE_IBitmap *bm = LICE_LoadImage(fs.Get(),NULL,false);
      if (bm)
      {
        LICE__Destroy(m_gfx_images.Get()[img]);
        m_gfx_images.Get()[img]=bm;
        return img;
      }
    }
  }
  return -1.0;

}

EEL_F eel_lice_state::gfx_setimgdim(int img, EEL_F *w, EEL_F *h)
{
  int rv=0;
#ifdef DYNAMIC_LICE
  if (!LICE__resize ||!LICE__GetWidth || !LICE__GetHeight||!__LICE_CreateBitmap) return 0.0;
#endif

  int use_w = (int)*w;
  int use_h = (int)*h;
  if (use_w<1 || use_h < 1) use_w=use_h=0;
  if (use_w > 8192) use_w=8192;
  if (use_h > 8192) use_h=8192;
  
  LICE_IBitmap *bm=NULL;
  if (img >= 0 && img < m_gfx_images.GetSize()) 
  {
    bm=m_gfx_images.Get()[img];  
    if (!bm) 
    {
      m_gfx_images.Get()[img] = bm = __LICE_CreateBitmap(1,use_w,use_h);
      rv=!!bm;
    }
    else 
    {
      rv=LICE__resize(bm,use_w,use_h);
    }
  }

  return rv?1.0:0.0;
}

void eel_lice_state::gfx_blurto(EEL_F x, EEL_F y)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_blurto");
  if (!dest
#ifdef DYNAMIC_LICE
    ||!LICE_Blur
#endif
    ) return;

  SetImageDirty(dest);
  
  int srcx = (int)x;
  int srcy = (int)y;
  int srcw=(int) (*m_gfx_x-x);
  int srch=(int) (*m_gfx_y-y);
  if (srch < 0) { srch=-srch; srcy = (int)*m_gfx_y; }
  if (srcw < 0) { srcw=-srcw; srcx = (int)*m_gfx_x; }
  LICE_Blur(dest,dest,srcx,srcy,srcx,srcy,srcw,srch);
  *m_gfx_x = x;
  *m_gfx_y = y;
}

static bool CoordsSrcDestOverlap(EEL_F *coords)
{
  if (coords[0]+coords[2] < coords[4]) return false;
  if (coords[0] > coords[4] + coords[6]) return false;
  if (coords[1]+coords[3] < coords[5]) return false;
  if (coords[1] > coords[5] + coords[7]) return false;
  return true;
}

void eel_lice_state::gfx_transformblit(EEL_F **parms, int div_w, int div_h, EEL_F *tab)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_transformblit");

  if (!dest
#ifdef DYNAMIC_LICE
    ||!LICE_ScaledBlit || !LICE_TransformBlit2 ||!LICE__GetWidth||!LICE__GetHeight
#endif 
    ) return;

  LICE_IBitmap *bm=GetImageForIndex(parms[0][0],"gfx_transformblit:src"); 
  if (!bm) return;

  const int bmw=LICE__GetWidth(bm);
  const int bmh=LICE__GetHeight(bm);
 
  const bool isFromFB = bm==m_framebuffer;

  SetImageDirty(dest);

  if (bm == dest)
  {
    if (!m_framebuffer_extra && LICE_FUNCTION_VALID(__LICE_CreateBitmap)) m_framebuffer_extra=__LICE_CreateBitmap(0,bmw,bmh);
    if (m_framebuffer_extra)
    {
    
      LICE__resize(bm=m_framebuffer_extra,bmw,bmh);
      LICE_ScaledBlit(bm,dest, // copy the entire image
        0,0,bmw,bmh,
        0.0f,0.0f,(float)bmw,(float)bmh,
        1.0f,LICE_BLIT_MODE_COPY);      
    }
  }
  LICE_TransformBlit2(dest,bm,(int)floor(parms[1][0]),(int)floor(parms[2][0]),(int)floor(parms[3][0]),(int)floor(parms[4][0]),tab,div_w,div_h, (float)*m_gfx_a,getCurModeForBlit(isFromFB));
}

EEL_F eel_lice_state::gfx_setfont(void *opaque, int np, EEL_F **parms)
{
  int a = np>0 ? ((int)floor(parms[0][0]))-1 : -1;

  if (a>=0 && a < m_gfx_fonts.GetSize())
  {
    gfxFontStruct *s = m_gfx_fonts.Get()+a;
    if (np>1 && LICE_FUNCTION_VALID(LICE_CreateFont) && LICE_FUNCTION_VALID(LICE__SetFromHFont))
    {
      const int sz=np>2 ? (int)parms[2][0] : 10;
      
      bool doCreate=false;
      int fontflag=0;
      if (!s->font) s->actual_fontname[0]=0;

      {
        EEL_STRING_MUTEXLOCK_SCOPE
      
        const char *face=EEL_STRING_GET_FOR_INDEX(parms[1][0],NULL);
        #ifdef EEL_STRING_DEBUGOUT
          if (!face) EEL_STRING_DEBUGOUT("gfx_setfont: invalid string identifier %f",parms[1][0]);
        #endif
        if (!face || !*face) face="Arial";

        {
          unsigned int c = np > 3 ? (unsigned int) parms[3][0] : 0;
          while (c)
          {
            switch (toupper(c&0xff))
            {
              case 'B': fontflag|=EELFONT_FLAG_BOLD; break;
              case 'I': fontflag|=EELFONT_FLAG_ITALIC; break;
              case 'U': fontflag|=EELFONT_FLAG_UNDERLINE; break;
              case 'R': fontflag|=16; break;   //LICE_FONT_FLAG_FX_BLUR
              case 'V': fontflag|=32; break;   //LICE_FONT_FLAG_FX_INVERT
              case 'M': fontflag|=64; break;   //LICE_FONT_FLAG_FX_MONO
              case 'S': fontflag|=128; break;  //LICE_FONT_FLAG_FX_SHADOW
              case 'O': fontflag|=256; break;  //LICE_FONT_FLAG_FX_OUTLINE
              case 'Z': fontflag|=1; break;    //LICE_FONT_FLAG_VERTICAL
              case 'Y': fontflag|=1|2; break;  //LICE_FONT_FLAG_VERTICAL|LICE_FONT_FLAG_VERTICAL_BOTTOMUP
            }
            c>>=8;
          }
        }
      

        if (fontflag != s->last_fontflag || sz!=s->last_fontsize || strncmp(s->last_fontname,face,sizeof(s->last_fontname)-1))
        {
          lstrcpyn_safe(s->last_fontname,face,sizeof(s->last_fontname));
          s->last_fontsize=sz;
          s->last_fontflag=fontflag;
          doCreate=1;
        }
      }

      if (doCreate)
      {
        s->actual_fontname[0]=0;
        if (!s->font) s->font=LICE_CreateFont();
        if (s->font)
        {
          const int fw = (fontflag&EELFONT_FLAG_BOLD) ? FW_BOLD : FW_NORMAL;
          const bool italic = !!(fontflag&EELFONT_FLAG_ITALIC);
          const bool underline = !!(fontflag&EELFONT_FLAG_UNDERLINE);
          HFONT hf=NULL;
#if defined(_WIN32) && !defined(WDL_NO_SUPPORT_UTF8)
          WCHAR wf[256];
          if (WDL_DetectUTF8(s->last_fontname)>0 &&
              GetVersion()<0x80000000 &&
              MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s->last_fontname,-1,wf,256))
          {
            hf = CreateFontW(sz,0,0,0,fw,italic,underline,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,wf);
          }
#endif
          if (!hf) hf = CreateFont(sz,0,0,0,fw,italic,underline,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,s->last_fontname);

          if (!hf)
          {
            s->use_fonth=0; // disable this font
          }
          else
          {
            TEXTMETRIC tm;
            tm.tmHeight = sz;

            if (!m_framebuffer && LICE_FUNCTION_VALID(__LICE_CreateBitmap)) m_framebuffer=__LICE_CreateBitmap(1,64,64);

            if (m_framebuffer && LICE_FUNCTION_VALID(LICE__GetDC))
            {
              HGDIOBJ oldFont = 0;
              HDC hdc=LICE__GetDC(m_framebuffer);
              if (hdc)
              {
                oldFont = SelectObject(hdc,hf);
                GetTextMetrics(hdc,&tm);

#if defined(_WIN32) && !defined(WDL_NO_SUPPORT_UTF8)
                if (GetVersion()<0x80000000 &&
                    GetTextFaceW(hdc,sizeof(wf)/sizeof(wf[0]),wf) &&
                    WideCharToMultiByte(CP_UTF8,0,wf,-1,s->actual_fontname,sizeof(s->actual_fontname),NULL,NULL))
                {
                  s->actual_fontname[sizeof(s->actual_fontname)-1]=0;
                }
                else
#endif
                  GetTextFace(hdc, sizeof(s->actual_fontname), s->actual_fontname);
                SelectObject(hdc,oldFont);
              }
            }

            s->use_fonth=wdl_max(tm.tmHeight,1);
            LICE__SetFromHFont(s->font,hf, (fontflag & ~EELFONT_FLAG_MASK) | 512 /*LICE_FONT_FLAG_OWNS_HFONT*/);
          }
        }
      }
    }

    
    if (s->font && s->use_fonth)
    {
      m_gfx_font_active=a;
      if (m_gfx_texth) *m_gfx_texth=s->use_fonth;
      return 1.0;
    }
    // try to init this font
  }
  #ifdef EEL_STRING_DEBUGOUT
  if (a >= m_gfx_fonts.GetSize()) EEL_STRING_DEBUGOUT("gfx_setfont: invalid font %d specified",a);
  #endif

  if (a<0||a>=m_gfx_fonts.GetSize()||!m_gfx_fonts.Get()[a].font)
  {
    m_gfx_font_active=-1;
    if (m_gfx_texth) *m_gfx_texth=8;
    return 1.0;
  }
  return 0.0;
}

void eel_lice_state::gfx_blitext2(int np, EEL_F **parms, int blitmode)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_blitext2");

  if (!dest
#ifdef DYNAMIC_LICE
    ||!LICE_ScaledBlit || !LICE_RotatedBlit||!LICE__GetWidth||!LICE__GetHeight
#endif 
    ) return;

  LICE_IBitmap *bm=GetImageForIndex(parms[0][0],"gfx_blitext2:src"); 
  if (!bm) return;

  const int bmw=LICE__GetWidth(bm);
  const int bmh=LICE__GetHeight(bm);
  
  // 0=img, 1=scale, 2=rotate
  double coords[8];
  const double sc = blitmode==0 && np > 1 ? parms[1][0] : 1.0,
            angle = blitmode==0 && np > 2 ? parms[2][0] : 0.0;
  if (blitmode==0)
  {
    parms+=2;
    np -= 2;
  }

  coords[0]=np > 1 ? parms[1][0] : 0.0f;
  coords[1]=np > 2 ? parms[2][0] : 0.0f;
  coords[2]=np > 3 ? parms[3][0] : bmw;
  coords[3]=np > 4 ? parms[4][0] : bmh;
  coords[4]=np > 5 ? parms[5][0] : *m_gfx_x;
  coords[5]=np > 6 ? parms[6][0] : *m_gfx_y;
  coords[6]=np > 7 ? parms[7][0] : coords[2]*sc;
  coords[7]=np > 8 ? parms[8][0] : coords[3]*sc;
 
  const bool isFromFB = bm == m_framebuffer;
  SetImageDirty(dest);
 
  if (bm == dest &&
      (blitmode != 0 || np > 1) && // legacy behavior to matech previous gfx_blit(3parm), do not use temp buffer
      CoordsSrcDestOverlap(coords))
  {
    if (!m_framebuffer_extra && LICE_FUNCTION_VALID(__LICE_CreateBitmap)) m_framebuffer_extra=__LICE_CreateBitmap(0,bmw,bmh);
    if (m_framebuffer_extra)
    {
    
      LICE__resize(bm=m_framebuffer_extra,bmw,bmh);
      LICE_ScaledBlit(bm,dest, // copy the source portion
        (int)coords[0],(int)coords[1],(int)coords[2],(int)coords[3],
        (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3],
        1.0f,LICE_BLIT_MODE_COPY);      
    }
  }
  
  if (blitmode==1)
  {
    if (LICE_FUNCTION_VALID(LICE_DeltaBlit))
      LICE_DeltaBlit(dest,bm,(int)coords[4],(int)coords[5],(int)coords[6],(int)coords[7],
                (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3],
                np > 9 ? (float)parms[9][0]:1.0f, // dsdx
                np > 10 ? (float)parms[10][0]:0.0f, // dtdx
                np > 11 ? (float)parms[11][0]:0.0f, // dsdy
                np > 12 ? (float)parms[12][0]:1.0f, // dtdy
                np > 13 ? (float)parms[13][0]:0.0f, // dsdxdy
                np > 14 ? (float)parms[14][0]:0.0f, // dtdxdy
                np <= 15 || parms[15][0] > 0.5, (float)*m_gfx_a,getCurModeForBlit(isFromFB));
  }
  else if (fabs(angle)>0.000000001)
  {
    LICE_RotatedBlit(dest,bm,(int)coords[4],(int)coords[5],(int)coords[6],(int)coords[7],
      (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3],
      (float)angle,true, (float)*m_gfx_a,getCurModeForBlit(isFromFB),
       np > 9 ? (float)parms[9][0] : 0.0f,
       np > 10 ? (float)parms[10][0] : 0.0f);
  }
  else
  {
    LICE_ScaledBlit(dest,bm,(int)coords[4],(int)coords[5],(int)coords[6],(int)coords[7],
      (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3], (float)*m_gfx_a,getCurModeForBlit(isFromFB));
  }
}

void eel_lice_state::gfx_blitext(EEL_F img, EEL_F *coords, EEL_F angle)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_blitext");

  if (!dest
#ifdef DYNAMIC_LICE
    ||!LICE_ScaledBlit || !LICE_RotatedBlit||!LICE__GetWidth||!LICE__GetHeight
#endif 
    ) return;

  LICE_IBitmap *bm=GetImageForIndex(img,"gfx_blitext:src");
  if (!bm) return;
  
  SetImageDirty(dest);
  const bool isFromFB = bm == m_framebuffer;
 
  int bmw=LICE__GetWidth(bm);
  int bmh=LICE__GetHeight(bm);
  
  if (bm == dest && CoordsSrcDestOverlap(coords))
  {
    if (!m_framebuffer_extra && LICE_FUNCTION_VALID(__LICE_CreateBitmap)) m_framebuffer_extra=__LICE_CreateBitmap(0,bmw,bmh);
    if ( m_framebuffer_extra)
    {
    
      LICE__resize(bm=m_framebuffer_extra,bmw,bmh);
      LICE_ScaledBlit(bm,dest, // copy the source portion
        (int)coords[0],(int)coords[1],(int)coords[2],(int)coords[3],
        (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3],
        1.0f,LICE_BLIT_MODE_COPY);      
    }
  }
  
  if (fabs(angle)>0.000000001)
  {
    LICE_RotatedBlit(dest,bm,(int)coords[4],(int)coords[5],(int)coords[6],(int)coords[7],
      (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3],(float)angle,
      true, (float)*m_gfx_a,getCurModeForBlit(isFromFB),
          (float)coords[8],(float)coords[9]);
  }
  else
  {
    LICE_ScaledBlit(dest,bm,(int)coords[4],(int)coords[5],(int)coords[6],(int)coords[7],
      (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3], (float)*m_gfx_a,getCurModeForBlit(isFromFB));
  }
}

void eel_lice_state::gfx_set(int np, EEL_F **parms)
{
  if (np < 1) return;
  if (m_gfx_r) *m_gfx_r = parms[0][0];
  if (m_gfx_g) *m_gfx_g = np > 1 ? parms[1][0] : parms[0][0];
  if (m_gfx_b) *m_gfx_b = np > 2 ? parms[2][0] : parms[0][0];
  if (m_gfx_a) *m_gfx_a = np > 3 ? parms[3][0] : 1.0;
  if (m_gfx_mode) *m_gfx_mode = np > 4 ? parms[4][0] : 0;
  if (np > 5 && m_gfx_dest) *m_gfx_dest = parms[5][0];
  if (m_gfx_a2) *m_gfx_a2 = np > 6 ? parms[6][0] : 1.0;
}

void eel_lice_state::gfx_getpixel(EEL_F *r, EEL_F *g, EEL_F *b)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_getpixel");
  if (!dest) return;

  int ret=LICE_FUNCTION_VALID(LICE_GetPixel)?LICE_GetPixel(dest,(int)*m_gfx_x, (int)*m_gfx_y):0;

  *r=LICE_GETR(ret)/255.0;
  *g=LICE_GETG(ret)/255.0;
  *b=LICE_GETB(ret)/255.0;

}


static int __drawTextWithFont(LICE_IBitmap *dest, const RECT *rect, LICE_IFont *font, const char *buf, int buflen, 
  int fg, int mode, float alpha, int flags, EEL_F *wantYoutput, EEL_F **measureOnly)
{
  if (font && LICE_FUNCTION_VALID(LICE__DrawText))
  {
    RECT tr=*rect;
    LICE__SetTextColor(font,fg);
    LICE__SetTextCombineMode(font,mode,alpha);

    int maxx=0;
    RECT r={0,0,tr.left,0};
    while (buflen>0)
    {
      int thislen = 0;
      while (thislen < buflen && buf[thislen] != '\n') thislen++;
      memset(&r,0,sizeof(r));
      int lineh = LICE__DrawText(font,dest,buf,thislen?thislen:1,&r,DT_SINGLELINE|DT_NOPREFIX|DT_CALCRECT);
      if (!measureOnly)
      {
        r.right += tr.left;
        lineh = LICE__DrawText(font,dest,buf,thislen?thislen:1,&tr,DT_SINGLELINE|DT_NOPREFIX|flags);
        if (wantYoutput) *wantYoutput = tr.top;
      }
      else
      {
        if (r.right > maxx) maxx=r.right;
      }
      tr.top += lineh;

      buflen -= thislen+1;
      buf += thislen+1;      
    }
    if (measureOnly) 
    {
      measureOnly[0][0] = maxx;
      measureOnly[1][0] = tr.top;
    }
    return r.right;
  }
  else
  { 
    int xpos=rect->left, ypos=rect->top;
    int x;
    int maxx=0,maxy=0;

    LICE_SubBitmap sbm(
#ifdef DYNAMIC_LICE
        (LICE_IBitmap_disabledAPI*)
#endif
        dest,rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top);

    if (!measureOnly)
    {
      if (!(flags & DT_NOCLIP))
      {
        if (rect->right <= rect->left || rect->bottom <= rect->top) return 0; // invalid clip rect hm

        xpos = ypos = 0;
        dest = &sbm;
      }
      if (flags & (DT_RIGHT|DT_BOTTOM|DT_CENTER|DT_VCENTER))
      {
        EEL_F w=0.0,h=0.0;
        EEL_F *mo[2] = { &w,&h};
        RECT tr={0,};
        __drawTextWithFont(dest,&tr,NULL,buf,buflen,0,0,0.0f,0,NULL,mo);

        if (flags & DT_RIGHT) xpos += (rect->right-rect->left) - (int)floor(w);
        else if (flags & DT_CENTER) xpos += (rect->right-rect->left)/2 - (int)floor(w*.5);

        if (flags & DT_BOTTOM) ypos += (rect->bottom-rect->top) - (int)floor(h);
        else if (flags & DT_VCENTER) ypos += (rect->bottom-rect->top)/2 - (int)floor(h*.5);
      }
    }
    const int sxpos = xpos;

    if (LICE_FUNCTION_VALID(LICE_DrawChar)) for(x=0;x<buflen;x++)
    {
      switch (buf[x])
      {
        case '\n': 
          ypos += 8; 
        case '\r': 
          xpos = sxpos; 
        break;
        case ' ': xpos += 8; break;
        case '\t': xpos += 8*5; break;
        default:
          if (!measureOnly) LICE_DrawChar(dest,xpos,ypos,buf[x], fg,alpha,mode);
          xpos += 8;
          if (xpos > maxx) maxx=xpos;
          maxy = ypos + 8;
        break;
      }
    }
    if (measureOnly)
    {
      measureOnly[0][0]=maxx;
      measureOnly[1][0]=maxy;
    }
    else
    {
      if (wantYoutput) *wantYoutput=ypos;
    }
    return xpos;
  }
}

void eel_lice_state::gfx_drawstr(void *opaque, EEL_F **parms, int nparms, int formatmode)// formatmode=1 for format, 2 for purely measure no format
{
  int nfmtparms = nparms-1;
  EEL_F **fmtparms = parms+1;
  const char *funcname =  formatmode==1?"gfx_printf":
                          formatmode==2?"gfx_measurestr":
                          formatmode==3?"gfx_measurechar" : "gfx_drawstr";

  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,funcname);
  if (!dest) return;

#ifdef DYNAMIC_LICE
  if (!LICE__GetWidth || !LICE__GetHeight) return;
#endif

  EEL_STRING_MUTEXLOCK_SCOPE

  WDL_FastString *fs=NULL;
  char buf[4096];
  int s_len=0;

  const char *s;
  if (formatmode==3) 
  {
    s_len = WDL_MakeUTFChar(buf, (int)parms[0][0], sizeof(buf));
    s=buf;
  }
  else 
  {
    s=EEL_STRING_GET_FOR_INDEX(parms[0][0],&fs);
    #ifdef EEL_STRING_DEBUGOUT
      if (!s) EEL_STRING_DEBUGOUT("gfx_%s: invalid string identifier %f",funcname,parms[0][0]);
    #endif
    if (!s) 
    {
      s="<bad string>";
      s_len = 12;
    }
    else if (formatmode==1)
    {
      extern int eel_format_strings(void *, const char *s, const char *ep, char *, int, int, EEL_F **);
      s_len = eel_format_strings(opaque,s,fs?(s+fs->GetLength()):NULL,buf,sizeof(buf),nfmtparms,fmtparms);
      if (s_len<1) return;
      s=buf;
    }
    else 
    {
      s_len = fs?fs->GetLength():(int)strlen(s);
    }
  }

  if (s_len)
  {
    SetImageDirty(dest);
    if (formatmode>=2)
    {
      if (nfmtparms==2)
      {
        RECT r={0,0,0,0};
        __drawTextWithFont(dest,&r,GetActiveFont(),s,s_len,
          getCurColor(),getCurMode(),(float)*m_gfx_a,0,NULL,fmtparms);
      }
    }
    else
    {    
      RECT r={(int)floor(*m_gfx_x),(int)floor(*m_gfx_y),0,0};
      int flags=DT_NOCLIP;
      if (formatmode == 0 && nparms >= 4)
      {
        flags=(int)*parms[1];
        flags &= (DT_CENTER|DT_RIGHT|DT_VCENTER|DT_BOTTOM|DT_NOCLIP);
        r.right=(int)*parms[2];
        r.bottom=(int)*parms[3];
      }
      *m_gfx_x=__drawTextWithFont(dest,&r,GetActiveFont(),s,s_len,
        getCurColor(),getCurMode(),(float)*m_gfx_a,flags,m_gfx_y,NULL);
    }
  }
}

void eel_lice_state::gfx_drawchar(EEL_F ch)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_drawchar");
  if (!dest) return;

  SetImageDirty(dest);

  int a=(int)(ch+0.5);
  if (a == '\r' || a=='\n') a=' ';

  char buf[32];
  const int buflen = WDL_MakeUTFChar(buf, a, sizeof(buf));

  RECT r={(int)floor(*m_gfx_x),(int)floor(*m_gfx_y),0,0};
  *m_gfx_x = __drawTextWithFont(dest,&r,
                         GetActiveFont(),buf,buflen,
                         getCurColor(),getCurMode(),(float)*m_gfx_a,DT_NOCLIP,NULL,NULL);

}


void eel_lice_state::gfx_drawnumber(EEL_F n, EEL_F ndigits)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_drawnumber");
  if (!dest) return;

  SetImageDirty(dest);

  char buf[512];
  int a=(int)(ndigits+0.5);
  if (a <0)a=0;
  else if (a > 16) a=16;
  snprintf(buf,sizeof(buf),"%.*f",a,n);

  RECT r={(int)floor(*m_gfx_x),(int)floor(*m_gfx_y),0,0};
  *m_gfx_x = __drawTextWithFont(dest,&r,
                           GetActiveFont(),buf,(int)strlen(buf),
                           getCurColor(),getCurMode(),(float)*m_gfx_a,DT_NOCLIP,NULL,NULL);
}
