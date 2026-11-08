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

#include "../Color.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

static float computeHue(float h, float m1, float m2)
{
    if (h < 0) h += 1;
    if (h > 1) h -= 1;
    if (h < 1.0f/6.0f)
        return m1 + (m2 - m1) * h * 6.0f;
    if (h < 3.0f/6.0f)
        return m2;
    if (h < 4.0f/6.0f)
        return m1 + (m2 - m1) * (2.0f/3.0f - h) * 6.0f;
    return m1;
}

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
    return static_cast<uchar>(value2 + 0.5f);
}

// -----------------------------------------------------------------------

Color::Color() noexcept
    : red(0.0f),
      green(0.0f),
      blue(0.0f),
      alpha(1.0f) {}

Color::Color(const int r, const int g, const int b, const float a) noexcept
    : red(static_cast<float>(r)/255.0f),
      green(static_cast<float>(g)/255.0f),
      blue(static_cast<float>(b)/255.0f),
      alpha(a)
{
    fixBounds();
}

Color::Color(const float r, const float g, const float b, const float a) noexcept
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

Color::Color(const Color& color1, const Color& color2, const float u) noexcept
    : red(color1.red),
      green(color1.green),
      blue(color1.blue),
      alpha(color1.alpha)
{
    interpolate(color2, u);
}

Color Color::withAlpha(const float alpha2) const noexcept
{
    Color color(*this);
    color.alpha = alpha2;
    return color;
}

Color Color::minus(const int value) const noexcept
{
    const float fvalue = static_cast<float>(value)/255.f;
    Color color(*this);
    color.red -= fvalue;
    color.green -= fvalue;
    color.blue -= fvalue;
    color.fixBounds();
    return color;
}

Color Color::minus(const float value) const noexcept
{
    Color color(*this);
    color.red -= value;
    color.green -= value;
    color.blue -= value;
    color.fixBounds();
    return color;
}

Color Color::plus(const int value) const noexcept
{
    const float fvalue = static_cast<float>(value)/255.f;
    Color color(*this);
    color.red += fvalue;
    color.green += fvalue;
    color.blue += fvalue;
    color.fixBounds();
    return color;
}

Color Color::plus(const float value) const noexcept
{
    Color color(*this);
    color.red += value;
    color.green += value;
    color.blue += value;
    color.fixBounds();
    return color;
}

Color Color::invert() const noexcept
{
    Color color(*this);
    color.red = 1.f - color.red;
    color.green = 1.f - color.green;
    color.blue = 1.f - color.blue;
    return color;
}

Color Color::asGrayscale() const noexcept
{
    Color color(*this);
    // values taken from https://goodcalculators.com/rgb-to-grayscale-conversion-calculator/
    color.red = color.green = color.blue = 0.299f * color.red + 0.587f * color.green + 0.114f * color.blue;
    return color;
}

Color Color::fromHSL(float hue, float saturation, float lightness, float alpha)
{
    float m1, m2;
    Color col;
    hue = fmodf(hue, 1.0f);
    if (hue < 0.0f) hue += 1.0f;
    fixRange(saturation);
    fixRange(lightness);
    m2 = lightness <= 0.5f ? (lightness * (1 + saturation)) : (lightness + saturation - lightness * saturation);
    m1 = 2 * lightness - m2;
    col.red = computeHue(hue + 1.0f/3.0f, m1, m2);
    col.green = computeHue(hue, m1, m2);
    col.blue = computeHue(hue - 1.0f/3.0f, m1, m2);
    col.alpha = alpha;
    col.fixBounds();
    return col;
}

Color Color::fromHTML(const char* rgb, const float alpha) noexcept
{
    Color fallback;
    DISTRHO_SAFE_ASSERT_RETURN(rgb != nullptr && rgb[0] != '\0', fallback);

    if (rgb[0] == '#')
        ++rgb;
    DISTRHO_SAFE_ASSERT_RETURN(rgb[0] != '\0', fallback);

    std::size_t rgblen = std::strlen(rgb);
    DISTRHO_SAFE_ASSERT_RETURN(rgblen == 3 || rgblen == 6, fallback);

    char rgbtmp[5] = { '0', 'x', '\0', '\0', '\0' };
    int r, g, b;

    if (rgblen == 3)
    {
        rgbtmp[2] = rgb[0];
        r = static_cast<int>(std::strtol(rgbtmp, nullptr, 16)) * 17;

        rgbtmp[2] = rgb[1];
        g = static_cast<int>(std::strtol(rgbtmp, nullptr, 16)) * 17;

        rgbtmp[2] = rgb[2];
        b = static_cast<int>(std::strtol(rgbtmp, nullptr, 16)) * 17;
    }
    else
    {
        rgbtmp[2] = rgb[0];
        rgbtmp[3] = rgb[1];
        r = static_cast<int>(std::strtol(rgbtmp, nullptr, 16));

        rgbtmp[2] = rgb[2];
        rgbtmp[3] = rgb[3];
        g = static_cast<int>(std::strtol(rgbtmp, nullptr, 16));

        rgbtmp[2] = rgb[4];
        rgbtmp[3] = rgb[5];
        b = static_cast<int>(std::strtol(rgbtmp, nullptr, 16));
    }

    return Color(r, g, b, alpha);
}

Color Color::fromRGB(const uint color, const float alpha) noexcept
{
    return Color(static_cast<int>(color >> 24) & 0xff,
                 static_cast<int>(color >> 16) & 0xff,
                 static_cast<int>(color >>  8) & 0xff,
                 alpha);
}

void Color::interpolate(const Color& other, float u) noexcept
{
    fixRange(u);
    const float oneMinusU = 1.0f - u;

    red   = (red   * oneMinusU) + (other.red   * u);
    green = (green * oneMinusU) + (other.green * u);
    blue  = (blue  * oneMinusU) + (other.blue  * u);
    alpha = (alpha * oneMinusU) + (other.alpha * u);

    fixBounds();
}

// -----------------------------------------------------------------------

bool Color::isEqual(const Color& color, const bool withAlpha) noexcept
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

bool Color::isNotEqual(const Color& color, const bool withAlpha) noexcept
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

END_NAMESPACE_DGL
