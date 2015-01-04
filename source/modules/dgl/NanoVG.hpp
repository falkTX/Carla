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

#ifndef DGL_NANO_WIDGET_HPP_INCLUDED
#define DGL_NANO_WIDGET_HPP_INCLUDED

#include "Color.hpp"
#include "Widget.hpp"

struct NVGcontext;
struct NVGpaint;

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// NanoImage

/**
   NanoVG Image class.

   This implements NanoVG images as a C++ class where deletion is handled automatically.
   Images need to be created within a NanoVG or NanoWidget class.
 */
class NanoImage
{
public:
   /**
      Destructor.
    */
    ~NanoImage();

   /**
      Get size.
    */
    Size<uint> getSize() const noexcept;

   /**
      Update image data.
    */
    void updateImage(const uchar* const data);

protected:
   /**
      Constructors are protected.
      NanoImages must be created within a NanoVG or NanoWidget class.
    */
    NanoImage(NVGcontext* const context, const int imageId) noexcept;

private:
    NVGcontext* fContext;
    int fImageId;
    Size<uint> fSize;
    friend class NanoVG;

    void _updateSize();

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NanoImage)
};

// -----------------------------------------------------------------------
// NanoVG

/**
   NanoVG class.

   This class exposes the NanoVG drawing API.
   All calls should be wrapped in beginFrame() and endFrame().

   @section State Handling
   NanoVG contains state which represents how paths will be rendered.
   The state contains transform, fill and stroke styles, text and font styles, and scissor clipping.

   @section Render styles
   Fill and stroke render style can be either a solid color or a paint which is a gradient or a pattern.
   Solid color is simply defined as a color value, different kinds of paints can be created
   using linearGradient(), boxGradient(), radialGradient() and imagePattern().

   Current render style can be saved and restored using save() and restore().

   @section Transforms
   The paths, gradients, patterns and scissor region are transformed by an transformation
   matrix at the time when they are passed to the API.
   The current transformation matrix is a affine matrix:
     [sx kx tx]
     [ky sy ty]
     [ 0  0  1]
   Where: sx,sy define scaling, kx,ky skewing, and tx,ty translation.
   The last row is assumed to be 0,0,1 and is not stored.

   Apart from resetTransform(), each transformation function first creates
   specific transformation matrix and pre-multiplies the current transformation by it.

   Current coordinate system (transformation) can be saved and restored using save() and restore().

   @section Images
   NanoVG allows you to load jpg, png, psd, tga, pic and gif files to be used for rendering.
   In addition you can upload your own image. The image loading is provided by stb_image.

   @section Paints
   NanoVG supports four types of paints: linear gradient, box gradient, radial gradient and image pattern.
   These can be used as paints for strokes and fills.

   @section Scissoring
   Scissoring allows you to clip the rendering into a rectangle. This is useful for varius
   user interface cases like rendering a text edit or a timeline.

   @section Paths
   Drawing a new shape starts with beginPath(), it clears all the currently defined paths.
   Then you define one or more paths and sub-paths which describe the shape. The are functions
   to draw common shapes like rectangles and circles, and lower level step-by-step functions,
   which allow to define a path curve by curve.

   NanoVG uses even-odd fill rule to draw the shapes. Solid shapes should have counter clockwise
   winding and holes should have counter clockwise order. To specify winding of a path you can
   call pathWinding(). This is useful especially for the common shapes, which are drawn CCW.

   Finally you can fill the path using current fill style by calling fill(), and stroke it
   with current stroke style by calling stroke().

   The curve segments and sub-paths are transformed by the current transform.

   @section Text
   NanoVG allows you to load .ttf files and use the font to render text.

   The appearance of the text can be defined by setting the current text style
   and by specifying the fill color. Common text and font settings such as
   font size, letter spacing and text align are supported. Font blur allows you
   to create simple text effects such as drop shadows.

   At render time the font face can be set based on the font handles or name.

   Font measure functions return values in local space, the calculations are
   carried in the same resolution as the final rendering. This is done because
   the text glyph positions are snapped to the nearest pixels sharp rendering.

   The local space means that values are not rotated or scale as per the current
   transformation. For example if you set font size to 12, which would mean that
   line height is 16, then regardless of the current scaling and rotation, the
   returned line height is always 16. Some measures may vary because of the scaling
   since aforementioned pixel snapping.

   While this may sound a little odd, the setup allows you to always render the
   same way regardless of scaling. i.e. following works regardless of scaling:

   @code
       const char* txt = "Text me up.";
       textBounds(vg, x,y, txt, NULL, bounds);
       beginPath(vg);
       roundedRect(vg, bounds[0], bounds[1], bounds[2]-bounds[0], bounds[3]-bounds[1]);
       fill(vg);
   @endcode

   Note: currently only solid color fill is supported for text.
 */
