// ----------------------------------------------------------------------
//
//  Copyright (C) 2007-2010 Fons Adriaensen <fons@linuxaudio.org>
//  Modified by falkTX on Jan-Apr 2015 for inclusion in Carla
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// ----------------------------------------------------------------------


#include <png.h>
#include <clxclient.h>

namespace REV1 {


XImage *png2img (const char *file, X_display *disp, XftColor *bgnd)
{
    FILE                 *F;
    png_byte             hdr [8];
    png_structp          png_ptr;
    png_infop            png_info;
    const unsigned char  **data, *p;
    int                  dx, dy, x, y, dp;
    float                vr, vg, vb, va, br, bg, bb;
    unsigned long        mr, mg, mb, pix;
    XImage               *image;

    F = fopen (file, "r");
    if (!F)
    {
	fprintf (stderr, "Can't open '%s'\n", file);
	return 0;
    }
    fread (hdr, 1, 8, F);
    if (png_sig_cmp (hdr, 0, 8))
    {
	fprintf (stderr, "'%s' is not a PNG file\n", file);
	return 0;
    }
    fseek (F, 0, SEEK_SET);

    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (! png_ptr)
    {
	fclose (F);
	return 0;
    }
    png_info = png_create_info_struct (png_ptr);
    if (! png_info)
    {
        png_destroy_read_struct (&png_ptr, 0, 0);
	fclose (F);
        return 0;
    }
    if (setjmp (png_jmpbuf (png_ptr)))
    {
        png_destroy_read_struct (&png_ptr, &png_info, 0);
        fclose (F);
	fprintf (stderr, "png:longjmp()\n");
	return 0;
    }

    png_init_io (png_ptr, F);
    png_read_png (png_ptr, png_info,
                  PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND,
                  0);

// This requires libpng14 or later. If you still have an
// older version, use the three commented lines instead.

    dx = png_get_image_width (png_ptr, png_info);
    dy = png_get_image_height (png_ptr, png_info);
    dp = (png_get_color_type (png_ptr, png_info) & PNG_COLOR_MASK_ALPHA) ? 4 : 3;

//    dx = png_info->width;
//    dy = png_info->height;
//    dp = (png_info->color_type & PNG_COLOR_MASK_ALPHA) ? 4 : 3;

    data = (const unsigned char **)(png_get_rows (png_ptr, png_info));

    image = XCreateImage (disp->dpy (),
                          disp->dvi (),
                          DefaultDepth (disp->dpy (), disp->dsn ()),
                          ZPixmap, 0, 0, dx, dy, 32, 0);
    image->data = new char [image->height * image->bytes_per_line];

    mr = image->red_mask;
    mg = image->green_mask;
    mb = image->blue_mask;

    vr = mr / 255.0f;
    vg = mg / 255.0f;
    vb = mb / 255.0f;
    if (bgnd)
    {
        br = bgnd->color.red   >> 8;
        bg = bgnd->color.green >> 8;
        bb = bgnd->color.blue  >> 8;
    }
    else br = bg = bb = 0;

    for (y = 0; y < dy; y++)
    {
	p = data [y];
        for (x = 0; x < dx; x++)
	{
	    va = (dp == 4) ? (p [3] / 255.0f) : 1;
	    pix = ((unsigned long)((p [0] * va + (1 - va) * br) * vr) & mr) 
                | ((unsigned long)((p [1] * va + (1 - va) * bg) * vg) & mg)
                | ((unsigned long)((p [2] * va + (1 - va) * bb) * vb) & mb);
	    XPutPixel (image, x, y, pix);
	    p += dp;
	}
    }

    png_destroy_read_struct (&png_ptr, &png_info, 0);
    fclose (F);

    return image;
}


}
