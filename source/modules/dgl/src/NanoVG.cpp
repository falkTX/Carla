/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "../NanoVG.hpp"
#include "../Window.hpp"

// -----------------------------------------------------------------------

#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg/nanovg_gl.h"

#if defined(NANOVG_GL2)
# define nvgCreateGL nvgCreateGL2
# define nvgDeleteGL nvgDeleteGL2
#elif defined(NANOVG_GL3)
# define nvgCreateGL nvgCreateGL3
# define nvgDeleteGL nvgDeleteGL3
#elif defined(NANOVG_GLES2)
# define nvgCreateGL nvgCreateGLES2
# define nvgDeleteGL nvgDeleteGLES2
#elif defined(NANOVG_GLES3)
# define nvgCreateGL nvgCreateGLES3
# define nvgDeleteGL nvgDeleteGLES3
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Paint

NanoVG::Paint::Paint() noexcept
    : radius(0.0f), feather(0.0f), innerColor(), outerColor(), imageId(0), repeat(REPEAT_NONE)
{
    std::memset(xform, 0, sizeof(float)*6);
    std::memset(extent, 0, sizeof(float)*2);
}

NanoVG::Paint::Paint(const NVGpaint& p) noexcept
    : radius(p.radius), feather(p.feather), innerColor(p.innerColor), outerColor(p.outerColor), imageId(p.image), repeat(static_cast<PatternRepeat>(p.repeat))
{
    std::memcpy(xform, p.xform, sizeof(float)*6);
    std::memcpy(extent, p.extent, sizeof(float)*2);
}

NanoVG::Paint::operator NVGpaint() const noexcept
{
    NVGpaint p;
    p.radius = radius;
    p.feather = feather;
    p.innerColor = innerColor;
    p.outerColor = outerColor;
    p.image = imageId;
    p.repeat = repeat;
    std::memcpy(p.xform, xform, sizeof(float)*6);
    std::memcpy(p.extent, extent, sizeof(float)*2);
    return p;
}

// -----------------------------------------------------------------------
// NanoImage

NanoImage::NanoImage(NVGcontext* const context, const int imageId) noexcept
    : fContext(context),
      fImageId(imageId),
      fSize()
{
    _updateSize();
}

NanoImage::~NanoImage()
{
    if (fContext != nullptr && fImageId != 0)
        nvgDeleteImage(fContext, fImageId);
}

Size<uint> NanoImage::getSize() const noexcept
{
    return fSize;
}

void NanoImage::updateImage(const uchar* const data)
{
    DISTRHO_SAFE_ASSERT_RETURN(data != nullptr,);

    if (fContext != nullptr && fImageId != 0)
    {
        nvgUpdateImage(fContext, fImageId, data);
        _updateSize();
    }
}

void NanoImage::_updateSize()
{
    int w=0, h=0;

    if (fContext != nullptr && fImageId != 0)
    {
        nvgImageSize(fContext, fImageId, &w, &h);

        if (w < 0) w = 0;
        if (h < 0) h = 0;
    }

    fSize.setSize(w, h);
}

// -----------------------------------------------------------------------
// NanoVG

NanoVG::NanoVG()
    : fContext(nvgCreateGL(512, 512, NVG_ANTIALIAS)),
      fInFrame(false)
{
    DISTRHO_SAFE_ASSERT_RETURN(fContext != nullptr,);
}

NanoVG::NanoVG(const int textAtlasWidth, const int textAtlasHeight)
    : fContext(nvgCreateGL(textAtlasWidth, textAtlasHeight, NVG_ANTIALIAS)),
      fInFrame(false)
{
    DISTRHO_SAFE_ASSERT_RETURN(fContext != nullptr,);
}

NanoVG::~NanoVG()
{
    DISTRHO_SAFE_ASSERT(! fInFrame);

    if (fContext != nullptr)
        nvgDeleteGL(fContext);
}

// -----------------------------------------------------------------------

void NanoVG::beginFrame(const uint width, const uint height, const float scaleFactor, const Alpha alpha)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(scaleFactor > 0.0f,);
    DISTRHO_SAFE_ASSERT_RETURN(! fInFrame,);

    fInFrame = true;
    nvgBeginFrame(fContext, width, height, scaleFactor, static_cast<NVGalpha>(alpha));
}

void NanoVG::beginFrame(Widget* const widget)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(widget != nullptr,);
    DISTRHO_SAFE_ASSERT_RETURN(! fInFrame,);

    Window& window(widget->getParentWindow());

    fInFrame = true;
    nvgBeginFrame(fContext, window.getWidth(), window.getHeight(), 1.0f, NVG_PREMULTIPLIED_ALPHA);
}