class NanoVG
{
public:
    enum Align {
        // Horizontal align
        ALIGN_LEFT     = 1 << 0, // Align horizontally to left (default).
        ALIGN_CENTER   = 1 << 1, // Align horizontally to center.
        ALIGN_RIGHT    = 1 << 2, // Align horizontally to right.
        // Vertical align
        ALIGN_TOP      = 1 << 3, // Align vertically to top.
        ALIGN_MIDDLE   = 1 << 4, // Align vertically to middle.
        ALIGN_BOTTOM   = 1 << 5, // Align vertically to bottom.
        ALIGN_BASELINE = 1 << 6  // Align vertically to baseline (default).
    };

    enum Alpha {
        STRAIGHT_ALPHA,
        PREMULTIPLIED_ALPHA
    };

    enum LineCap {
        BUTT,
        ROUND,
        SQUARE,
        BEVEL,
        MITER
    };

    enum PatternRepeat {
        REPEAT_NONE = 0x0, // No repeat
        REPEAT_X    = 0x1, // Repeat in X direction
        REPEAT_Y    = 0x2  // Repeat in Y direction
    };

    enum Solidity {
        SOLID = 1, // CCW
        HOLE  = 2  // CW
    };

    enum Winding {
        CCW = 1, // Winding for solid shapes
        CW  = 2  // Winding for holes
    };

    struct Paint {
        float xform[6];
        float extent[2];
        float radius;
        float feather;
        Color innerColor;
        Color outerColor;
        int   imageId;
        PatternRepeat repeat;

        Paint() noexcept;

       /**
          @internal
        */
        Paint(const NVGpaint&) noexcept;
        operator NVGpaint() const noexcept;
    };

    struct GlyphPosition {
        const char* str;  // Position of the glyph in the input string.
        float x;          // The x-coordinate of the logical glyph position.
        float minx, maxx; // The bounds of the glyph shape.
    };

    struct TextRow {
        const char* start; // Pointer to the input text where the row starts.
        const char* end;   // Pointer to the input text where the row ends (one past the last character).
        const char* next;  // Pointer to the beginning of the next row.
        float width;       // Logical width of the row.
        float minx, maxx;  // Actual bounds of the row. Logical with and bounds can differ because of kerning and some parts over extending.
    };

    typedef int FontId;

   /**
      Constructor.
      Uses 512x512 as default atlas size.
    */
    NanoVG();

   /**
      Constructor using custom text atlas size.
    */
    NanoVG(const int textAtlasWidth, const int textAtlasHeight);

   /**
      Destructor.
    */
    ~NanoVG();

   /**
      Get the NanoVG context.
      You should not need this under normal circumstances.
    */
    NVGcontext* getContext() const noexcept
    {
        return fContext;
    }

   /**
      Begin drawing a new frame.
      @param withAlha Controls if drawing the shapes to the render target should be done using straight or pre-multiplied alpha.
    */
    void beginFrame(const uint width, const uint height, const float scaleFactor = 1.0f, const Alpha alpha = PREMULTIPLIED_ALPHA);

   /**
      Begin drawing a new frame inside a widget.
    */
    void beginFrame(Widget* const widget);

   /**
      Ends drawing flushing remaining render state.
    */
    void endFrame();

   /* --------------------------------------------------------------------
    * State Handling */

   /**
      Pushes and saves the current render state into a state stack.
      A matching restore() must be used to restore the state.
    */
    void save();

   /**
      Pops and restores current render state.
    */
    void restore();

   /**
      Resets current render state to default values. Does not affect the render state stack.
    */
    void reset();

   /* --------------------------------------------------------------------
    * Render styles */

