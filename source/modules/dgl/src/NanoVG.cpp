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
// Conversions

NanoVG::Color::Color() noexcept
    : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}

NanoVG::Color::Color(const NVGcolor& c) noexcept
    : r(c.r), g(c.g), b(c.b), a(c.a) {}

NanoVG::Color::operator NVGcolor() const noexcept
{
    NVGcolor nc = {{{ r, g, b, a }}};
    return nc;
}

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

NanoImage::NanoImage(NVGcontext* context, int imageId) noexcept
    : fContext(context),
      fImageId(imageId) {}

NanoImage::~NanoImage()
{
    if (fContext != nullptr && fImageId != 0)
        nvgDeleteImage(fContext, fImageId);
}

Size<int> NanoImage::getSize() const
{
    int w=0, h=0;

    if (fContext != nullptr && fImageId != 0)
        nvgImageSize(fContext, fImageId, &w, &h);

    return Size<int>(w, h);
}

void NanoImage::updateImage(const uchar* data)
{
    if (fContext != nullptr && fImageId != 0)
        nvgUpdateImage(fContext, fImageId, data);
}

// -----------------------------------------------------------------------
// NanoVG

NanoVG::NanoVG()
    : fContext(nvgCreateGL(512, 512, NVG_ANTIALIAS))
{
    DISTRHO_SAFE_ASSERT_RETURN(fContext != nullptr,);
}

NanoVG::NanoVG(int textAtlasWidth, int textAtlasHeight)
    : fContext(nvgCreateGL(textAtlasWidth, textAtlasHeight, NVG_ANTIALIAS))
{
    DISTRHO_SAFE_ASSERT_RETURN(fContext != nullptr,);
}

NanoVG::~NanoVG()
{
    if (fContext == nullptr)
        return;

    nvgDeleteGL(fContext);
}

// -----------------------------------------------------------------------

void NanoVG::beginFrame(int width, int height, float scaleFactor, Alpha alpha)
{
    nvgBeginFrame(fContext, width, height, scaleFactor, static_cast<NVGalpha>(alpha));
}

void NanoVG::beginFrame(Widget* widget)
{
    DISTRHO_SAFE_ASSERT_RETURN(widget != nullptr,);

    Window& window(widget->getParentWindow());
    nvgBeginFrame(fContext, window.getWidth(), window.getHeight(), 1.0f, NVG_PREMULTIPLIED_ALPHA);
}

void NanoVG::endFrame()
{
    nvgEndFrame(fContext);
}

// -----------------------------------------------------------------------
// Color utils

NanoVG::Color NanoVG::RGB(uchar r, uchar g, uchar b)
{
    return nvgRGB(r, g, b);
}

NanoVG::Color NanoVG::RGBf(float r, float g, float b)
{
    return nvgRGBf(r, g, b);
}

NanoVG::Color NanoVG::RGBA(uchar r, uchar g, uchar b, uchar a)
{
    return nvgRGBA(r, g, b, a);
}

NanoVG::Color NanoVG::RGBAf(float r, float g, float b, float a)
{
    return nvgRGBAf(r, g, b, a);
}

NanoVG::Color NanoVG::lerpRGBA(const Color& c0, const Color& c1, float u)
{
    return nvgLerpRGBA(c0, c1, u);
}

NanoVG::Color NanoVG::HSL(float h, float s, float l)
{
    return nvgHSL(h, s, l);
}

NanoVG::Color NanoVG::HSLA(float h, float s, float l, uchar a)
{
    return nvgHSLA(h, s, l, a);
}

// -----------------------------------------------------------------------
// State Handling

void NanoVG::save()
{
    nvgSave(fContext);
}

void NanoVG::restore()
{
    nvgRestore(fContext);
}

void NanoVG::reset()
{
    nvgReset(fContext);
}

// -----------------------------------------------------------------------
// Render styles

void NanoVG::strokeColor(const Color& color)
{
    nvgStrokeColor(fContext, color);
}

void NanoVG::strokePaint(const Paint& paint)
{
    nvgStrokePaint(fContext, paint);
}