void NanoVG::endFrame()
{
    DISTRHO_SAFE_ASSERT_RETURN(fInFrame,);

    if (fContext != nullptr)
        nvgEndFrame(fContext);

    fInFrame = false;
}

// -----------------------------------------------------------------------
// State Handling

void NanoVG::save()
{
    if (fContext != nullptr)
        nvgSave(fContext);
}

void NanoVG::restore()
{
    if (fContext != nullptr)
        nvgRestore(fContext);
}

void NanoVG::reset()
{
    if (fContext != nullptr)
        nvgReset(fContext);
}

// -----------------------------------------------------------------------
// Render styles

void NanoVG::strokeColor(const Color& color)
{
    if (fContext != nullptr)
        nvgStrokeColor(fContext, color);
}

void NanoVG::strokeColor(const int red, const int green, const int blue, const int alpha)
{
    if (fContext != nullptr)
        nvgStrokeColor(fContext, nvgRGBA(red, green, blue, alpha));
}

void NanoVG::strokeColor(const float red, const float green, const float blue, const float alpha)
{
    if (fContext != nullptr)
        nvgStrokeColor(fContext, nvgRGBAf(red, green, blue, alpha));
}

void NanoVG::strokePaint(const Paint& paint)
{
    if (fContext != nullptr)
        nvgStrokePaint(fContext, paint);
}

void NanoVG::fillColor(const Color& color)
{
    if (fContext != nullptr)
        nvgFillColor(fContext, color);
}

void NanoVG::fillColor(const int red, const int green, const int blue, const int alpha)
{
    if (fContext != nullptr)
        nvgFillColor(fContext, nvgRGBA(red, green, blue, alpha));
}

void NanoVG::fillColor(const float red, const float green, const float blue, const float alpha)
{
    if (fContext != nullptr)
        nvgFillColor(fContext, nvgRGBAf(red, green, blue, alpha));
}

void NanoVG::fillPaint(const Paint& paint)
{
    if (fContext != nullptr)
        nvgFillPaint(fContext, paint);
}

void NanoVG::miterLimit(float limit)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(limit > 0.0f,);

    nvgMiterLimit(fContext, limit);
}

void NanoVG::strokeWidth(float size)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(size > 0.0f,);

    nvgStrokeWidth(fContext, size);
}

void NanoVG::lineCap(NanoVG::LineCap cap)
{
    if (fContext != nullptr)
        nvgLineCap(fContext, cap);
}

void NanoVG::lineJoin(NanoVG::LineCap join)
{
    if (fContext != nullptr)
        nvgLineJoin(fContext, join);
}

// -----------------------------------------------------------------------
// Transforms

void NanoVG::resetTransform()
{
    if (fContext != nullptr)
        nvgResetTransform(fContext);
}

void NanoVG::transform(float a, float b, float c, float d, float e, float f)
{
    if (fContext != nullptr)
        nvgTransform(fContext, a, b, c, d, e, f);
}

void NanoVG::translate(float x, float y)
{
    if (fContext != nullptr)
        nvgTranslate(fContext, x, y);
}

void NanoVG::rotate(float angle)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(angle > 0.0f,);

    nvgRotate(fContext, angle);
}

void NanoVG::skewX(float angle)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(angle > 0.0f,);

    nvgSkewX(fContext, angle);
}

void NanoVG::skewY(float angle)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(angle > 0.0f,);

    nvgSkewY(fContext, angle);
}

void NanoVG::scale(float x, float y)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(x > 0.0f,);
    DISTRHO_SAFE_ASSERT_RETURN(y > 0.0f,);

    nvgScale(fContext, x, y);
}

void NanoVG::currentTransform(float xform[6])
{
    if (fContext != nullptr)
        nvgCurrentTransform(fContext, xform);
}

void NanoVG::transformIdentity(float dst[6])
{
    nvgTransformIdentity(dst);
}

void NanoVG::transformTranslate(float dst[6], float tx, float ty)
{
    nvgTransformTranslate(dst, tx, ty);
}

void NanoVG::transformScale(float dst[6], float sx, float sy)
{
    nvgTransformScale(dst, sx, sy);
}

void NanoVG::transformRotate(float dst[6], float a)
{
    nvgTransformRotate(dst, a);
}

void NanoVG::transformSkewX(float dst[6], float a)
{
    nvgTransformSkewX(dst, a);
}

void NanoVG::transformSkewY(float dst[6], float a)
{
    nvgTransformSkewY(dst, a);
}

void NanoVG::transformMultiply(float dst[6], const float src[6])
{
    nvgTransformMultiply(dst, src);
}

void NanoVG::transformPremultiply(float dst[6], const float src[6])
{
    nvgTransformPremultiply(dst, src);
}

