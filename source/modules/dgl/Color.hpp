/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_COLOR_HPP_INCLUDED
#define DGL_COLOR_HPP_INCLUDED

#include "Base.hpp"

struct NVGcolor;

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

/**
   A color made from red, green, blue and alpha floating-point values in [0..1] range.
*/
struct Color {
   /**
      Direct access to the color values.
    */
    union {
        float rgba[4];
        struct { float red, green, blue, alpha; };
    };

   /**
      Create solid black color.
    */
    Color() noexcept;

   /**
      Create a color from red, green, blue and alpha numeric values.
      Values must be in [0..255] range.
    */
    Color(int red, int green, int blue, int alpha = 255) noexcept;

   /**
      Create a color from red, green, blue and alpha floating-point values.
      Values must in [0..1] range.
    */
    Color(float red, float green, float blue, float alpha = 1.0f) noexcept;

   /**
      Create a color by copying another color.
    */
    Color(const Color& color) noexcept;
    Color& operator=(const Color& color) noexcept;

   /**
      Create a color by linearly interpolating two other colors.
    */
    Color(const Color& color1, const Color& color2, float u) noexcept;

   /**
      Create a color specified by hue, saturation and lightness.
      Values must in [0..1] range.
    */
    static Color fromHSL(float hue, float saturation, float lightness, float alpha = 1.0f);

   /**
      Create a color from a HTML string like "#333" or "#112233".
    */
    static Color fromHTML(const char* rgb, float alpha = 1.0f);

   /**
      Linearly interpolate this color against another.
    */
    void interpolate(const Color& other, float u) noexcept;

   /**
      Check if this color matches another.
      @note Comparison is forced within 8-bit color values.
    */
    bool isEqual(const Color& color, bool withAlpha = true) noexcept;
    bool isNotEqual(const Color& color, bool withAlpha = true) noexcept;
    bool operator==(const Color& color) noexcept;
    bool operator!=(const Color& color) noexcept;

   /**
      Fix color bounds if needed.
    */
    void fixBounds() noexcept;

   /**
      @internal
      Needed for NanoVG compatibility.
    */
    Color(const NVGcolor&) noexcept;
    operator NVGcolor() const noexcept;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_COLOR_HPP_INCLUDED