   /**
      Sets current stroke style to a solid color.
    */
    void strokeColor(const Color& color);

   /**
      Sets current stroke style to a solid color, made from red, green, blue and alpha numeric values.
      Values must be in [0..255] range.
    */
    void strokeColor(const int red, const int green, const int blue, const int alpha = 255);

   /**
      Sets current stroke style to a solid color, made from red, green, blue and alpha numeric values.
      Values must in [0..1] range.
    */
    void strokeColor(const float red, const float green, const float blue, const float alpha = 1.0f);

   /**
      Sets current stroke style to a paint, which can be a one of the gradients or a pattern.
    */
    void strokePaint(const Paint& paint);

   /**
      Sets current fill style to a solid color.
    */
    void fillColor(const Color& color);

   /**
      Sets current fill style to a solid color, made from red, green, blue and alpha numeric values.
      Values must be in [0..255] range.
    */
    void fillColor(const int red, const int green, const int blue, const int alpha = 255);

   /**
      Sets current fill style to a solid color, made from red, green, blue and alpha numeric values.
      Values must in [0..1] range.
    */
    void fillColor(const float red, const float green, const float blue, const float alpha = 1.0f);

   /**
      Sets current fill style to a paint, which can be a one of the gradients or a pattern.
    */
    void fillPaint(const Paint& paint);

   /**
      Sets the miter limit of the stroke style.
      Miter limit controls when a sharp corner is beveled.
    */
    void miterLimit(float limit);

   /**
      Sets the stroke width of the stroke style.
    */
    void strokeWidth(float size);

   /**
      Sets how the end of the line (cap) is drawn,
      Can be one of: BUTT, ROUND, SQUARE.
    */
    void lineCap(LineCap cap = BUTT);

   /**
      Sets how sharp path corners are drawn.
      Can be one of MITER, ROUND, BEVEL.
    */
    void lineJoin(LineCap join = MITER);

   /* --------------------------------------------------------------------
    * Transforms */

   /**
      Resets current transform to a identity matrix.
    */
    void resetTransform();

   /**
      Pre-multiplies current coordinate system by specified matrix.
      The parameters are interpreted as matrix as follows:
        [a c e]
        [b d f]
        [0 0 1]
    */
    void transform(float a, float b, float c, float d, float e, float f);

   /**
      Translates current coordinate system.
    */
    void translate(float x, float y);

   /**
      Rotates current coordinate system. Angle is specified in radians.
    */
    void rotate(float angle);

   /**
      Skews the current coordinate system along X axis. Angle is specified in radians.
    */
    void skewX(float angle);

   /**
      Skews the current coordinate system along Y axis. Angle is specified in radians.
    */
    void skewY(float angle);

   /**
      Scales the current coordinate system.
    */
    void scale(float x, float y);

   /**
      Stores the top part (a-f) of the current transformation matrix in to the specified buffer.
        [a c e]
        [b d f]
        [0 0 1]
    */
    void currentTransform(float xform[6]);

   /**
      The following functions can be used to make calculations on 2x3 transformation matrices.
      A 2x3 matrix is represented as float[6]. */

   /**
      Sets the transform to identity matrix.
    */
    static void transformIdentity(float dst[6]);

   /**
      Sets the transform to translation matrix
    */
    static void transformTranslate(float dst[6], float tx, float ty);

   /**
      Sets the transform to scale matrix.
    */
    static void transformScale(float dst[6], float sx, float sy);

   /**
      Sets the transform to rotate matrix. Angle is specified in radians.
    */
    static void transformRotate(float dst[6], float a);

   /**
      Sets the transform to skew-x matrix. Angle is specified in radians.
    */
    static void transformSkewX(float dst[6], float a);

   /**
      Sets the transform to skew-y matrix. Angle is specified in radians.
    */
    static void transformSkewY(float dst[6], float a);

   /**
      Sets the transform to the result of multiplication of two transforms, of A = A*B.
    */
    static void transformMultiply(float dst[6], const float src[6]);

   /**
      Sets the transform to the result of multiplication of two transforms, of A = B*A.
    */
    static void transformPremultiply(float dst[6], const float src[6]);

   /**
      Sets the destination to inverse of specified transform.
      Returns 1 if the inverse could be calculated, else 0.
    */
    static int transformInverse(float dst[6], const float src[6]);

