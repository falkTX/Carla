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

#include "jsusfx.h"
#include "jsusfx_gfx.h"
#include "WDL/eel2/ns-eel-int.h"

#define EEL_GFX_GET_INTERFACE(opaque) ((opaque) ? (((JsusFx*)opaque)->gfx) : nullptr)

#define REAPER_GET_INTERFACE(opaque) (*(JsusFx*)opaque)

static EEL_F * NSEEL_CGEN_CALL _gfx_lineto(void *opaque, EEL_F *xpos, EEL_F *ypos, EEL_F *useaa)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_lineto(*xpos, *ypos, *useaa);
  return xpos;
}
static EEL_F * NSEEL_CGEN_CALL _gfx_lineto2(void *opaque, EEL_F *xpos, EEL_F *ypos)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_lineto(*xpos, *ypos, 1.0f);
  return xpos;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_rectto(void *opaque, EEL_F *xpos, EEL_F *ypos)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_rectto(*xpos, *ypos);
  return xpos;
}

static EEL_F NSEEL_CGEN_CALL _gfx_line(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_line((int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_rect(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_rect((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL _gfx_roundrect(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_roundrect((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL _gfx_arc(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_arc((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL _gfx_set(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_set((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL _gfx_gradrect(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_grad_or_muladd_rect(0,(int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_muladdrect(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_grad_or_muladd_rect(1,(int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_deltablit(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_blitext2((int)np,parms,1);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_transformblit(void *opaque, INT_PTR np, EEL_F **parms)
{
#if 0 // todo
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
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
    EEL_F **blocks = ctx->m_vmref  ? ((compileContext*)ctx->m_vmref)->ram_state.blocks : 0;
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
#endif
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_circle(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  bool aa = true, fill = false;
  if (np>3) fill = parms[3][0] > 0.5;
  if (np>4) aa = parms[4][0] > 0.5;
  if (ctx) ctx->gfx_circle((float)parms[0][0], (float)parms[1][0], (float)parms[2][0], fill, aa);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_triangle(void* opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_triangle(parms, np);
  return 0.0;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_drawnumber(void *opaque, EEL_F *n, EEL_F *nd)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_drawnumber(*n, *nd);
  return n;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_drawchar(void *opaque, EEL_F *n)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_drawchar(*n);
  return n;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_measurestr(void *opaque, EEL_F *str, EEL_F *xOut, EEL_F *yOut)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx)
  {
    EEL_F *p[3]={str,xOut,yOut};
    ctx->gfx_drawstr(opaque,p,3,2);
  }
  return str;
}
static EEL_F * NSEEL_CGEN_CALL _gfx_measurechar(void *opaque, EEL_F *str, EEL_F *xOut, EEL_F *yOut)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx)
  {
    EEL_F *p[3]={str,xOut,yOut};
    ctx->gfx_drawstr(opaque,p,3,3);
  }
  return str;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_drawstr(void *opaque, EEL_F *n)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_drawstr(opaque,&n,1,0);
  return n;
}
static EEL_F NSEEL_CGEN_CALL _gfx_printf(void *opaque, INT_PTR nparms, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx && nparms>0)
  {
    EEL_F v= **parms;
    ctx->gfx_drawstr(opaque,parms,nparms,1);
    return v;
  }
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_showmenu(void* opaque, INT_PTR nparms, EEL_F **parms)
{
  JsusFxGfx* ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) return ctx->gfx_showmenu(parms, (int)nparms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_setcursor(void* opaque,  INT_PTR nparms, EEL_F **parms)
{
  JsusFxGfx* ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) return ctx->gfx_setcursor(parms, (int)nparms);
  return 0.0;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_setpixel(void *opaque, EEL_F *r, EEL_F *g, EEL_F *b)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_setpixel(*r, *g, *b);
  return r;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_getpixel(void *opaque, EEL_F *r, EEL_F *g, EEL_F *b)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_getpixel(r, g, b);
  return r;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_blit(void *opaque, EEL_F *img, EEL_F *scale, EEL_F *rotate)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_blit(*img,*scale,*rotate);
  return img;
}

static EEL_F NSEEL_CGEN_CALL _gfx_setfont(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) return ctx->gfx_setfont((int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_getfont(void *opaque, INT_PTR np, EEL_F **parms)
{
  // todo : implement _gfx_getfont
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_blit2(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx && np>=3)
  {
    ctx->gfx_blitext2((int)np,parms,0);
    return *(parms[0]);
  }
  return 0.0;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_blitext(void *opaque, EEL_F *img, EEL_F *coordidx, EEL_F *rotate)
{
#if 0 // todo
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
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
    EEL_F **blocks = ctx->m_vmref  ? ((compileContext*)ctx->m_vmref)->ram_state.blocks : 0;
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
#endif
  return img;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_blurto(void *opaque, EEL_F *x, EEL_F *y)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_blurto(*x,*y);
  return x;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_getimgdim(void *opaque, EEL_F *img, EEL_F *w, EEL_F *h)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) ctx->gfx_getimgdim(*img,w,h);
  return img;
}

static EEL_F NSEEL_CGEN_CALL _gfx_loadimg(void *opaque, EEL_F *img, EEL_F *fr)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
  if (ctx) return ctx->gfx_loadimg(jsusFx,(int)*img,*fr);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_setimgdim(void *opaque, EEL_F *img, EEL_F *w, EEL_F *h)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) return ctx->gfx_setimgdim((int)*img,w,h);
  return 0.0;
}

//

static EEL_F NSEEL_CGEN_CALL _gfx_getchar(void *opaque, EEL_F *p)
{
  JsusFxGfx *ctx=EEL_GFX_GET_INTERFACE(opaque);
  if (ctx) return ctx->gfx_getchar(*p);
  return 0.0;
}

//

// todo : remove __stub
static EEL_F NSEEL_CGEN_CALL __stub(void *opaque, INT_PTR np, EEL_F **parms)
{
  return 0.0;
}

void JsusFxGfx::init(NSEEL_VMCTX vm) {
	m_gfx_r = NSEEL_VM_regvar(vm,"gfx_r");
	m_gfx_g = NSEEL_VM_regvar(vm,"gfx_g");
	m_gfx_b = NSEEL_VM_regvar(vm,"gfx_b");
	m_gfx_a = NSEEL_VM_regvar(vm,"gfx_a");

	m_gfx_w = NSEEL_VM_regvar(vm,"gfx_w");
	m_gfx_h = NSEEL_VM_regvar(vm,"gfx_h");
	
	m_gfx_x = NSEEL_VM_regvar(vm,"gfx_x");
	m_gfx_y = NSEEL_VM_regvar(vm,"gfx_y");
	
	m_gfx_mode = NSEEL_VM_regvar(vm,"gfx_mode");
	m_gfx_clear = NSEEL_VM_regvar(vm,"gfx_clear");
	m_gfx_texth = NSEEL_VM_regvar(vm,"gfx_texth");
	m_gfx_dest = NSEEL_VM_regvar(vm,"gfx_dest");
	
	m_mouse_x = NSEEL_VM_regvar(vm,"mouse_x");
	m_mouse_y = NSEEL_VM_regvar(vm,"mouse_y");
	m_mouse_cap = NSEEL_VM_regvar(vm,"mouse_cap");
	m_mouse_wheel = NSEEL_VM_regvar(vm,"mouse_wheel");
	
	// LICE
	NSEEL_addfunc_retptr("gfx_lineto",3,NSEEL_PProc_THIS,&_gfx_lineto);
	NSEEL_addfunc_retptr("gfx_lineto",2,NSEEL_PProc_THIS,&_gfx_lineto2);
	NSEEL_addfunc_retptr("gfx_rectto",2,NSEEL_PProc_THIS,&_gfx_rectto);
	NSEEL_addfunc_varparm("gfx_rect",4,NSEEL_PProc_THIS,&_gfx_rect);
	NSEEL_addfunc_varparm("gfx_line",4,NSEEL_PProc_THIS,&_gfx_line); // 5th param is optionally AA
	NSEEL_addfunc_varparm("gfx_gradrect",8,NSEEL_PProc_THIS,&_gfx_gradrect);
	NSEEL_addfunc_varparm("gfx_muladdrect",7,NSEEL_PProc_THIS,&_gfx_muladdrect);
	NSEEL_addfunc_varparm("gfx_deltablit",9,NSEEL_PProc_THIS,&_gfx_deltablit);
	NSEEL_addfunc_exparms("gfx_transformblit",8,NSEEL_PProc_THIS,&_gfx_transformblit);
	NSEEL_addfunc_varparm("gfx_circle",3,NSEEL_PProc_THIS,&_gfx_circle);
	NSEEL_addfunc_varparm("gfx_triangle", 6, NSEEL_PProc_THIS, &_gfx_triangle);
	NSEEL_addfunc_varparm("gfx_roundrect",5,NSEEL_PProc_THIS,&_gfx_roundrect);
	NSEEL_addfunc_varparm("gfx_arc",5,NSEEL_PProc_THIS,&_gfx_arc);
	NSEEL_addfunc_retptr("gfx_blurto",2,NSEEL_PProc_THIS,&_gfx_blurto);
	NSEEL_addfunc_exparms("gfx_showmenu",1,NSEEL_PProc_THIS,&_gfx_showmenu);
	NSEEL_addfunc_varparm("gfx_setcursor",1, NSEEL_PProc_THIS, &_gfx_setcursor);
	NSEEL_addfunc_retptr("gfx_drawnumber",2,NSEEL_PProc_THIS,&_gfx_drawnumber);
	NSEEL_addfunc_retptr("gfx_drawchar",1,NSEEL_PProc_THIS,&_gfx_drawchar);
	NSEEL_addfunc_retptr("gfx_drawstr",1,NSEEL_PProc_THIS,&_gfx_drawstr);
	NSEEL_addfunc_retptr("gfx_measurestr",3,NSEEL_PProc_THIS,&_gfx_measurestr);
	NSEEL_addfunc_retptr("gfx_measurechar",3,NSEEL_PProc_THIS,&_gfx_measurechar);
	NSEEL_addfunc_varparm("gfx_printf",1,NSEEL_PProc_THIS,&_gfx_printf);
	NSEEL_addfunc_retptr("gfx_setpixel",3,NSEEL_PProc_THIS,&_gfx_setpixel);
	NSEEL_addfunc_retptr("gfx_getpixel",3,NSEEL_PProc_THIS,&_gfx_getpixel);
	NSEEL_addfunc_retptr("gfx_getimgdim",3,NSEEL_PProc_THIS,&_gfx_getimgdim);
	NSEEL_addfunc_retval("gfx_setimgdim",3,NSEEL_PProc_THIS,&_gfx_setimgdim);
	NSEEL_addfunc_retval("gfx_loadimg",2,NSEEL_PProc_THIS,&_gfx_loadimg);
	NSEEL_addfunc_retptr("gfx_blit",3,NSEEL_PProc_THIS,&_gfx_blit);
	NSEEL_addfunc_retptr("gfx_blitext",3,NSEEL_PProc_THIS,&_gfx_blitext);
	NSEEL_addfunc_varparm("gfx_blit",4,NSEEL_PProc_THIS,&_gfx_blit2);
	NSEEL_addfunc_varparm("gfx_setfont",1,NSEEL_PProc_THIS,&_gfx_setfont);
	NSEEL_addfunc_varparm("gfx_getfont",1,NSEEL_PProc_THIS,&_gfx_getfont);
	NSEEL_addfunc_varparm("gfx_set",1,NSEEL_PProc_THIS,&_gfx_set);
	
	NSEEL_addfunc_retval("gfx_getchar",1,NSEEL_PProc_THIS,&_gfx_getchar);
}
