/*
 * PatchBay Canvas Themes
 * Copyright (C) 2010-2019 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "theme.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QPen>

//---------------------------------------------------------------------------------------------------------------------

struct Theme::PrivateData {
    const Theme::List idx;

    // Canvas
    QColor canvas_bg;

    // Boxes
    QPen box_pen;
    QPen box_pen_sel;
    QColor box_bg_1;
    QColor box_bg_2;
    QColor box_shadow;
    // box_header_pixmap
    int box_header_height;
    int box_header_spacing;

    QPen box_text;
    QPen box_text_sel;
    int box_text_ypos;
    QString box_font_name;
    int box_font_size;
    QFont::Weight box_font_state;

    BackgroundType box_bg_type;
    bool box_use_icon;

    // Ports
    QPen port_text;
    int port_text_ypos = 12;
    // port_bg_pixmap = None;
    QString port_font_name;
    int port_font_size;
    QFont::Weight port_font_state;
    PortType port_mode;

    QPen port_audio_jack_pen;
    QPen port_audio_jack_pen_sel;
    QPen port_midi_jack_pen;
    QPen port_midi_jack_pen_sel;
    QPen port_midi_alsa_pen;
    QPen port_midi_alsa_pen_sel;
    QPen port_parameter_pen;
    QPen port_parameter_pen_sel;

    QColor port_audio_jack_bg;
    QColor port_audio_jack_bg_sel;
    QColor port_midi_jack_bg;
    QColor port_midi_jack_bg_sel;
    QColor port_midi_alsa_bg;
    QColor port_midi_alsa_bg_sel;
    QColor port_parameter_bg;
    QColor port_parameter_bg_sel;

    QColor port_audio_jack_text;
    QColor port_audio_jack_text_sel;
    QColor port_midi_jack_text;
    QColor port_midi_jack_text_sel;
    QColor port_midi_alsa_text;
    QColor port_midi_alsa_text_sel;
    QColor port_parameter_text;
    QColor port_parameter_text_sel;

    int port_height;
    int port_offset;
    int port_spacing;
    int port_spacingT;

    // Lines
    QColor line_audio_jack;
    QColor line_audio_jack_sel;
    QColor line_audio_jack_glow;
    QColor line_midi_jack;
    QColor line_midi_jack_sel;
    QColor line_midi_jack_glow;
    QColor line_midi_alsa;
    QColor line_midi_alsa_sel;
    QColor line_midi_alsa_glow;
    QColor line_parameter;
    QColor line_parameter_sel;
    QColor line_parameter_glow;

    QPen rubberband_pen;
    QColor rubberband_brush;

    //-----------------------------------------------------------------------------------------------------------------

    PrivateData(const Theme::List id)
        : idx(id)
    {
        switch (idx)
        {
        case THEME_MODERN_DARK:
            // Canvas
            canvas_bg = QColor(0, 0, 0);

            // Boxes
            box_pen = QPen(QColor(76, 77, 78), 1, Qt::SolidLine);
            box_pen_sel = QPen(QColor(206, 207, 208), 1, Qt::DashLine);
            box_bg_1 = QColor(32, 34, 35);
            box_bg_2 = QColor(43, 47, 48);
            box_shadow = QColor(89, 89, 89, 180);
            // box_header_pixmap = None;
            box_header_height = 24;
            box_header_spacing = 0;

            box_text = QPen(QColor(240, 240, 240), 0);
            box_text_sel = box_text;
            box_text_ypos = 16;
            box_font_name = "Deja Vu Sans";
            box_font_size = 11;
            box_font_state = QFont::Bold;

            box_bg_type = THEME_BG_GRADIENT;
            box_use_icon = true;

            // Ports
            port_text = QPen(QColor(250, 250, 250), 0);
            port_text_ypos = 12;
            // port_bg_pixmap = None;
            port_font_name = "Deja Vu Sans";
            port_font_size = 11;
            port_font_state = QFont::Normal;
            port_mode = THEME_PORT_POLYGON;

            port_audio_jack_pen = QPen(QColor(63, 90, 126), 1);
            port_audio_jack_pen_sel = QPen(QColor(63 + 30, 90 + 30, 126 + 30), 1);
            port_midi_jack_pen = QPen(QColor(159, 44, 42), 1);
            port_midi_jack_pen_sel = QPen(QColor(159 + 30, 44 + 30, 42 + 30), 1);
            port_midi_alsa_pen = QPen(QColor(93, 141, 46), 1);
            port_midi_alsa_pen_sel = QPen(QColor(93 + 30, 141 + 30, 46 + 30), 1);
            port_parameter_pen = QPen(QColor(137, 76, 43), 1);
            port_parameter_pen_sel = QPen(QColor(137 + 30, 76 + 30, 43 + 30), 1);

            port_audio_jack_bg = QColor(35, 61, 99);
            port_audio_jack_bg_sel = QColor(35 + 50, 61 + 50, 99 + 50);
            port_midi_jack_bg = QColor(120, 15, 16);
            port_midi_jack_bg_sel = QColor(120 + 50, 15 + 50, 16 + 50);
            port_midi_alsa_bg = QColor(64, 112, 18);
            port_midi_alsa_bg_sel = QColor(64 + 50, 112 + 50, 18 + 50);
            port_parameter_bg = QColor(101, 47, 16);
            port_parameter_bg_sel = QColor(101 + 50, 47 + 50, 16 + 50);

            /*
            port_audio_jack_text = port_text;
            port_audio_jack_text_sel = port_text;
            port_midi_jack_text = port_text;
            port_midi_jack_text_sel = port_text;
            port_midi_alsa_text = port_text;
            port_midi_alsa_text_sel = port_text;
            port_parameter_text = port_text;
            port_parameter_text_sel = port_text;
            */

            port_height = 16;
            port_offset = 0;
            port_spacing = 2;
            port_spacingT = 2;

            // Lines
            line_audio_jack = QColor(63, 90, 126);
            line_audio_jack_sel = QColor(63 + 90, 90 + 90, 126 + 90);
            line_audio_jack_glow = QColor(100, 100, 200);
            line_midi_jack = QColor(159, 44, 42);
            line_midi_jack_sel = QColor(159 + 90, 44 + 90, 42 + 90);
            line_midi_jack_glow = QColor(200, 100, 100);
            line_midi_alsa = QColor(93, 141, 46);
            line_midi_alsa_sel = QColor(93 + 90, 141 + 90, 46 + 90);
            line_midi_alsa_glow = QColor(100, 200, 100);
            line_parameter = QColor(137, 76, 43);
            line_parameter_sel = QColor(137 + 90, 76 + 90, 43 + 90);
            line_parameter_glow = QColor(166, 133, 133);

            rubberband_pen = QPen(QColor(206, 207, 208), 1, Qt::SolidLine);
            rubberband_brush = QColor(76, 77, 78, 100);
            break;

