/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#ifdef _MSC_VER
// instantiated template classes whose methods are defined elsewhere
# pragma warning(disable:4661)
#endif

#include "../NanoVG.hpp"
#include "SubWidgetPrivateData.hpp"

#ifndef DGL_NO_SHARED_RESOURCES
# include "Resources.hpp"
#endif

// -----------------------------------------------------------------------

#if defined(DISTRHO_OS_WINDOWS)
# include <windows.h>
# define DGL_EXT(PROC, func) static PROC func;
DGL_EXT(PFNGLACTIVETEXTUREPROC,            glActiveTexture)
DGL_EXT(PFNGLATTACHSHADERPROC,             glAttachShader)
DGL_EXT(PFNGLBINDATTRIBLOCATIONPROC,       glBindAttribLocation)
DGL_EXT(PFNGLBINDBUFFERPROC,               glBindBuffer)
DGL_EXT(PFNGLBUFFERDATAPROC,               glBufferData)
DGL_EXT(PFNGLCOMPILESHADERPROC,            glCompileShader)
DGL_EXT(PFNGLCREATEPROGRAMPROC,            glCreateProgram)
DGL_EXT(PFNGLCREATESHADERPROC,             glCreateShader)
DGL_EXT(PFNGLDELETEBUFFERSPROC,            glDeleteBuffers)
DGL_EXT(PFNGLDELETEPROGRAMPROC,            glDeleteProgram)
DGL_EXT(PFNGLDELETESHADERPROC,             glDeleteShader)
DGL_EXT(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray)
DGL_EXT(PFNGLENABLEVERTEXATTRIBARRAYPROC,  glEnableVertexAttribArray)
DGL_EXT(PFNGLGENBUFFERSPROC,               glGenBuffers)
DGL_EXT(PFNGLGETPROGRAMIVPROC,             glGetProgramiv)
DGL_EXT(PFNGLGETPROGRAMINFOLOGPROC,        glGetProgramInfoLog)
DGL_EXT(PFNGLGETSHADERIVPROC,              glGetShaderiv)
DGL_EXT(PFNGLGETSHADERINFOLOGPROC,         glGetShaderInfoLog)
DGL_EXT(PFNGLGETUNIFORMLOCATIONPROC,       glGetUniformLocation)
DGL_EXT(PFNGLLINKPROGRAMPROC,              glLinkProgram)
DGL_EXT(PFNGLSHADERSOURCEPROC,             glShaderSource)
DGL_EXT(PFNGLSTENCILOPSEPARATEPROC,        glStencilOpSeparate)
DGL_EXT(PFNGLUNIFORM1IPROC,                glUniform1i)
DGL_EXT(PFNGLUNIFORM2FVPROC,               glUniform2fv)
DGL_EXT(PFNGLUNIFORM4FVPROC,               glUniform4fv)
DGL_EXT(PFNGLUSEPROGRAMPROC,               glUseProgram)
DGL_EXT(PFNGLVERTEXATTRIBPOINTERPROC,      glVertexAttribPointer)
DGL_EXT(PFNGLBLENDFUNCSEPARATEPROC,        glBlendFuncSeparate)
# ifdef DGL_USE_NANOVG_FBO
DGL_EXT(PFNGLCHECKFRAMEBUFFERSTATUSPROC,   glCheckFramebufferStatus)
DGL_EXT(PFNGLBINDFRAMEBUFFERPROC,          glBindFramebuffer)
DGL_EXT(PFNGLBINDRENDERBUFFERPROC,         glBindRenderbuffer)
DGL_EXT(PFNGLDELETEFRAMEBUFFERSPROC,       glDeleteFramebuffers)
DGL_EXT(PFNGLDELETERENDERBUFFERSPROC,      glDeleteRenderbuffers)
DGL_EXT(PFNGLFRAMEBUFFERTEXTURE2DPROC,     glFramebufferTexture2D)
DGL_EXT(PFNGLFRAMEBUFFERRENDERBUFFERPROC,  glFramebufferRenderbuffer)
DGL_EXT(PFNGLGENFRAMEBUFFERSPROC,          glGenFramebuffers)
DGL_EXT(PFNGLGENRENDERBUFFERSPROC,         glGenRenderbuffers)
DGL_EXT(PFNGLRENDERBUFFERSTORAGEPROC,      glRenderbufferStorage)
# endif
# ifdef DGL_USE_OPENGL3
DGL_EXT(PFNGLBINDBUFFERRANGEPROC,          glBindBufferRange)
DGL_EXT(PFNGLBINDVERTEXARRAYPROC,          glBindVertexArray)
DGL_EXT(PFNGLDELETEVERTEXARRAYSPROC,       glDeleteVertexArrays)
DGL_EXT(PFNGLGENERATEMIPMAPPROC,           glGenerateMipmap)
DGL_EXT(PFNGLGETUNIFORMBLOCKINDEXPROC,     glGetUniformBlockIndex)
DGL_EXT(PFNGLGENVERTEXARRAYSPROC,          glGenVertexArrays)
DGL_EXT(PFNGLUNIFORMBLOCKBINDINGPROC,      glUniformBlockBinding)
# endif
# undef DGL_EXT
#endif