void NanoVG::fillColor(const Color& color)
{
    nvgFillColor(fContext, color);
}

void NanoVG::fillPaint(const Paint& paint)
{
    nvgFillPaint(fContext, paint);
}

void NanoVG::miterLimit(float limit)
{
    nvgMiterLimit(fContext, limit);
}

void NanoVG::strokeWidth(float size)
{
    nvgStrokeWidth(fContext, size);
}

void NanoVG::lineCap(NanoVG::LineCap cap)
{
    nvgLineCap(fContext, cap);
}

void NanoVG::lineJoin(NanoVG::LineCap join)
{
    nvgLineJoin(fContext, join);
}

// -----------------------------------------------------------------------
// Transforms

void NanoVG::resetTransform()
{
    nvgResetTransform(fContext);
}

void NanoVG::transform(float a, float b, float c, float d, float e, float f)
{
    nvgTransform(fContext, a, b, c, d, e, f);
}

void NanoVG::translate(float x, float y)
{
    nvgTranslate(fContext, x, y);
}

void NanoVG::rotate(float angle)
{
    nvgRotate(fContext, angle);
}

void NanoVG::skewX(float angle)
{
    nvgSkewX(fContext, angle);
}

void NanoVG::skewY(float angle)
{
    nvgSkewY(fContext, angle);
}

void NanoVG::scale(float x, float y)
{
    nvgScale(fContext, x, y);
}

void NanoVG::currentTransform(float xform[6])
{
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
    if (const int imageId = nvgCreateImage(fContext, filename))
        return new NanoImage(fContext, imageId);
    return nullptr;
}

NanoImage* NanoVG::createImageMem(uchar* data, int ndata)
{
    if (const int imageId = nvgCreateImageMem(fContext, data, ndata))
        return new NanoImage(fContext, imageId);
    return nullptr;
}

NanoImage* NanoVG::createImageRGBA(int w, int h, const uchar* data)
{
    if (const int imageId = nvgCreateImageRGBA(fContext, w, h, data))
        return new NanoImage(fContext, imageId);
    return nullptr;
}

// -----------------------------------------------------------------------
// Paints

NanoVG::Paint NanoVG::linearGradient(float sx, float sy, float ex, float ey, const NanoVG::Color& icol, const NanoVG::Color& ocol)
{
    return nvgLinearGradient(fContext, sx, sy, ex, ey, icol, ocol);
}

NanoVG::Paint NanoVG::boxGradient(float x, float y, float w, float h, float r, float f, const NanoVG::Color& icol, const NanoVG::Color& ocol)
{
    return nvgBoxGradient(fContext, x, y, w, h, r, f, icol, ocol);
}

NanoVG::Paint NanoVG::radialGradient(float cx, float cy, float inr, float outr, const NanoVG::Color& icol, const NanoVG::Color& ocol)
{
    return nvgRadialGradient(fContext, cx, cy, inr, outr, icol, ocol);
}

NanoVG::Paint NanoVG::imagePattern(float ox, float oy, float ex, float ey, float angle, const NanoImage* image, NanoVG::PatternRepeat repeat)
{
    DISTRHO_SAFE_ASSERT_RETURN(image != nullptr, Paint());
    return nvgImagePattern(fContext, ox, oy, ex, ey, angle, image->fImageId, repeat);
}

// -----------------------------------------------------------------------
// Scissoring

void NanoVG::scissor(float x, float y, float w, float h)
{
    nvgScissor(fContext, x, y, w, h);
}

void NanoVG::resetScissor()
{
    nvgResetScissor(fContext);
}

// -----------------------------------------------------------------------
// Paths

void NanoVG::beginPath()
{
    nvgBeginPath(fContext);
}

void NanoVG::moveTo(float x, float y)
{
    nvgMoveTo(fContext, x, y);
}

void NanoVG::lineTo(float x, float y)
{
    nvgLineTo(fContext, x, y);
}

void NanoVG::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
{
    nvgBezierTo(fContext, c1x, c1y, c2x, c2y, x, y);
}