   /**
      Transform a point by given transform.
    */
    static void transformPoint(float& dstx, float& dsty, const float xform[6], float srcx, float srcy);

   /**
      Convert degrees to radians.
    */
    static float degToRad(float deg);

   /**
      Convert radians to degrees.
    */
    static float radToDeg(float rad);

   /* --------------------------------------------------------------------
    * Images */

   /**
      Creates image by loading it from the disk from specified file name.
    */
    NanoImage* createImage(const char* filename);

   /**
      Creates image by loading it from the specified chunk of memory.
    */
    NanoImage* createImageMem(uchar* data, int ndata);

   /**
      Creates image from specified image data.
    */
    NanoImage* createImageRGBA(uint w, uint h, const uchar* data);

   /* --------------------------------------------------------------------
    * Paints */

   /**
      Creates and returns a linear gradient. Parameters (sx,sy)-(ex,ey) specify the start and end coordinates
      of the linear gradient, icol specifies the start color and ocol the end color.
      The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
    */
    Paint linearGradient(float sx, float sy, float ex, float ey, const Color& icol, const Color& ocol);

   /**
      Creates and returns a box gradient. Box gradient is a feathered rounded rectangle, it is useful for rendering
      drop shadows or highlights for boxes. Parameters (x,y) define the top-left corner of the rectangle,
      (w,h) define the size of the rectangle, r defines the corner radius, and f feather. Feather defines how blurry
      the border of the rectangle is. Parameter icol specifies the inner color and ocol the outer color of the gradient.
      The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
    */
    Paint boxGradient(float x, float y, float w, float h, float r, float f, const Color& icol, const Color& ocol);

   /**
      Creates and returns a radial gradient. Parameters (cx,cy) specify the center, inr and outr specify
      the inner and outer radius of the gradient, icol specifies the start color and ocol the end color.
      The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
    */
    Paint radialGradient(float cx, float cy, float inr, float outr, const Color& icol, const Color& ocol);

   /**
      Creates and returns an image pattern. Parameters (ox,oy) specify the left-top location of the image pattern,
      (ex,ey) the size of one image, angle rotation around the top-left corner, image is handle to the image to render,
      and repeat tells if the image should be repeated across x or y.
      The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
    */
    Paint imagePattern(float ox, float oy, float ex, float ey, float angle, const NanoImage* image, PatternRepeat repeat);

   /* --------------------------------------------------------------------
    * Scissoring */

   /**
      Sets the current
      The scissor rectangle is transformed by the current transform.
    */
    void scissor(float x, float y, float w, float h);

   /**
      Reset and disables scissoring.
    */
    void resetScissor();

   /* --------------------------------------------------------------------
    * Paths */

   /**
      Clears the current path and sub-paths.
    */
    void beginPath();

   /**
      Starts new sub-path with specified point as first point.
    */
    void moveTo(float x, float y);

   /**
      Adds line segment from the last point in the path to the specified point.
    */
    void lineTo(float x, float y);

