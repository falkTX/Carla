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

#define STBIW_WINDOWS_UTF8
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION

#if defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "stb_image_write.h"
#if defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

//------------------------------------------------------------------------------
#define WDL_NO_DEFINE_MINMAX
#include "WDL/lice/lice.h"
#include "WDL/wdltypes.h"
#include <memory>

static std::unique_ptr<unsigned char[]> bmp_to_stbi(LICE_IBitmap *bmp, unsigned ch)
{
    unsigned w = (unsigned)bmp->getWidth();
    unsigned h = (unsigned)bmp->getHeight();
    unsigned srcspan = (unsigned)bmp->getRowSpan();
    bool srcflip = bmp->isFlipped();
    LICE_pixel *srcpx = bmp->getBits();
    std::unique_ptr<unsigned char[]> result;

    if (ch == 4) {
        unsigned char *dstpx = new unsigned char[4 * w * h];
        result.reset(dstpx);
        for (unsigned row = 0; row < h; ++row) {
            LICE_pixel *src = srcpx + srcspan * (srcflip ? (h - 1 - row) : row);
            unsigned char *dst = dstpx + row * (4 * w);
            for (unsigned col = 0; col < w; ++col, ++src, dst += 4) {
                LICE_pixel px = *src;
                dst[0] = LICE_GETR(px);
                dst[1] = LICE_GETG(px);
                dst[2] = LICE_GETB(px);
                dst[3] = LICE_GETA(px);
            }
        }
    }
    else if (ch == 3) {
        unsigned char *dstpx = new unsigned char[3 * w * h];
        result.reset(dstpx);
        for (unsigned row = 0; row < h; ++row) {
            LICE_pixel *src = srcpx + srcspan * (srcflip ? (h - 1 - row) : row);
            unsigned char *dst = dstpx + row * (3 * w);
            for (unsigned col = 0; col < w; ++col, ++src, dst += 3) {
                LICE_pixel px = *src;
                dst[0] = LICE_GETR(px);
                dst[1] = LICE_GETG(px);
                dst[2] = LICE_GETB(px);
            }
        }
    }

    return result;
}

bool LICE_WritePNG(const char *filename, LICE_IBitmap *bmp, bool wantalpha)
{
    unsigned ch = wantalpha ? 4 : 3;
    std::unique_ptr<unsigned char[]> data = bmp_to_stbi(bmp, ch);

    if (!data)
        return false;

    return stbi_write_png(filename, bmp->getWidth(), bmp->getHeight(), (int)ch, data.get(), 0);
}

bool LICE_WriteJPG(const char *filename, LICE_IBitmap *bmp, int quality, bool force_baseline)
{
    (void)force_baseline; // always baseline

    unsigned ch = 4;
    std::unique_ptr<unsigned char[]> data = bmp_to_stbi(bmp, ch);

    if (!data)
        return false;

    return stbi_write_jpg(filename, bmp->getWidth(), bmp->getHeight(), (int)ch, data.get(), quality);
}