int NanoVG::transformInverse(float dst[6], const float src[6])
{
    return nvgTransformInverse(dst, src);
}

void NanoVG::transformPoint(float& dstx, float& dsty, const float xform[6], float srcx, float srcy)
{
    nvgTransformPoint(&dstx, &dsty, xform, srcx, srcy);
}

float NanoVG::degToRad(float deg)
{
    return nvgDegToRad(deg);
}

float NanoVG::radToDeg(float rad)
{
    return nvgRadToDeg(rad);
}

// -----------------------------------------------------------------------
// Images

NanoImage* NanoVG::createImage(const char* filename)
{
    if (fContext == nullptr) return nullptr;
    DISTRHO_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', nullptr);

    if (const int imageId = nvgCreateImage(fContext, filename))
        return new NanoImage(fContext, imageId);

    return nullptr;
}

NanoImage* NanoVG::createImageMem(uchar* data, int ndata)
{
    if (fContext == nullptr) return nullptr;
    DISTRHO_SAFE_ASSERT_RETURN(data != nullptr, nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(ndata > 0, nullptr);

    if (const int imageId = nvgCreateImageMem(fContext, data, ndata))
        return new NanoImage(fContext, imageId);

    return nullptr;
}

NanoImage* NanoVG::createImageRGBA(uint w, uint h, const uchar* data)
{
    if (fContext == nullptr) return nullptr;
    DISTRHO_SAFE_ASSERT_RETURN(data != nullptr, nullptr);

    if (const int imageId = nvgCreateImageRGBA(fContext, w, h, data))
        return new NanoImage(fContext, imageId);

    return nullptr;
}

// -----------------------------------------------------------------------
// Paints

NanoVG::Paint NanoVG::linearGradient(float sx, float sy, float ex, float ey, const Color& icol, const Color& ocol)
{
    if (fContext == nullptr) return Paint();
    return nvgLinearGradient(fContext, sx, sy, ex, ey, icol, ocol);
}

NanoVG::Paint NanoVG::boxGradient(float x, float y, float w, float h, float r, float f, const Color& icol, const Color& ocol)
{
    if (fContext == nullptr) return Paint();
    return nvgBoxGradient(fContext, x, y, w, h, r, f, icol, ocol);
}

NanoVG::Paint NanoVG::radialGradient(float cx, float cy, float inr, float outr, const Color& icol, const Color& ocol)
{
    if (fContext == nullptr) return Paint();
    return nvgRadialGradient(fContext, cx, cy, inr, outr, icol, ocol);
}

NanoVG::Paint NanoVG::imagePattern(float ox, float oy, float ex, float ey, float angle, const NanoImage* image, NanoVG::PatternRepeat repeat)
{
    if (fContext == nullptr) return Paint();
    DISTRHO_SAFE_ASSERT_RETURN(image != nullptr, Paint());

    return nvgImagePattern(fContext, ox, oy, ex, ey, angle, image->fImageId, repeat);
}

// -----------------------------------------------------------------------
// Scissoring

void NanoVG::scissor(float x, float y, float w, float h)
{
    if (fContext != nullptr)
        nvgScissor(fContext, x, y, w, h);
}

void NanoVG::resetScissor()
{
    if (fContext != nullptr)
        nvgResetScissor(fContext);
}

// -----------------------------------------------------------------------
// Paths

void NanoVG::beginPath()
{
    if (fContext != nullptr)
        nvgBeginPath(fContext);
}

void NanoVG::moveTo(float x, float y)
{
    if (fContext != nullptr)
        nvgMoveTo(fContext, x, y);
}

void NanoVG::lineTo(float x, float y)
{
    if (fContext != nullptr)
        nvgLineTo(fContext, x, y);
}

void NanoVG::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
{
    if (fContext != nullptr)
        nvgBezierTo(fContext, c1x, c1y, c2x, c2y, x, y);
}

void NanoVG::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    if (fContext != nullptr)
        nvgArcTo(fContext, x1, y1, x2, y2, radius);
}

void NanoVG::closePath()
{
    if (fContext != nullptr)
        nvgClosePath(fContext);
}

void NanoVG::pathWinding(NanoVG::Winding dir)
{
    if (fContext != nullptr)
        nvgPathWinding(fContext, dir);
}

void NanoVG::arc(float cx, float cy, float r, float a0, float a1, NanoVG::Winding dir)
{
    if (fContext != nullptr)
        nvgArc(fContext, cx, cy, r, a0, a1, dir);
}

void NanoVG::rect(float x, float y, float w, float h)
{
    if (fContext != nullptr)
        nvgRect(fContext, x, y, w, h);
}

void NanoVG::roundedRect(float x, float y, float w, float h, float r)
{
    if (fContext != nullptr)
        nvgRoundedRect(fContext, x, y, w, h, r);
}