#if 0
    elif idx == THEME_MODERN_DARK_TINY:
        # Canvas
        canvas_bg = QColor(0, 0, 0)

        # Boxes
        box_pen = QPen(QColor(76, 77, 78), 1, Qt::SolidLine)
        box_pen_sel = QPen(QColor(206, 207, 208), 1, Qt::DashLine)
        box_bg_1 = QColor(32, 34, 35)
        box_bg_2 = QColor(43, 47, 48)
        box_shadow = QColor(89, 89, 89, 180)
        box_header_pixmap = None
        box_header_height = 14
        box_header_spacing = 0

        box_text = QPen(QColor(240, 240, 240), 0)
        box_text_sel = ox_text
        box_text_ypos = 10
        box_font_name = "Deja Vu Sans"
        box_font_size = 10
        box_font_state = QFont::Bold

        box_bg_type = THEME_BG_GRADIENT
        box_use_icon = false

        # Ports
        port_text = QPen(QColor(250, 250, 250), 0)
        port_text_ypos = 9
        port_bg_pixmap = None
        port_font_name = "Deja Vu Sans"
        port_font_size = 9
        port_font_state = QFont::Normal
        port_mode = THEME_PORT_POLYGON

        port_audio_jack_pen = QPen(QColor(63, 90, 126), 1)
        port_audio_jack_pen_sel = QPen(QColor(63 + 30, 90 + 30, 126 + 30), 1)
        port_midi_jack_pen = QPen(QColor(159, 44, 42), 1)
        port_midi_jack_pen_sel = QPen(QColor(159 + 30, 44 + 30, 42 + 30), 1)
        port_midi_alsa_pen = QPen(QColor(93, 141, 46), 1)
        port_midi_alsa_pen_sel = QPen(QColor(93 + 30, 141 + 30, 46 + 30), 1)
        port_parameter_pen = QPen(QColor(137, 76, 43), 1)
        port_parameter_pen_sel = QPen(QColor(137 + 30, 76 + 30, 43 + 30), 1)

        port_audio_jack_bg = QColor(35, 61, 99)
        port_audio_jack_bg_sel = QColor(35 + 50, 61 + 50, 99 + 50)
        port_midi_jack_bg = QColor(120, 15, 16)
        port_midi_jack_bg_sel = QColor(120 + 50, 15 + 50, 16 + 50)
        port_midi_alsa_bg = QColor(64, 112, 18)
        port_midi_alsa_bg_sel = QColor(64 + 50, 112 + 50, 18 + 50)
        port_parameter_bg = QColor(101, 47, 16)
        port_parameter_bg_sel = QColor(101 + 50, 47 + 50, 16 + 50)

        port_audio_jack_text = port_text
        port_audio_jack_text_sel = port_text
        port_midi_jack_text = port_text
        port_midi_jack_text_sel = port_text
        port_midi_alsa_text = port_text
        port_midi_alsa_text_sel = port_text
        port_parameter_text = port_text
        port_parameter_text_sel = port_text

        port_height = 12
        port_offset = 0
        port_spacing = 1
        port_spacingT = 1

        # Lines
        line_audio_jack = QColor(63, 90, 126)
        line_audio_jack_sel = QColor(63 + 90, 90 + 90, 126 + 90)
        line_audio_jack_glow = QColor(100, 100, 200)
        line_midi_jack = QColor(159, 44, 42)
        line_midi_jack_sel = QColor(159 + 90, 44 + 90, 42 + 90)
        line_midi_jack_glow = QColor(200, 100, 100)
        line_midi_alsa = QColor(93, 141, 46)
        line_midi_alsa_sel = QColor(93 + 90, 141 + 90, 46 + 90)
        line_midi_alsa_glow = QColor(100, 200, 100)
        line_parameter = QColor(137, 76, 43)
        line_parameter_sel = QColor(137 + 90, 76 + 90, 43 + 90)
        line_parameter_glow = QColor(166, 133, 133)

        rubberband_pen = QPen(QColor(206, 207, 208), 1, Qt::SolidLine)
        rubberband_brush = QColor(76, 77, 78, 100)

    elif idx == THEME_MODERN_LIGHT:
        # Canvas
        canvas_bg = QColor(248, 249, 250)

        # Boxes
        box_pen = QPen(QColor(176, 177, 178), 1, Qt::SolidLine)
        box_pen_sel = QPen(QColor(1, 2, 3), 2, Qt::DashLine)
        box_bg_1 = QColor(250, 250, 250)
        box_bg_2 = QColor(200, 200, 200)
        box_shadow = QColor(1, 1, 1, 100)
        box_header_pixmap = None
        box_header_height = 24
        box_header_spacing = 0

        box_text = QPen(QColor(1, 1, 1), 0)
        box_text_sel = ox_text
        box_text_ypos = 16
        box_font_name = "Ubuntu"
        box_font_size = 11
        box_font_state = QFont::Bold

        box_bg_type = THEME_BG_GRADIENT
        box_use_icon = True

        # Ports
        port_text = QPen(QColor(255, 255, 255), 1)
        port_text_ypos = 12
        port_bg_pixmap = None
        port_font_name = "Ubuntu"
        port_font_size = 11
        port_font_state = QFont::Bold
        port_mode = THEME_PORT_POLYGON

        port_audio_jack_pen = QPen(QColor(103, 130, 166), 2)
        port_audio_jack_pen_sel = QPen(QColor(103 + 136, 190 + 130, 226 + 130), 1)
        port_midi_jack_pen = QPen(QColor(159, 44, 42), 1)
        port_midi_jack_pen_sel = QPen(QColor(90 + 30, 44 + 30, 42 + 30), 1)
        port_midi_alsa_pen = QPen(QColor(93, 141, 46), 1)
        port_midi_alsa_pen_sel = QPen(QColor(93 + 30, 141 + 30, 46 + 30), 1)
        port_parameter_pen = QPen(QColor(137, 76, 43), 1)
        port_parameter_pen_sel = QPen(QColor(137 + 30, 76 + 30, 43 + 30), 1)

        port_audio_jack_bg = QColor(0, 0, 180)
        port_audio_jack_bg_sel = QColor(135 + 150, 161 + 150, 199 + 150)
        port_midi_jack_bg = QColor(130, 15, 16)
        port_midi_jack_bg_sel = QColor(90 + 30, 15 + 50, 16 + 50)
        port_midi_alsa_bg = QColor(64, 112, 18)
        port_midi_alsa_bg_sel = QColor(64 + 50, 112 + 50, 18 + 50)
        port_parameter_bg = QColor(101, 47, 16)
        port_parameter_bg_sel = QColor(101 + 50, 47 + 50, 16 + 50)

        port_audio_jack_text = port_text
        port_audio_jack_text_sel = port_text
        port_midi_jack_text = port_text
        port_midi_jack_text_sel = port_text
        port_midi_alsa_text = port_text
        port_midi_alsa_text_sel = port_text
        port_parameter_text = port_text
        port_parameter_text_sel = port_text

        port_height = 16
        port_offset = 0
        port_spacing = 2
        port_spacingT = 2

        # Lines
        line_audio_jack = QColor(63, 90, 126)
        line_audio_jack_sel = QColor(63 + 63, 90 + 90, 126 + 90)
        line_audio_jack_glow = QColor(100, 100, 200)
        line_midi_jack = QColor(159, 44, 42)
        line_midi_jack_sel = QColor(159 + 44, 44 + 90, 42 + 90)
        line_midi_jack_glow = QColor(200, 100, 100)
        line_midi_alsa = QColor(93, 141, 46)
        line_midi_alsa_sel = QColor(93 + 90, 141 + 90, 46 + 90)
        line_midi_alsa_glow = QColor(100, 200, 100)
        line_parameter = QColor(137, 43, 43)
        line_parameter_sel = QColor(137 + 90, 76 + 90, 43 + 90)
        line_parameter_glow = QColor(166, 133, 133)

        rubberband_pen = QPen(QColor(206, 207, 208), 1, Qt::SolidLine)
        rubberband_brush = QColor(76, 77, 78, 100)

    elif idx == THEME_CLASSIC_DARK:
        # Canvas
        canvas_bg = QColor(0, 0, 0)

        # Boxes
        box_pen = QPen(QColor(147 - 70, 151 - 70, 143 - 70), 2, Qt::SolidLine)
        box_pen_sel = QPen(QColor(147, 151, 143), 2, Qt::DashLine)
        box_bg_1 = QColor(30, 34, 36)
        box_bg_2 = QColor(30, 34, 36)
        box_shadow = QColor(89, 89, 89, 180)
        box_header_pixmap = None
        box_header_height = 19
        box_header_spacing = 0

        box_text = QPen(QColor(255, 255, 255), 0)
        box_text_sel = ox_text
        box_text_ypos = 12
        box_font_name = "Sans"
        box_font_size = 12
        box_font_state = QFont::Normal

        box_bg_type = THEME_BG_GRADIENT
        box_use_icon = false

        # Ports
        port_text = QPen(QColor(250, 250, 250), 0)
        port_text_ypos = 11
        port_bg_pixmap = None
        port_font_name = "Sans"
        port_font_size = 11
        port_font_state = QFont::Normal
        port_mode = THEME_PORT_SQUARE

        port_audio_jack_pen = QPen(QColor(35, 61, 99), Qt::NoPen, 0)
        port_audio_jack_pen_sel = QPen(QColor(255, 0, 0), Qt::NoPen, 0)
        port_midi_jack_pen = QPen(QColor(120, 15, 16), Qt::NoPen, 0)
        port_midi_jack_pen_sel = QPen(QColor(255, 0, 0), Qt::NoPen, 0)
        port_midi_alsa_pen = QPen(QColor(63, 112, 19), Qt::NoPen, 0)
        port_midi_alsa_pen_sel = QPen(QColor(255, 0, 0), Qt::NoPen, 0)
        port_parameter_pen = QPen(QColor(101, 47, 17), Qt::NoPen, 0)
        port_parameter_pen_sel = QPen(QColor(255, 0, 0), Qt::NoPen, 0)

        port_audio_jack_bg = QColor(35, 61, 99)
        port_audio_jack_bg_sel = QColor(255, 0, 0)
        port_midi_jack_bg = QColor(120, 15, 16)
        port_midi_jack_bg_sel = QColor(255, 0, 0)
        port_midi_alsa_bg = QColor(63, 112, 19)
        port_midi_alsa_bg_sel = QColor(255, 0, 0)
        port_parameter_bg = QColor(101, 47, 17)
        port_parameter_bg_sel = QColor(255, 0, 0)

        port_audio_jack_text = port_text
        port_audio_jack_text_sel = port_text
        port_midi_jack_text = port_text
        port_midi_jack_text_sel = port_text
        port_midi_alsa_text = port_text
        port_midi_alsa_text_sel = port_text
        port_parameter_text = port_text
        port_parameter_text_sel = port_text

        port_height = 14
        port_offset = 0
        port_spacing = 1
        port_spacingT = 0

        # Lines
        line_audio_jack = QColor(53, 78, 116)
        line_audio_jack_sel = QColor(255, 0, 0)
        line_audio_jack_glow = QColor(255, 0, 0)
        line_midi_jack = QColor(139, 32, 32)
        line_midi_jack_sel = QColor(255, 0, 0)
        line_midi_jack_glow = QColor(255, 0, 0)
        line_midi_alsa = QColor(81, 130, 36)
        line_midi_alsa_sel = QColor(255, 0, 0)
        line_midi_alsa_glow = QColor(255, 0, 0)
        line_parameter = QColor(120, 65, 33)
        line_parameter_sel = QColor(255, 0, 0)
        line_parameter_glow = QColor(255, 0, 0)

        rubberband_pen = QPen(QColor(147, 151, 143), 2, Qt::SolidLine)
        rubberband_brush = QColor(35, 61, 99, 100)

    elif idx == THEME_OOSTUDIO:
        # Canvas
        canvas_bg = QColor(11, 11, 11)

        # Boxes
        box_pen = QPen(QColor(76, 77, 78), 1, Qt::SolidLine)
        box_pen_sel = QPen(QColor(189, 122, 214), 1, Qt::DashLine)
        box_bg_1 = QColor(46, 46, 46)
        box_bg_2 = QColor(23, 23, 23)
        box_shadow = QColor(89, 89, 89, 180)
        box_header_pixmap = QPixmap(":/bitmaps/canvas/frame_node_header.png")
        box_header_height = 22
        box_header_spacing = 6

        box_text = QPen(QColor(144, 144, 144), 0)
        box_text_sel = QPen(QColor(189, 122, 214), 0)
        box_text_ypos = 16
        box_font_name = "Deja Vu Sans"
        box_font_size = 11
        box_font_state = QFont::Bold

        box_bg_type = THEME_BG_SOLID
        box_use_icon = false

        # Ports
        normalPortBG = QColor(46, 46, 46)
        selPortBG = QColor(23, 23, 23)

        port_text = QPen(QColor(155, 155, 155), 0)
        port_text_ypos = 14
        port_bg_pixmap = QPixmap(":/bitmaps/canvas/frame_port_bg.png")
        port_font_name = "Deja Vu Sans"
        port_font_size = 11
        port_font_state = QFont::Normal
        port_mode = THEME_PORT_SQUARE

        port_audio_jack_pen = QPen(selPortBG, 2)
        port_audio_jack_pen_sel = QPen(QColor(1, 230, 238), 1)
        port_midi_jack_pen = QPen(selPortBG, 2)
        port_midi_jack_pen_sel = QPen(QColor(252, 118, 118), 1)
        port_midi_alsa_pen = QPen(selPortBG, 2)
        port_midi_alsa_pen_sel = QPen(QColor(129, 244, 118), 0)
        port_parameter_pen = QPen(selPortBG, 2)
        port_parameter_pen_sel = QPen(QColor(137, 76, 43), 1)

        port_audio_jack_bg = normalPortBG
        port_audio_jack_bg_sel = selPortBG
        port_midi_jack_bg = normalPortBG
        port_midi_jack_bg_sel = selPortBG
        port_midi_alsa_bg = normalPortBG
        port_midi_alsa_bg_sel = selPortBG
        port_parameter_bg = normalPortBG
        port_parameter_bg_sel = selPortBG

        port_audio_jack_text = port_text
        port_audio_jack_text_sel = port_audio_jack_pen_sel
        port_midi_jack_text = port_text
        port_midi_jack_text_sel = port_midi_jack_pen_sel
        port_midi_alsa_text = port_text
        port_midi_alsa_text_sel = port_midi_alsa_pen_sel
        port_parameter_text = port_text
        port_parameter_text_sel = port_parameter_pen_sel

        # missing, ports 2
        port_height = 21
        port_offset = 1
        port_spacing = 3
        port_spacingT = 0

        # Lines
        line_audio_jack = QColor(64, 64, 64)
        line_audio_jack_sel = QColor(1, 230, 238)
        line_audio_jack_glow = QColor(100, 200, 100)
        line_midi_jack = QColor(64, 64, 64)
        line_midi_jack_sel = QColor(252, 118, 118)
        line_midi_jack_glow = QColor(200, 100, 100)
        line_midi_alsa = QColor(64, 64, 64)
        line_midi_alsa_sel = QColor(129, 244, 118)
        line_midi_alsa_glow = QColor(100, 200, 100)
        line_parameter = QColor(64, 64, 64)
        line_parameter_sel = QColor(137+90, 76+90, 43+90)
        line_parameter_glow = QColor(166, 133, 133)

        rubberband_pen = QPen(QColor(1, 230, 238), 2, Qt::SolidLine)
        rubberband_brush = QColor(90, 90, 90, 100)
#endif

        default:
            break;
        }
    }
};

//---------------------------------------------------------------------------------------------------------------------

Theme::Theme(const Theme::List idx)
    : self(new PrivateData(idx)) {}

//---------------------------------------------------------------------------------------------------------------------

Theme::List getDefaultTheme()
{
    return Theme::THEME_MODERN_DARK;
}

const char* getThemeName(const Theme::List idx)
{
    switch (idx)
    {
    case Theme::THEME_MODERN_DARK:
        return "Modern Dark";
    case Theme::THEME_MODERN_DARK_TINY:
        return "Modern Dark (Tiny)";
    case Theme::THEME_MODERN_LIGHT:
        return "Modern Light";
    case Theme::THEME_CLASSIC_DARK:
        return "Classic Dark";
    case Theme::THEME_OOSTUDIO:
        return "OpenOctave Studio";
    default:
        return "";
    }
}

const char* getDefaultThemeName()
{
    return "Modern Dark";
}

//---------------------------------------------------------------------------------------------------------------------