   /**
      Adds bezier segment from last point in the path via two control points to the specified point.
    */
    void bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);

   /**
      Adds an arc segment at the corner defined by the last path point, and two specified points.
    */
    void arcTo(float x1, float y1, float x2, float y2, float radius);

   /**
      Closes current sub-path with a line segment.
    */
    void closePath();

   /**
      Sets the current sub-path winding.
    */
    void pathWinding(Winding dir);

   /**
      Creates new arc shaped sub-path.
    */
    void arc(float cx, float cy, float r, float a0, float a1, Winding dir);

   /**
      Creates new rectangle shaped sub-path.
    */
    void rect(float x, float y, float w, float h);

   /**
      Creates new rounded rectangle shaped sub-path.
    */
    void roundedRect(float x, float y, float w, float h, float r);

   /**
      Creates new ellipse shaped sub-path.
    */
    void ellipse(float cx, float cy, float rx, float ry);

   /**
      Creates new circle shaped sub-path.
    */
    void circle(float cx, float cy, float r);

   /**
      Fills the current path with current fill style.
    */
    void fill();

   /**
      Fills the current path with current stroke style.
    */
    void stroke();

   /* --------------------------------------------------------------------
    * Text */

   /**
      Creates font by loading it from the disk from specified file name.
      Returns handle to the font.
    */
    FontId createFont(const char* name, const char* filename);

   /**
      Creates font by loading it from the specified memory chunk.
      Returns handle to the font.
    */
    FontId createFontMem(const char* name, uchar* data, int ndata, bool freeData);

   /**
      Finds a loaded font of specified name, and returns handle to it, or -1 if the font is not found.
    */
    FontId findFont(const char* name);

   /**
      Sets the font size of current text style.
    */
    void fontSize(float size);

   /**
      Sets the blur of current text style.
    */
    void fontBlur(float blur);

   /**
      Sets the letter spacing of current text style.
    */
    void textLetterSpacing(float spacing);

   /**
      Sets the proportional line height of current text style. The line height is specified as multiple of font size.
    */
    void textLineHeight(float lineHeight);

   /**
      Sets the text align of current text style.
    */
    void textAlign(Align align);

   /**
      Sets the text align of current text style.
      Overloaded function for convenience.
      @see Align
    */
    void textAlign(int align);

   /**
      Sets the font face based on specified id of current text style.
    */
    void fontFaceId(FontId font);

   /**
      Sets the font face based on specified name of current text style.
    */
    void fontFace(const char* font);

   /**
      Draws text string at specified location. If end is specified only the sub-string up to the end is drawn.
    */
    float text(float x, float y, const char* string, const char* end);

   /**
      Draws multi-line text string at specified location wrapped at the specified width. If end is specified only the sub-string up to the end is drawn.
      White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
      Words longer than the max width are slit at nearest character (i.e. no hyphenation).
    */
    void textBox(float x, float y, float breakRowWidth, const char* string, const char* end);

   /**
      Measures the specified text string. The bounds value are [xmin,ymin, xmax,ymax].
      Returns the horizontal advance of the measured text (i.e. where the next character should drawn).
      Measured values are returned in local coordinate space.
    */
    float textBounds(float x, float y, const char* string, const char* end, Rectangle<float>& bounds);

   /**
      Measures the specified multi-text string. Parameter bounds should be a pointer to float[4],
      if the bounding box of the text should be returned. The bounds value are [xmin,ymin, xmax,ymax]
      Measured values are returned in local coordinate space.
    */
    void textBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds);

   /**
      Calculates the glyph x positions of the specified text. If end is specified only the sub-string will be used.
      Measured values are returned in local coordinate space.
    */
    int textGlyphPositions(float x, float y, const char* string, const char* end, GlyphPosition* positions, int maxPositions);

   /**
      Returns the vertical metrics based on the current text style.
      Measured values are returned in local coordinate space.
    */
    void textMetrics(float* ascender, float* descender, float* lineh);

   /**
      Breaks the specified text into lines. If end is specified only the sub-string will be used.
      White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
      Words longer than the max width are slit at nearest character (i.e. no hyphenation).
    */
    int textBreakLines(const char* string, const char* end, float breakRowWidth, TextRow* rows, int maxRows);

private:
    NVGcontext* const fContext;
    bool fInFrame;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NanoVG)
};

// -----------------------------------------------------------------------
// NanoWidget

/**
   NanoVG Widget class.

   This class implements the NanoVG drawing API inside a DGL Widget.
   The drawing function onDisplay() is implemented internally but a
   new onNanoDisplay() needs to be overridden instead.
 */
class NanoWidget : public Widget,
                   public NanoVG
{
public:
   /**
      Constructor.
    */
    NanoWidget(Window& parent)
        : Widget(parent),
          NanoVG()
    {
        setNeedsScaling(true);
    }

protected:
   /**
      New virtual onDisplay function.
      @see onDisplay
    */
    virtual void onNanoDisplay() = 0;

private:
   /**
      Widget display function.
      Implemented internally to wrap begin/endFrame() automatically.
    */
    void onDisplay() override
    {
        //glPushAttrib(GL_PIXEL_MODE_BIT|GL_STENCIL_BUFFER_BIT|GL_ENABLE_BIT);
        beginFrame(getWidth(), getHeight());
        onNanoDisplay();
        endFrame();
        //glPopAttrib();
        glDisable(GL_CULL_FACE);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NanoWidget)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_NANO_WIDGET_HPP_INCLUDED