// -----------------------------------------------------------------------
// Include NanoVG OpenGL implementation

//#define STB_IMAGE_STATIC
#if defined(DGL_USE_GLES2)
# define NANOVG_GLES2_IMPLEMENTATION
#elif defined(DGL_USE_GLES3)
# define NANOVG_GLES3_IMPLEMENTATION
#elif defined(DGL_USE_OPENGL3)
# define NANOVG_GL3_IMPLEMENTATION
#else
# define NANOVG_GL2_IMPLEMENTATION
#endif

#if defined(DISTRHO_OS_MAC) && defined(NANOVG_GL2_IMPLEMENTATION)
# define glBindVertexArray glBindVertexArrayAPPLE
# define glDeleteVertexArrays glDeleteVertexArraysAPPLE
# define glGenVertexArrays glGenVertexArraysAPPLE
#endif

#include "nanovg/nanovg_gl.h"

#ifdef DGL_USE_NANOVG_FBO
# define NANOVG_FBO_VALID 1
# include "nanovg/nanovg_gl_utils.h"
#endif

#if defined(NANOVG_GL2)
# define nvgCreateGLfn nvgCreateGL2
# define nvgDeleteGL nvgDeleteGL2
# define nvglCreateImageFromHandle nvglCreateImageFromHandleGL2
# define nvglImageHandle nvglImageHandleGL2
#elif defined(NANOVG_GL3)
# define nvgCreateGLfn nvgCreateGL3
# define nvgDeleteGL nvgDeleteGL3
# define nvglCreateImageFromHandle nvglCreateImageFromHandleGL3
# define nvglImageHandle nvglImageHandleGL3
#elif defined(NANOVG_GLES2)
# define nvgCreateGLfn nvgCreateGLES2
# define nvgDeleteGL nvgDeleteGLES2
# define nvglCreateImageFromHandle nvglCreateImageFromHandleGLES2
# define nvglImageHandle nvglImageHandleGLES2
#elif defined(NANOVG_GLES3)
# define nvgCreateGLfn nvgCreateGLES3
# define nvgDeleteGL nvgDeleteGLES3
# define nvglCreateImageFromHandle nvglCreateImageFromHandleGLES3
# define nvglImageHandle nvglImageHandleGLES3
#endif

// -----------------------------------------------------------------------

START_NAMESPACE_DGL