void NanoVG::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    nvgArcTo(fContext, x1, y1, x2, y2, radius);
}

void NanoVG::closePath()
{
    nvgClosePath(fContext);
}

void NanoVG::pathWinding(NanoVG::Winding dir)
{
    nvgPathWinding(fContext, dir);
}

void NanoVG::arc(float cx, float cy, float r, float a0, float a1, NanoVG::Winding dir)
{
    nvgArc(fContext, cx, cy, r, a0, a1, dir);
}

void NanoVG::rect(float x, float y, float w, float h)
{
    nvgRect(fContext, x, y, w, h);
}

void NanoVG::roundedRect(float x, float y, float w, float h, float r)
{
    nvgRoundedRect(fContext, x, y, w, h, r);
}

void NanoVG::ellipse(float cx, float cy, float rx, float ry)
{
    nvgEllipse(fContext, cx, cy, rx, ry);
}

void NanoVG::circle(float cx, float cy, float r)
{
    nvgCircle(fContext, cx, cy, r);
}

void NanoVG::fill()
{
    nvgFill(fContext);
}

void NanoVG::stroke()
{
    nvgStroke(fContext);
}

// -----------------------------------------------------------------------
// Text

NanoVG::FontId NanoVG::createFont(const char* name, const char* filename)
{
    return nvgCreateFont(fContext, name, filename);
}

NanoVG::FontId NanoVG::createFontMem(const char* name, uchar* data, int ndata, bool freeData)
{
    return nvgCreateFontMem(fContext, name, data, ndata, freeData);
}

NanoVG::FontId NanoVG::findFont(const char* name)
{
    return nvgFindFont(fContext, name);
}

void NanoVG::fontSize(float size)
{
    nvgFontSize(fContext, size);
}

void NanoVG::fontBlur(float blur)
{
    nvgFontBlur(fContext, blur);
}

void NanoVG::textLetterSpacing(float spacing)
{
    nvgTextLetterSpacing(fContext, spacing);
}

void NanoVG::textLineHeight(float lineHeight)
{
    nvgTextLineHeight(fContext, lineHeight);
}

void NanoVG::textAlign(NanoVG::Align align)
{
    nvgTextAlign(fContext, align);
}

void NanoVG::textAlign(int align)
{
    nvgTextAlign(fContext, align);
}

void NanoVG::fontFaceId(FontId font)
{
    nvgFontFaceId(fContext, font);
}

void NanoVG::fontFace(const char* font)
{
    nvgFontFace(fContext, font);
}

float NanoVG::text(float x, float y, const char* string, const char* end)
{
    return nvgText(fContext, x, y, string, end);
}

void NanoVG::textBox(float x, float y, float breakRowWidth, const char* string, const char* end)
{
    nvgTextBox(fContext, x, y, breakRowWidth, string, end);
}

float NanoVG::textBounds(float x, float y, const char* string, const char* end, Rectangle<float>& bounds)
{
    float b[4];
    const float ret = nvgTextBounds(fContext, x, y, string, end, b);
    bounds = Rectangle<float>(b[0], b[1], b[2], b[3]);
    return ret;
}

void NanoVG::textBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds)
{
    nvgTextBoxBounds(fContext, x, y, breakRowWidth, string, end, bounds);
}

int NanoVG::textGlyphPositions(float x, float y, const char* string, const char* end, NanoVG::GlyphPosition* positions, int maxPositions)
{
    return nvgTextGlyphPositions(fContext, x, y, string, end, (NVGglyphPosition*)positions, maxPositions);
}

void NanoVG::textMetrics(float* ascender, float* descender, float* lineh)
{
    nvgTextMetrics(fContext, ascender, descender, lineh);
}

int NanoVG::textBreakLines(const char* string, const char* end, float breakRowWidth, NanoVG::TextRow* rows, int maxRows)
{
    return nvgTextBreakLines(fContext, string, end, breakRowWidth, (NVGtextRow*)rows, maxRows);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

extern "C" {
#include "nanovg/nanovg.c"
}

// -----------------------------------------------------------------------
