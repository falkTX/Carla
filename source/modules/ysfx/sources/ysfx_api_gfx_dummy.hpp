// Copyright 2021 Jean Pierre Cimalando
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once
#include "ysfx_eel_utils.hpp"

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_stub_varparm(void *, INT_PTR, EEL_F **)
{
    return 0;
}

static EEL_F *NSEEL_CGEN_CALL ysfx_api_gfx_stub_retptr1(void *, EEL_F *arg)
{
  return arg;
}

static EEL_F *NSEEL_CGEN_CALL ysfx_api_gfx_stub_retptr2(void *, EEL_F *arg, EEL_F *)
{
  return arg;
}

static EEL_F *NSEEL_CGEN_CALL ysfx_api_gfx_stub_retptr3(void *, EEL_F *arg, EEL_F *, EEL_F *)
{
  return arg;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_stub_retval1(void *, EEL_F *)
{
  return 0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_stub_retval2(void *, EEL_F *, EEL_F *)
{
  return 0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_gfx_stub_retval3(void *, EEL_F *, EEL_F *, EEL_F *)
{
  return 0;
}

#define ysfx_api_gfx_lineto ysfx_api_gfx_stub_retptr3
#define ysfx_api_gfx_lineto2 ysfx_api_gfx_stub_retptr2
#define ysfx_api_gfx_rectto ysfx_api_gfx_stub_retptr2
#define ysfx_api_gfx_rect ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_line ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_gradrect ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_muladdrect ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_deltablit ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_transformblit ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_circle ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_triangle ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_roundrect ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_arc ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_blurto ysfx_api_gfx_stub_retptr2
#define ysfx_api_gfx_showmenu ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_setcursor ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_drawnumber ysfx_api_gfx_stub_retptr2
#define ysfx_api_gfx_drawchar ysfx_api_gfx_stub_retptr1
#define ysfx_api_gfx_drawstr ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_measurestr ysfx_api_gfx_stub_retptr3
#define ysfx_api_gfx_measurechar ysfx_api_gfx_stub_retptr3
#define ysfx_api_gfx_printf ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_setpixel ysfx_api_gfx_stub_retptr3
#define ysfx_api_gfx_getpixel ysfx_api_gfx_stub_retptr3
#define ysfx_api_gfx_getimgdim ysfx_api_gfx_stub_retptr3
#define ysfx_api_gfx_setimgdim ysfx_api_gfx_stub_retval3
#define ysfx_api_gfx_loadimg ysfx_api_gfx_stub_retval2
#define ysfx_api_gfx_blit ysfx_api_gfx_stub_retptr3
#define ysfx_api_gfx_blitext ysfx_api_gfx_stub_retptr3
#define ysfx_api_gfx_blit2 ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_setfont ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_getfont ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_set ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_getdropfile ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_getsyscol ysfx_api_gfx_stub_varparm
#define ysfx_api_gfx_getchar ysfx_api_gfx_stub_retval1