NVGcontext* nvgCreateGL(int flags)
{
#if defined(DISTRHO_OS_WINDOWS)
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
    static bool needsInit = true;
# define DGL_EXT(PROC, func) \
      if (needsInit) func = (PROC) wglGetProcAddress ( #func ); \
      DISTRHO_SAFE_ASSERT_RETURN(func != nullptr, nullptr);
# define DGL_EXT2(PROC, func, fallback) \
      if (needsInit) { \
        func = (PROC) wglGetProcAddress ( #func ); \
        if (func == nullptr) func = (PROC) wglGetProcAddress ( #fallback ); \
      } DISTRHO_SAFE_ASSERT_RETURN(func != nullptr, nullptr);
DGL_EXT(PFNGLACTIVETEXTUREPROC,            glActiveTexture)
DGL_EXT(PFNGLATTACHSHADERPROC,             glAttachShader)
DGL_EXT(PFNGLBINDATTRIBLOCATIONPROC,       glBindAttribLocation)
DGL_EXT(PFNGLBINDBUFFERPROC,               glBindBuffer)
DGL_EXT(PFNGLBUFFERDATAPROC,               glBufferData)
DGL_EXT(PFNGLCOMPILESHADERPROC,            glCompileShader)
DGL_EXT(PFNGLCREATEPROGRAMPROC,            glCreateProgram)
DGL_EXT(PFNGLCREATESHADERPROC,             glCreateShader)
DGL_EXT(PFNGLDELETEBUFFERSPROC,            glDeleteBuffers)
DGL_EXT(PFNGLDELETEPROGRAMPROC,            glDeleteProgram)
DGL_EXT(PFNGLDELETESHADERPROC,             glDeleteShader)
DGL_EXT(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray)
DGL_EXT(PFNGLENABLEVERTEXATTRIBARRAYPROC,  glEnableVertexAttribArray)
DGL_EXT(PFNGLGENBUFFERSPROC,               glGenBuffers)
DGL_EXT(PFNGLGETPROGRAMIVPROC,             glGetProgramiv)
DGL_EXT(PFNGLGETPROGRAMINFOLOGPROC,        glGetProgramInfoLog)
DGL_EXT(PFNGLGETSHADERIVPROC,              glGetShaderiv)
DGL_EXT(PFNGLGETSHADERINFOLOGPROC,         glGetShaderInfoLog)
DGL_EXT(PFNGLGETUNIFORMLOCATIONPROC,       glGetUniformLocation)
DGL_EXT(PFNGLLINKPROGRAMPROC,              glLinkProgram)
DGL_EXT(PFNGLSHADERSOURCEPROC,             glShaderSource)
DGL_EXT(PFNGLSTENCILOPSEPARATEPROC,        glStencilOpSeparate)
DGL_EXT(PFNGLUNIFORM1IPROC,                glUniform1i)
DGL_EXT(PFNGLUNIFORM2FVPROC,               glUniform2fv)
DGL_EXT(PFNGLUNIFORM4FVPROC,               glUniform4fv)
DGL_EXT(PFNGLUSEPROGRAMPROC,               glUseProgram)
DGL_EXT(PFNGLVERTEXATTRIBPOINTERPROC,      glVertexAttribPointer)
DGL_EXT(PFNGLBLENDFUNCSEPARATEPROC,        glBlendFuncSeparate)
# ifdef DGL_USE_NANOVG_FBO
DGL_EXT(PFNGLCHECKFRAMEBUFFERSTATUSPROC,   glCheckFramebufferStatus)
DGL_EXT2(PFNGLBINDFRAMEBUFFERPROC,         glBindFramebuffer,         glBindFramebufferEXT)
DGL_EXT2(PFNGLBINDRENDERBUFFERPROC,        glBindRenderbuffer,        glBindRenderbufferEXT)
DGL_EXT2(PFNGLDELETEFRAMEBUFFERSPROC,      glDeleteFramebuffers,      glDeleteFramebuffersEXT)
DGL_EXT2(PFNGLDELETERENDERBUFFERSPROC,     glDeleteRenderbuffers,     glDeleteRenderbuffersEXT)
DGL_EXT2(PFNGLFRAMEBUFFERTEXTURE2DPROC,    glFramebufferTexture2D,    glFramebufferTexture2DEXT)
DGL_EXT2(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer, glFramebufferRenderbufferEXT)
DGL_EXT2(PFNGLGENFRAMEBUFFERSPROC,         glGenFramebuffers,         glGenFramebuffersEXT)
DGL_EXT2(PFNGLGENRENDERBUFFERSPROC,        glGenRenderbuffers,        glGenRenderbuffersEXT)
DGL_EXT2(PFNGLRENDERBUFFERSTORAGEPROC,     glRenderbufferStorage,     glRenderbufferStorageEXT)
# endif
# ifdef DGL_USE_OPENGL3
DGL_EXT(PFNGLBINDBUFFERRANGEPROC,          glBindBufferRange)
DGL_EXT(PFNGLBINDVERTEXARRAYPROC,          glBindVertexArray)
DGL_EXT(PFNGLDELETEVERTEXARRAYSPROC,       glDeleteVertexArrays)
DGL_EXT(PFNGLGENERATEMIPMAPPROC,           glGenerateMipmap)
DGL_EXT(PFNGLGETUNIFORMBLOCKINDEXPROC,     glGetUniformBlockIndex)
DGL_EXT(PFNGLGENVERTEXARRAYSPROC,          glGenVertexArrays)
DGL_EXT(PFNGLUNIFORMBLOCKBINDINGPROC,      glUniformBlockBinding)
# endif
# undef DGL_EXT
# undef DGL_EXT2
    needsInit = false;
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic pop
# endif
#endif
    return nvgCreateGLfn(flags);
}

// -----------------------------------------------------------------------
// DGL Color class conversion

Color::Color(const NVGcolor& c) noexcept
    : red(c.r), green(c.g), blue(c.b), alpha(c.a)
{
    fixBounds();
}

Color::operator NVGcolor() const noexcept
{
    NVGcolor nc;
    nc.r = red;
    nc.g = green;
    nc.b = blue;
    nc.a = alpha;
    return nc;
}

// -----------------------------------------------------------------------
// NanoImage

NanoImage::NanoImage()
    : fHandle(),
      fSize() {}

NanoImage::NanoImage(const Handle& handle)
    : fHandle(handle),
      fSize()
{
    DISTRHO_SAFE_ASSERT_RETURN(fHandle.context != nullptr && fHandle.imageId != 0,);

    _updateSize();
}

NanoImage::~NanoImage()
{
    if (fHandle.context != nullptr && fHandle.imageId != 0)
        nvgDeleteImage(fHandle.context, fHandle.imageId);
}

NanoImage& NanoImage::operator=(const Handle& handle)
{
    if (fHandle.context != nullptr && fHandle.imageId != 0)
        nvgDeleteImage(fHandle.context, fHandle.imageId);

    fHandle.context = handle.context;
    fHandle.imageId = handle.imageId;
    _updateSize();

    return *this;
}

bool NanoImage::isValid() const noexcept
{
    return (fHandle.context != nullptr && fHandle.imageId != 0);
}

Size<uint> NanoImage::getSize() const noexcept
{
    return fSize;
}

GLuint NanoImage::getTextureHandle() const
{
    DISTRHO_SAFE_ASSERT_RETURN(fHandle.context != nullptr && fHandle.imageId != 0, 0);

    return nvglImageHandle(fHandle.context, fHandle.imageId);
}

void NanoImage::update(const uchar* const data)
{
    DISTRHO_SAFE_ASSERT_RETURN(fHandle.context != nullptr && fHandle.imageId != 0,);
    DISTRHO_SAFE_ASSERT_RETURN(data != nullptr,);

    nvgUpdateImage(fHandle.context, fHandle.imageId, data);
}

void NanoImage::_updateSize()
{
    int w=0, h=0;

    nvgImageSize(fHandle.context, fHandle.imageId, &w, &h);

    if (w < 0) w = 0;
    if (h < 0) h = 0;

    fSize.setSize(static_cast<uint>(w), static_cast<uint>(h));
}

// -----------------------------------------------------------------------
// Paint

NanoVG::Paint::Paint() noexcept
    : radius(0.0f), feather(0.0f), innerColor(), outerColor(), imageId(0)
{
    std::memset(xform, 0, sizeof(float)*6);
    std::memset(extent, 0, sizeof(float)*2);
}

NanoVG::Paint::Paint(const NVGpaint& p) noexcept
    : radius(p.radius), feather(p.feather), innerColor(p.innerColor), outerColor(p.outerColor), imageId(p.image)
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
    std::memcpy(p.xform, xform, sizeof(float)*6);
    std::memcpy(p.extent, extent, sizeof(float)*2);
    return p;
}

// -----------------------------------------------------------------------
// NanoVG

NanoVG::NanoVG(int flags)
    : fContext(nvgCreateGL(flags)),
      fInFrame(false),
      fIsSubWidget(false)
{
    DISTRHO_CUSTOM_SAFE_ASSERT("Failed to create NanoVG context, expect a black screen", fContext != nullptr);
}

NanoVG::NanoVG(NVGcontext* const context)
    : fContext(context),
      fInFrame(false),
      fIsSubWidget(true)
{
    DISTRHO_CUSTOM_SAFE_ASSERT("Failed to create NanoVG context, expect a black screen", fContext != nullptr);
}

NanoVG::~NanoVG()
{
    DISTRHO_CUSTOM_SAFE_ASSERT("Destroying NanoVG context with still active frame", ! fInFrame);

    if (fContext != nullptr && ! fIsSubWidget)
        nvgDeleteGL(fContext);
}

// -----------------------------------------------------------------------

void NanoVG::beginFrame(const uint width, const uint height, const float scaleFactor)
{
    DISTRHO_SAFE_ASSERT_RETURN(scaleFactor > 0.0f,);
    DISTRHO_SAFE_ASSERT_RETURN(! fInFrame,);
    fInFrame = true;

    if (fContext != nullptr)
        nvgBeginFrame(fContext, static_cast<int>(width), static_cast<int>(height), scaleFactor);
}

void NanoVG::beginFrame(Widget* const widget)
{
    DISTRHO_SAFE_ASSERT_RETURN(widget != nullptr,);
    DISTRHO_SAFE_ASSERT_RETURN(! fInFrame,);
    fInFrame = true;

    if (fContext == nullptr)
        return;

    if (TopLevelWidget* const tlw = widget->getTopLevelWidget())
        nvgBeginFrame(fContext,
                      static_cast<int>(tlw->getWidth()),
                      static_cast<int>(tlw->getHeight()),
                      tlw->getScaleFactor());
}

void NanoVG::cancelFrame()
{
    DISTRHO_SAFE_ASSERT_RETURN(fInFrame,);

    if (fContext != nullptr)
        nvgCancelFrame(fContext);

    fInFrame = false;
}

void NanoVG::endFrame()
{
    DISTRHO_SAFE_ASSERT_RETURN(fInFrame,);

    // Save current blend state
    GLboolean blendEnabled;
    GLint blendSrc, blendDst;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);

    if (fContext != nullptr)
        nvgEndFrame(fContext);

    // Restore blend state
    if (blendEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    glBlendFunc(blendSrc, blendDst);

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
    {
        DISTRHO_SAFE_ASSERT_RETURN(red   >= 0 && red   <= 255,);
        DISTRHO_SAFE_ASSERT_RETURN(green >= 0 && green <= 255,);
        DISTRHO_SAFE_ASSERT_RETURN(blue  >= 0 && blue  <= 255,);
        DISTRHO_SAFE_ASSERT_RETURN(alpha >= 0 && alpha <= 255,);

        nvgStrokeColor(fContext, nvgRGBA(static_cast<uchar>(red),
                                         static_cast<uchar>(green),
                                         static_cast<uchar>(blue),
                                         static_cast<uchar>(alpha)));
    }
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
    {
        DISTRHO_SAFE_ASSERT_RETURN(red   >= 0 && red   <= 255,);
        DISTRHO_SAFE_ASSERT_RETURN(green >= 0 && green <= 255,);
        DISTRHO_SAFE_ASSERT_RETURN(blue  >= 0 && blue  <= 255,);
        DISTRHO_SAFE_ASSERT_RETURN(alpha >= 0 && alpha <= 255,);

        nvgFillColor(fContext, nvgRGBA(static_cast<uchar>(red),
                                       static_cast<uchar>(green),
                                       static_cast<uchar>(blue),
                                       static_cast<uchar>(alpha)));
    }
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

void NanoVG::globalAlpha(float alpha)
{
    if (fContext != nullptr)
        nvgGlobalAlpha(fContext, alpha);
}

void NanoVG::globalTint(Color tint)
{
    if (fContext != nullptr)
        nvgGlobalTint(fContext, tint);
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
    if (fContext != nullptr)
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
    DISTRHO_SAFE_ASSERT_RETURN(d_isNotZero(x),);
    DISTRHO_SAFE_ASSERT_RETURN(d_isNotZero(y),);

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

NanoImage::Handle NanoVG::createImageFromFile(const char* filename, ImageFlags imageFlags)
{
    return createImageFromFile(filename, static_cast<int>(imageFlags));
}

NanoImage::Handle NanoVG::createImageFromFile(const char* filename, int imageFlags)
{
    if (fContext == nullptr) return NanoImage::Handle();
    DISTRHO_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', NanoImage::Handle());

    return NanoImage::Handle(fContext, nvgCreateImage(fContext, filename, imageFlags));
}

NanoImage::Handle NanoVG::createImageFromMemory(const uchar* data, uint dataSize, ImageFlags imageFlags)
{
    return createImageFromMemory(data, dataSize, static_cast<int>(imageFlags));
}

NanoImage::Handle NanoVG::createImageFromMemory(const uchar* data, uint dataSize, int imageFlags)
{
    if (fContext == nullptr) return NanoImage::Handle();
    DISTRHO_SAFE_ASSERT_RETURN(data != nullptr, NanoImage::Handle());
    DISTRHO_SAFE_ASSERT_RETURN(dataSize > 0,    NanoImage::Handle());

    return NanoImage::Handle(fContext, nvgCreateImageMem(fContext, imageFlags, data, static_cast<int>(dataSize)));
}

NanoImage::Handle NanoVG::createImageFromRawMemory(uint w, uint h, const uchar* data,
                                                   ImageFlags imageFlags, ImageFormat format)
{
    return createImageFromRawMemory(w, h, data, static_cast<int>(imageFlags), format);
}

NanoImage::Handle NanoVG::createImageFromRawMemory(uint w, uint h, const uchar* data,
                                                   int imageFlags, ImageFormat format)
{
    if (fContext == nullptr) return NanoImage::Handle();
    DISTRHO_SAFE_ASSERT_RETURN(data != nullptr, NanoImage::Handle());

    NVGtexture nvgformat;
    switch (format)
    {
    case kImageFormatGrayscale:
        nvgformat = NVG_TEXTURE_ALPHA;
        break;
    case kImageFormatBGR:
        nvgformat = NVG_TEXTURE_BGR;
        break;
    case kImageFormatBGRA:
        nvgformat = NVG_TEXTURE_BGRA;
        break;
    case kImageFormatRGB:
        nvgformat = NVG_TEXTURE_RGB;
        break;
    case kImageFormatRGBA:
        nvgformat = NVG_TEXTURE_RGBA;
        break;
    default:
        return NanoImage::Handle();
    }

    return NanoImage::Handle(fContext, nvgCreateImageRaw(fContext,
                                                         static_cast<int>(w),
                                                         static_cast<int>(h), imageFlags, nvgformat, data));
}

NanoImage::Handle NanoVG::createImageFromRGBA(uint w, uint h, const uchar* data, ImageFlags imageFlags)
{
    return createImageFromRGBA(w, h, data, static_cast<int>(imageFlags));
}

NanoImage::Handle NanoVG::createImageFromRGBA(uint w, uint h, const uchar* data, int imageFlags)
{
    if (fContext == nullptr) return NanoImage::Handle();
    DISTRHO_SAFE_ASSERT_RETURN(data != nullptr, NanoImage::Handle());

    return NanoImage::Handle(fContext, nvgCreateImageRGBA(fContext,
                                                          static_cast<int>(w),
                                                          static_cast<int>(h), imageFlags, data));
}

NanoImage::Handle NanoVG::createImageFromTextureHandle(GLuint textureId, uint w, uint h,
                                                       ImageFlags imageFlags, bool deleteTexture)
{
    return createImageFromTextureHandle(textureId, w, h, static_cast<int>(imageFlags), deleteTexture);
}

NanoImage::Handle NanoVG::createImageFromTextureHandle(GLuint textureId, uint w, uint h,
                                                       int imageFlags, bool deleteTexture)
{
    if (fContext == nullptr) return NanoImage::Handle();
    DISTRHO_SAFE_ASSERT_RETURN(textureId != 0, NanoImage::Handle());

    if (! deleteTexture)
        imageFlags |= NVG_IMAGE_NODELETE;

    return NanoImage::Handle(fContext, nvglCreateImageFromHandle(fContext,
                                                                 textureId,
                                                                 static_cast<int>(w),
                                                                 static_cast<int>(h), imageFlags));
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

NanoVG::Paint NanoVG::imagePattern(float ox, float oy, float ex, float ey, float angle, const NanoImage& image, float alpha)
{
    if (fContext == nullptr) return Paint();

    const int imageId(image.fHandle.imageId);
    DISTRHO_SAFE_ASSERT_RETURN(imageId != 0, Paint());

    return nvgImagePattern(fContext, ox, oy, ex, ey, angle, imageId, alpha);
}

// -----------------------------------------------------------------------
// Scissoring

void NanoVG::scissor(float x, float y, float w, float h)
{
    if (fContext != nullptr)
        nvgScissor(fContext, x, y, w, h);
}

void NanoVG::intersectScissor(float x, float y, float w, float h)
{
    if (fContext != nullptr)
        nvgIntersectScissor(fContext, x, y, w, h);
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

void NanoVG::quadTo(float cx, float cy, float x, float y)
{
    if (fContext != nullptr)
        nvgQuadTo(fContext, cx, cy, x, y);
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

NanoVG::FontId NanoVG::createFontFromFile(const char* name, const char* filename)
{
    DISTRHO_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', -1);
    DISTRHO_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', -1);
    DISTRHO_SAFE_ASSERT_RETURN(fContext != nullptr, -1);

    return nvgCreateFont(fContext, name, filename);
}

NanoVG::FontId NanoVG::createFontFromMemory(const char* name, const uchar* data, uint dataSize, bool freeData)
{
    DISTRHO_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', -1);
    DISTRHO_SAFE_ASSERT_RETURN(data != nullptr, -1);
    DISTRHO_SAFE_ASSERT_RETURN(fContext != nullptr, -1);

    return nvgCreateFontMem(fContext, name, const_cast<uchar*>(data), static_cast<int>(dataSize), freeData);
}

NanoVG::FontId NanoVG::findFont(const char* name)
{
    DISTRHO_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', -1);
    DISTRHO_SAFE_ASSERT_RETURN(fContext != nullptr, -1);

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

    float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float ret = nvgTextBounds(fContext, x, y, string, end, b);
    bounds = Rectangle<float>(b[0], b[1], b[2] - b[0], b[3] - b[1]);
    return ret;
}

void NanoVG::textBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float bounds[4])
{
    if (fContext == nullptr) return;
    DISTRHO_SAFE_ASSERT_RETURN(string != nullptr && string[0] != '\0',);

    nvgTextBoxBounds(fContext, x, y, breakRowWidth, string, end, bounds);
}

int NanoVG::textGlyphPositions(float x, float y, const char* string, const char* end, NanoVG::GlyphPosition& positions, int maxPositions)
{
    if (fContext == nullptr) return 0;
    DISTRHO_SAFE_ASSERT_RETURN(string != nullptr && string[0] != '\0', 0);

    return nvgTextGlyphPositions(fContext, x, y, string, end, (NVGglyphPosition*)&positions, maxPositions);
}

void NanoVG::textMetrics(float* ascender, float* descender, float* lineh)
{
    if (fContext != nullptr)
        nvgTextMetrics(fContext, ascender, descender, lineh);
}

int NanoVG::textBreakLines(const char* string, const char* end, float breakRowWidth, NanoVG::TextRow& rows, int maxRows)
{
    if (fContext != nullptr)
        return nvgTextBreakLines(fContext, string, end, breakRowWidth, (NVGtextRow*)&rows, maxRows);
    return 0;
}

#ifndef DGL_NO_SHARED_RESOURCES
bool NanoVG::loadSharedResources()
{
    if (fContext == nullptr) return false;

    if (nvgFindFont(fContext, NANOVG_DEJAVU_SANS_TTF) >= 0)
        return true;

    using namespace dpf_resources;

    return nvgCreateFontMem(fContext, NANOVG_DEJAVU_SANS_TTF, (uchar*)dejavusans_ttf, dejavusans_ttf_size, 0) >= 0;
}
#endif

// -----------------------------------------------------------------------

template <class BaseWidget>
void NanoBaseWidget<BaseWidget>::displayChildren()
{
    std::list<SubWidget*> children(BaseWidget::getChildren());

    for (std::list<SubWidget*>::iterator it = children.begin(); it != children.end(); ++it)
    {
        if (NanoSubWidget* const subwidget = dynamic_cast<NanoSubWidget*>(*it))
        {
            if (subwidget->fUsingParentContext && subwidget->isVisible())
                subwidget->onDisplay();
        }
    }
}

// -----------------------------------------------------------------------
// NanoSubWidget

template <>
NanoBaseWidget<SubWidget>::NanoBaseWidget(Widget* const parentWidget, int flags)
    : SubWidget(parentWidget),
      NanoVG(flags),
      fUsingParentContext(false)
{
    setNeedsViewportScaling();
}

template <>
NanoBaseWidget<SubWidget>::NanoBaseWidget(NanoSubWidget* const parentWidget)
    : SubWidget(parentWidget),
      NanoVG(parentWidget->getContext()),
      fUsingParentContext(true)
{
    setSkipDrawing();
}

template <>
NanoBaseWidget<SubWidget>::NanoBaseWidget(NanoTopLevelWidget* const parentWidget)
    : SubWidget(parentWidget),
      NanoVG(parentWidget->getContext()),
      fUsingParentContext(true)
{
    setSkipDrawing();
}

template <>
inline void NanoBaseWidget<SubWidget>::onDisplay()
{
    if (fUsingParentContext)
    {
        NanoVG::save();
        translate(SubWidget::getAbsoluteX(), SubWidget::getAbsoluteY());
        onNanoDisplay();
        NanoVG::restore();
        displayChildren();
    }
    else
    {
        NanoVG::beginFrame(SubWidget::getWidth(), SubWidget::getHeight());
        onNanoDisplay();
        displayChildren();
        NanoVG::endFrame();
       #ifdef DGL_USE_OPENGL3
        glUseProgram(reinterpret_cast<const OpenGL3GraphicsContext&>(getGraphicsContext()).program);
       #endif
    }
}

template class NanoBaseWidget<SubWidget>;

// -----------------------------------------------------------------------
// NanoTopLevelWidget

template <>
NanoBaseWidget<TopLevelWidget>::NanoBaseWidget(Window& windowToMapTo, int flags)
    : TopLevelWidget(windowToMapTo),
      NanoVG(flags),
      fUsingParentContext(false) {}

template <>
inline void NanoBaseWidget<TopLevelWidget>::onDisplay()
{
    NanoVG::beginFrame(TopLevelWidget::getWidth(), TopLevelWidget::getHeight());
    onNanoDisplay();
    displayChildren();
    NanoVG::endFrame();
}

template class NanoBaseWidget<TopLevelWidget>;

// -----------------------------------------------------------------------
// NanoStandaloneWindow

template <>
NanoBaseWidget<StandaloneWindow>::NanoBaseWidget(Application& app, int flags)
    : StandaloneWindow(app),
      NanoVG(flags),
      fUsingParentContext(false) {}

template <>
NanoBaseWidget<StandaloneWindow>::NanoBaseWidget(Application& app, Window& parentWindow, int flags)
    : StandaloneWindow(app, parentWindow),
      NanoVG(flags),
      fUsingParentContext(false) {}

template <>
inline void NanoBaseWidget<StandaloneWindow>::onDisplay()
{
    NanoVG::beginFrame(Window::getWidth(), Window::getHeight());
    onNanoDisplay();
    displayChildren();
    NanoVG::endFrame();
}

template class NanoBaseWidget<StandaloneWindow>;

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#undef final

#if defined(__GNUC__) && (__GNUC__ >= 6)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmisleading-indentation"
# pragma GCC diagnostic ignored "-Wshift-negative-value"
#endif

extern "C" {
#include "nanovg/nanovg.c"
}

#if defined(__GNUC__) && (__GNUC__ >= 6)
# pragma GCC diagnostic pop
#endif

// -----------------------------------------------------------------------
