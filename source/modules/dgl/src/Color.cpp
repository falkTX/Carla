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

#include "../Color.hpp"

#include "nanovg/nanovg.h"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

static void fixRange(float& value)
{
    /**/ if (value < 0.0f)
        value = 0.0f;
    else if (value > 1.0f)
        value = 1.0f;
}

static float getFixedRange(const float& value)
{
    if (value <= 0.0f)
        return 0.0f;
    if (value >= 1.0f)
        return 1.0f;
    return value;
}

static uchar getFixedRange2(const float& value)
{
    const float value2(getFixedRange(value)*255.0f);
    if (value2 <= 0.0f)
        return 0;
    if (value2 >= 255.0f)
        return 255;
    return static_cast<uchar>(value2);
}

// -----------------------------------------------------------------------

Color::Color() noexcept
    : red(1.0f),
      green(1.0f),
      blue(1.0f),
      alpha(1.0f) {}

Color::Color(int r, int g, int b, int a) noexcept
    : red(static_cast<float>(r)/255.0f),
      green(static_cast<float>(g)/255.0f),
      blue(static_cast<float>(b)/255.0f),
      alpha(static_cast<float>(a)/255.0f)
{
    fixBounds();
}

Color::Color(float r, float g, float b, float a) noexcept
    : red(r),
      green(g),
      blue(b),
      alpha(a)
{
    fixBounds();
}

Color::Color(const Color& color) noexcept
    : red(color.red),
      green(color.green),
      blue(color.blue),
      alpha(color.alpha)
{
    fixBounds();
}

Color& Color::operator=(const Color& color) noexcept
{
    red   = color.red;
    green = color.green;
    blue  = color.blue;
    alpha = color.alpha;
    fixBounds();
    return *this;
}

Color::Color(const Color& color1, const Color& color2, float u) noexcept
    : red(color1.red),
      green(color1.green),
      blue(color1.blue),
      alpha(color1.alpha)
{
    interpolate(color2, u);
}

Color Color::fromHSL(float hue, float saturation, float lightness, float alpha)
{
    return nvgHSLA(hue, saturation, lightness, static_cast<uchar>(getFixedRange(alpha)*255.0f));
}

Color Color::fromHTML(const char* rgb, float alpha)
{
    Color fallback;
    DISTRHO_SAFE_ASSERT_RETURN(rgb != nullptr && rgb[0] != '\0', fallback);

    if (rgb[0] == '#') ++rgb;
    DISTRHO_SAFE_ASSERT_RETURN(rgb[0] != '\0', fallback);

    std::size_t rgblen(std::strlen(rgb));
    DISTRHO_SAFE_ASSERT_RETURN(rgblen == 3 || rgblen == 6, fallback);

    char rgbtmp[3] = { '\0', '\0', '\0' };
    int r, g, b;

    if (rgblen == 3)
    {
        rgbtmp[0] = rgb[0];
        r = static_cast<int>(std::strtol(rgbtmp, nullptr, 16));

        rgbtmp[0] = rgb[1];
        g = static_cast<int>(std::strtol(rgbtmp, nullptr, 16));

        rgbtmp[0] = rgb[2];
        b = static_cast<int>(std::strtol(rgbtmp, nullptr, 16));
    }
    else
    {
        rgbtmp[0] = rgb[0];
        rgbtmp[1] = rgb[1];
        r = static_cast<int>(std::strtol(rgbtmp, nullptr, 16));

        rgbtmp[0] = rgb[2];
        rgbtmp[1] = rgb[3];
        g = static_cast<int>(std::strtol(rgbtmp, nullptr, 16));

        rgbtmp[0] = rgb[4];
        rgbtmp[1] = rgb[5];
        b = static_cast<int>(std::strtol(rgbtmp, nullptr, 16));
    }

    return Color(r, g, b, static_cast<int>(getFixedRange(alpha)*255.0f));
}

void Color::interpolate(const Color& other, float u) noexcept
{
    fixRange(u);
    const float oneMinusU(1.0f - u);

    red   = red   * oneMinusU + other.red   * u;
    green = green * oneMinusU + other.green * u;
    blue  = blue  * oneMinusU + other.blue  * u;
    alpha = alpha * oneMinusU + other.alpha * u;

    fixBounds();
}

// -----------------------------------------------------------------------

bool Color::isEqual(const Color& color, bool withAlpha) noexcept
{
    const uchar r1 = getFixedRange2(rgba[0]);
    const uchar g1 = getFixedRange2(rgba[1]);
    const uchar b1 = getFixedRange2(rgba[2]);
    const uchar a1 = getFixedRange2(rgba[3]);

    const uchar r2 = getFixedRange2(color.rgba[0]);
    const uchar g2 = getFixedRange2(color.rgba[1]);
    const uchar b2 = getFixedRange2(color.rgba[2]);
    const uchar a2 = getFixedRange2(color.rgba[3]);

    if (withAlpha)
        return (r1 == r2 && g1 == g2 && b1 == b2 && a1 == a2);
    else
        return (r1 == r2 && g1 == g2 && b1 == b2);
}

bool Color::isNotEqual(const Color& color, bool withAlpha) noexcept
{
    const uchar r1 = getFixedRange2(rgba[0]);
    const uchar g1 = getFixedRange2(rgba[1]);
    const uchar b1 = getFixedRange2(rgba[2]);
    const uchar a1 = getFixedRange2(rgba[3]);

    const uchar r2 = getFixedRange2(color.rgba[0]);
    const uchar g2 = getFixedRange2(color.rgba[1]);
    const uchar b2 = getFixedRange2(color.rgba[2]);
    const uchar a2 = getFixedRange2(color.rgba[3]);

    if (withAlpha)
        return (r1 != r2 || g1 != g2 || b1 != b2 || a1 != a2);
    else
        return (r1 != r2 || g1 != g2 || b1 != b2);
}

bool Color::operator==(const Color& color) noexcept
{
    return isEqual(color, true);
}

bool Color::operator!=(const Color& color) noexcept
{
    return isNotEqual(color, true);
}

// -----------------------------------------------------------------------

void Color::fixBounds() noexcept
{
    fixRange(red);
    fixRange(green);
    fixRange(blue);
    fixRange(alpha);
}

// -----------------------------------------------------------------------

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

END_NAMESPACE_DGL