void NanoVG::ellipse(float cx, float cy, float rx, float ry)
{
    if (fContext != nullptr)
        nvgEllipse(fContext, cx, cy, rx, ry);
}

void NanoVG::circle(float cx, float cy, float r)
{
    if (fContext != nullptr)
        nvgCircle(fContext, cx, cy, r);
}

void NanoVG::fill()
{
    if (fContext != nullptr)
        nvgFill(fContext);
}

void NanoVG::stroke()
{
    if (fContext != nullptr)
        nvgStroke(fContext);
}

// -----------------------------------------------------------------------
// Text

NanoVG::FontId NanoVG::createFont(const char* name, const char* filename)
{
    if (fContext == nullptr) return -1;
    DISTRHO_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', -1);
    DISTRHO_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', -1);

    return nvgCreateFont(fContext, name, filename);
}

NanoVG::FontId NanoVG::createFontMem(const char* name, uchar* data, int ndata, bool freeData)
{
    if (fContext == nullptr) return -1;
    DISTRHO_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', -1);
    DISTRHO_SAFE_ASSERT_RETURN(data != nullptr, -1);

    return nvgCreateFontMem(fContext, name, data, ndata, freeData);
}

NanoVG::FontId NanoVG::findFont(const char* name)
{
    if (fContext == nullptr) return -1;
    DISTRHO_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', -1);

    return nvgFindFont(fContext, name);
}

void NanoVG::fontSize(float size)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(size > 0.0f,);

    nvgFontSize(fContext, size);
}

void NanoVG::fontBlur(float blur)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(blur >= 0.0f,);

    nvgFontBlur(fContext, blur);
}

void NanoVG::textLetterSpacing(float spacing)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(spacing >= 0.0f,);

    nvgTextLetterSpacing(fContext, spacing);
}

void NanoVG::textLineHeight(float lineHeight)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(lineHeight > 0.0f,);

    nvgTextLineHeight(fContext, lineHeight);
}

void NanoVG::textAlign(NanoVG::Align align)
{
    if (fContext != nullptr)
        nvgTextAlign(fContext, align);
}

void NanoVG::textAlign(int align)
{
    if (fContext != nullptr)
        nvgTextAlign(fContext, align);
}

void NanoVG::fontFaceId(FontId font)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(font >= 0,);

    nvgFontFaceId(fContext, font);
}

void NanoVG::fontFace(const char* font)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(font != nullptr && font[0] != '\0',);

    nvgFontFace(fContext, font);
}

float NanoVG::text(float x, float y, const char* string, const char* end)
{
    if (fContext == nullptr) return 0.0f;
    DISTRHO_SAFE_ASSERT_RETURN(string != nullptr && string[0] != '\0', 0.0f);

    return nvgText(fContext, x, y, string, end);
}

void NanoVG::textBox(float x, float y, float breakRowWidth, const char* string, const char* end)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(string != nullptr && string[0] != '\0',);

    nvgTextBox(fContext, x, y, breakRowWidth, string, end);
}

float NanoVG::textBounds(float x, float y, const char* string, const char* end, Rectangle<float>& bounds)
{
    if (fContext == nullptr) return 0.0f;
    DISTRHO_SAFE_ASSERT_RETURN(string != nullptr && string[0] != '\0', 0.0f);

    float b[4];
    const float ret = nvgTextBounds(fContext, x, y, string, end, b);
    bounds = Rectangle<float>(b[0], b[1], b[2], b[3]);
    return ret;
}

void NanoVG::textBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds)
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(string != nullptr && string[0] != '\0',);

    nvgTextBoxBounds(fContext, x, y, breakRowWidth, string, end, bounds);
}

int NanoVG::textGlyphPositions(float x, float y, const char* string, const char* end, NanoVG::GlyphPosition* positions, int maxPositions)
{
    if (fContext == nullptr) return 0;
    DISTRHO_SAFE_ASSERT_RETURN(string != nullptr && string[0] != '\0', 0);

    return nvgTextGlyphPositions(fContext, x, y, string, end, (NVGglyphPosition*)positions, maxPositions);
}

void NanoVG::textMetrics(float* ascender, float* descender, float* lineh)
{
    if (fContext != nullptr)
        nvgTextMetrics(fContext, ascender, descender, lineh);
}

int NanoVG::textBreakLines(const char* string, const char* end, float breakRowWidth, NanoVG::TextRow* rows, int maxRows)
{
    if (fContext != nullptr)
        return nvgTextBreakLines(fContext, string, end, breakRowWidth, (NVGtextRow*)rows, maxRows);
    return 0;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

extern "C" {
#include "nanovg/nanovg.c"
}

// -----------------------------------------------------------------------
