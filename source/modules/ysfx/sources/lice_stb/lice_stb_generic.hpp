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

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_WINDOWS_UTF8

#if defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#if defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

//------------------------------------------------------------------------------
#include "WDL/lice/lice.h"
#include "WDL/wdltypes.h"

static LICE_IBitmap *LICE_LoadSTB(const char *filename, LICE_IBitmap *bmp)
{
    LICE_IBitmap *delbmp = nullptr;
    stbi_uc *srcpx = nullptr;
    LICE_pixel *dstpx = nullptr;
    bool dstflip = false;
    unsigned dstspan = 0;
    unsigned w = 0;
    unsigned h = 0;
    unsigned ch = 0;

    srcpx = stbi_load(filename, (int *)&w, (int *)&h, (int *)&ch, 4);
    if (!srcpx)
        goto fail;

    if (bmp)
        bmp->resize(w, h);
    else
        bmp = delbmp = new WDL_NEW LICE_MemBitmap(w, h);

    if (!bmp || (unsigned)bmp->getWidth() != w || (unsigned)bmp->getHeight() != h)
        goto fail;

    dstpx = bmp->getBits();
    dstflip = bmp->isFlipped();
    dstspan = bmp->getRowSpan();

    for (unsigned row = 0; row < h; ++row) {
        const stbi_uc *src = srcpx + row * (4 * w);
        LICE_pixel *dst = dstpx + dstspan * (dstflip ? (h - 1 - row) : row);
        for (unsigned col = 0; col < w; ++col, src += 4, ++dst)
            *dst = LICE_RGBA(src[0], src[1], src[2], src[3]);
    }

    stbi_image_free(srcpx);
    return bmp;

fail:
    delete delbmp;
    stbi_image_free(srcpx);
    return nullptr;
}
