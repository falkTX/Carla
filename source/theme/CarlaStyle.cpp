/*
 * Carla Style, based on Qt5 fusion style
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies)
 * Copyright (C) 2013-2019 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the doc/LGPL.txt file
 */

#include "CarlaStylePrivate.hpp"

#include <QtCore/qmath.h>
#include <QtCore/QStringBuilder>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtGui/QPainter>
# include <QtGui/QPixmapCache>
# include <QtWidgets/qdrawutil.h>
# include <QtWidgets/QApplication>
# include <QtWidgets/QComboBox>
# include <QtWidgets/QGroupBox>
# include <QtWidgets/QMainWindow>
# include <QtWidgets/QProgressBar>
# include <QtWidgets/QPushButton>
# include <QtWidgets/QScrollBar>
# include <QtWidgets/QSlider>
# include <QtWidgets/QSpinBox>
# include <QtWidgets/QSplitter>
# include <QtWidgets/QWizard>
# define QStyleOptionFrameV3 QStyleOptionFrame
# define QStyleOptionProgressBarV2 QStyleOptionProgressBar
#else
# ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdeprecated-register"
# endif
# include <QtGui/QPainter>
# include <QtGui/QPixmapCache>
# include <QtGui/QApplication>
# include <QtGui/QComboBox>
# include <QtGui/QGroupBox>
# include <QtGui/QMainWindow>
# include <QtGui/QProgressBar>
# include <QtGui/QPushButton>
# include <QtGui/QScrollBar>
# include <QtGui/QSlider>
# include <QtGui/QSpinBox>
# include <QtGui/QSplitter>
# include <QtGui/QWizard>
# ifdef __clang__
#  pragma clang diagnostic pop
# endif
#endif

#include <cstdio>

#define BEGIN_STYLE_PIXMAPCACHE(a) \
    QRect rect = option->rect; \
    QPixmap internalPixmapCache; \
    QImage imageCache; \
    QPainter *p = painter; \
    QString unique = uniqueName((a), option, option->rect.size()); \
    int txType = painter->deviceTransform().type() | painter->worldTransform().type(); \
    bool doPixmapCache = txType <= QTransform::TxTranslate; \
    if (doPixmapCache && QPixmapCache::find(unique, internalPixmapCache)) { \
        painter->drawPixmap(option->rect.topLeft(), internalPixmapCache); \
    } else { \
        if (doPixmapCache) { \
            rect.setRect(0, 0, option->rect.width(), option->rect.height()); \
            imageCache = QImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied); \
            imageCache.fill(0); \
            p = new QPainter(&imageCache); \
        }

#define END_STYLE_PIXMAPCACHE \
        if (doPixmapCache) { \
            p->end(); \
            delete p; \
            internalPixmapCache = QPixmap::fromImage(imageCache); \
            painter->drawPixmap(option->rect.topLeft(), internalPixmapCache); \
            QPixmapCache::insert(unique, internalPixmapCache); \
        } \
    }

enum Direction {
    TopDown,
    FromLeft,
    BottomUp,
    FromRight
};

// from windows style
static const int windowsItemFrame     =  2; // menu item frame width
static const int windowsItemHMargin   =  3; // menu item hor text margin
static const int windowsItemVMargin   =  8; // menu item ver text margin
static const int windowsRightBorder   = 15; // right border on windows

static const int groupBoxBottomMargin =  0; // space below the groupbox
static const int groupBoxTopMargin    =  3;

/* XPM */
static const char * const qt_titlebar_context_help[] = {
    "10 10 3 1",
    "  c None",
    "# c #000000",
    "+ c #444444",
    "  +####+  ",
    " ###  ### ",
    " ##    ## ",
    "     +##+ ",
    "    +##   ",
    "    ##    ",
    "    ##    ",
    "          ",
    "    ##    ",
    "    ##    "};

static const qreal Q_PI = qreal(3.14159265358979323846);

// internal helper. Converts an integer value to an unique string token
template <typename T>
struct HexString
{
    HexString(const T t)
        : val(t) {}

    void write(QChar* &dest) const
    {
        const ushort hexChars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        const char* c = reinterpret_cast<const char*>(&val);

        for (uint i = 0; i < sizeof(T); ++i)
        {
            *dest++ = hexChars[*c & 0xf];
            *dest++ = hexChars[(*c & 0xf0) >> 4];
            ++c;
        }
    }

    const T val;
};

// specialization to enable fast concatenating of our string tokens to a string
template <typename T>
struct QConcatenable<HexString<T> >
{
    typedef HexString<T> type;
    enum { ExactSize = true };
    static int size(const HexString<T> &) { return sizeof(T) * 2; }
    static inline void appendTo(const HexString<T> &str, QChar *&out) { str.write(out); }
    typedef QString ConvertTo;
};

inline int qt_div_255(int x)
{
    return (x + (x>>8) + 0x80) >> 8;
}

inline QPixmap styleCachePixmap(const QSize &size)
{
    return QPixmap(size);
}

int calcBigLineSize(int radius)
{
    int bigLineSize = radius / 6;
    if (bigLineSize < 4)
        bigLineSize = 4;
    if (bigLineSize > radius / 2)
        bigLineSize = radius / 2;
    return bigLineSize;
}

static QPolygonF calcLines(const QStyleOptionSlider* dial)
{
    QPolygonF poly;
    int width = dial->rect.width();
    int height = dial->rect.height();
    qreal r = qMin(width, height) / 2;
    int bigLineSize = calcBigLineSize(int(r));

    qreal xc = width / 2 + 0.5;
    qreal yc = height / 2 + 0.5;
    const int ns = dial->tickInterval;
    if (!ns) // Invalid values may be set by Qt Designer.
        return poly;
    int notches = (dial->maximum + ns - 1 - dial->minimum) / ns;
    if (notches <= 0)
        return poly;
    if (dial->maximum < dial->minimum || dial->maximum - dial->minimum > 1000) {
        int maximum = dial->minimum + 1000;
        notches = (maximum + ns - 1 - dial->minimum) / ns;
    }

    poly.resize(2 + 2 * notches);
    int smallLineSize = bigLineSize / 2;
    for (int i = 0; i <= notches; ++i) {
        qreal angle = dial->dialWrapping ? Q_PI * 3 / 2 - i * 2 * Q_PI / notches
                  : (Q_PI * 8 - i * 10 * Q_PI / notches) / 6;
        qreal s = qSin(angle);
        qreal c = qCos(angle);
        if (i == 0 || (((ns * i) % (dial->pageStep ? dial->pageStep : 1)) == 0)) {
            poly[2 * i] = QPointF(xc + (r - bigLineSize) * c,
                                  yc - (r - bigLineSize) * s);
            poly[2 * i + 1] = QPointF(xc + r * c, yc - r * s);
        } else {
            poly[2 * i] = QPointF(xc + (r - 1 - smallLineSize) * c,
                                  yc - (r - 1 - smallLineSize) * s);
            poly[2 * i + 1] = QPointF(xc + (r - 1) * c, yc -(r - 1) * s);
        }
    }
    return poly;
}

static QPointF calcRadialPos(const QStyleOptionSlider *dial, qreal offset)
{
    const int width = dial->rect.width();
    const int height = dial->rect.height();
    const int r = qMin(width, height) / 2;
    const int currentSliderPosition = dial->upsideDown ? dial->sliderPosition : (dial->maximum - dial->sliderPosition);
    qreal a = 0;
    if (dial->maximum == dial->minimum)
        a = Q_PI / 2;
    else if (dial->dialWrapping)
        a = Q_PI * 3 / 2 - (currentSliderPosition - dial->minimum) * 2 * Q_PI
            / (dial->maximum - dial->minimum);
    else
        a = (Q_PI * 8 - (currentSliderPosition - dial->minimum) * 10 * Q_PI
            / (dial->maximum - dial->minimum)) / 6;
    qreal xc = width / 2.0;
    qreal yc = height / 2.0;
    qreal len = r - calcBigLineSize(r) - 3;
    qreal back = offset * len;
    QPointF pos(QPointF(xc + back * qCos(a), yc - back * qSin(a)));
    return pos;
}

static QString uniqueName(const QString &key, const QStyleOption *option, const QSize &size)
{
    const QStyleOptionComplex* complexOption = qstyleoption_cast<const QStyleOptionComplex *>(option);
    QString tmp = key % HexString<uint>(option->state)
                      % HexString<uint>(option->direction)
                      % HexString<uint>(complexOption ? uint(complexOption->activeSubControls) : 0u)
                      % HexString<quint64>(option->palette.cacheKey())
                      % HexString<uint>(size.width())
                      % HexString<uint>(size.height());

#ifndef QT_NO_SPINBOX
    if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
        tmp = tmp % HexString<uint>(spinBox->buttonSymbols)
                  % HexString<uint>(spinBox->stepEnabled)
                  % QLatin1Char(spinBox->frame ? '1' : '0'); ;
    }
#endif // QT_NO_SPINBOX
    return tmp;
}

// This will draw a nice and shiny QDial for us. We don't want
// all the shinyness in QWindowsStyle, hence we place it here

static void drawDial(const QStyleOptionSlider* option, QPainter* painter)
{
    QPalette pal = option->palette;
    QColor buttonColor = pal.button().color();
    const int width = option->rect.width();
    const int height = option->rect.height();
    const bool enabled = option->state & QStyle::State_Enabled;
    qreal r = qMin(width, height) / 2;
    r -= r/50;
    const qreal penSize = r/20.0;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Draw notches
    if (option->subControls & QStyle::SC_DialTickmarks) {
        painter->setPen(option->palette.dark().color().darker(120));
        painter->drawLines(calcLines(option));
    }

    // Cache dial background
    BEGIN_STYLE_PIXMAPCACHE(QString::fromLatin1("qdial"));
    p->setRenderHint(QPainter::Antialiasing);

    const qreal d_ = r / 6;
    const qreal dx = option->rect.x() + d_ + (width - 2 * r) / 2 + 1;
    const qreal dy = option->rect.y() + d_ + (height - 2 * r) / 2 + 1;

    QRectF br = QRectF(dx + 0.5, dy + 0.5,
                       int(r * 2 - 2 * d_ - 2),
                       int(r * 2 - 2 * d_ - 2));
    buttonColor.setHsv(buttonColor .hue(),
                       qMin(140, buttonColor .saturation()),
                       qMax(180, buttonColor.value()));
    QColor shadowColor(0, 0, 0, 20);

    if (enabled) {
        // Drop shadow
        qreal shadowSize = qMax(1.0, penSize/2.0);
        QRectF shadowRect= br.adjusted(-2*shadowSize, -2*shadowSize,
                                       2*shadowSize, 2*shadowSize);
        QRadialGradient shadowGradient(shadowRect.center().x(),
                                       shadowRect.center().y(), shadowRect.width()/2.0,
                                       shadowRect.center().x(), shadowRect.center().y());
        shadowGradient.setColorAt(qreal(0.91), QColor(0, 0, 0, 40));
        shadowGradient.setColorAt(qreal(1.0), Qt::transparent);
        p->setBrush(shadowGradient);
        p->setPen(Qt::NoPen);
        p->translate(shadowSize, shadowSize);
        p->drawEllipse(shadowRect);
        p->translate(-shadowSize, -shadowSize);

        // Main gradient
        QRadialGradient gradient(br.center().x() - br.width()/3, dy,
                                 br.width()*1.3, br.center().x(),
                                 br.center().y() - br.height()/2);
        gradient.setColorAt(0, buttonColor.lighter(110));
        gradient.setColorAt(qreal(0.5), buttonColor);
        gradient.setColorAt(qreal(0.501), buttonColor.darker(102));
        gradient.setColorAt(1, buttonColor.darker(115));
        p->setBrush(gradient);
    } else {
        p->setBrush(Qt::NoBrush);
    }

    p->setPen(QPen(buttonColor.darker(280)));
    p->drawEllipse(br);
    p->setBrush(Qt::NoBrush);
    p->setPen(buttonColor.lighter(110));
    p->drawEllipse(br.adjusted(1, 1, -1, -1));

    if (option->state & QStyle::State_HasFocus) {
        QColor highlight = pal.highlight().color();
        highlight.setHsv(highlight.hue(),
                         qMin(160, highlight.saturation()),
                         qMax(230, highlight.value()));
        highlight.setAlpha(127);
        p->setPen(QPen(highlight, 2.0));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(br.adjusted(-1, -1, 1, 1));
    }

    END_STYLE_PIXMAPCACHE

    QPointF dp = calcRadialPos(option, qreal(0.70));
    buttonColor = buttonColor.lighter(104);
    buttonColor.setAlphaF(qreal(0.8));
    const qreal ds = r/qreal(7.0);
    QRectF dialRect(dp.x() - ds, dp.y() - ds, 2*ds, 2*ds);
    QRadialGradient dialGradient(dialRect.center().x() + dialRect.width()/2,
                                 dialRect.center().y() + dialRect.width(),
                                 dialRect.width()*2,
                                 dialRect.center().x(), dialRect.center().y());
    dialGradient.setColorAt(1, buttonColor.darker(140));
    dialGradient.setColorAt(qreal(0.4), buttonColor.darker(120));
    dialGradient.setColorAt(0, buttonColor.darker(110));
    if (penSize > 3.0) {
        painter->setPen(QPen(QColor(0, 0, 0, 25), penSize));
        painter->drawLine(calcRadialPos(option, qreal(0.90)), calcRadialPos(option, qreal(0.96)));
    }

    painter->setBrush(dialGradient);
    painter->setPen(QColor(255, 255, 255, 150));
    painter->drawEllipse(dialRect.adjusted(-1, -1, 1, 1));
    painter->setPen(QColor(0, 0, 0, 80));
    painter->drawEllipse(dialRect);
    painter->restore();
}

static QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor = 50)
{
    const int maxFactor = 100;
    QColor tmp = colorA;
    tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
    tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
    tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
    return tmp;
}

static QPixmap colorizedImage(const QString &fileName, const QColor &color, int rotation = 0)
{
    QString pixmapName = QLatin1String("$qt_ia-") % fileName % HexString<uint>(color.rgba()) % QString::number(rotation);
    QPixmap pixmap;
    if (!QPixmapCache::find(pixmapName, pixmap)) {
        QImage image(fileName);

        if (image.format() != QImage::Format_ARGB32_Premultiplied)
            image = image.convertToFormat( QImage::Format_ARGB32_Premultiplied);

        int width = image.width();
        int height = image.height();
        int source = color.rgba();

        unsigned char sourceRed = qRed(source);
        unsigned char sourceGreen = qGreen(source);
        unsigned char sourceBlue = qBlue(source);

        for (int y = 0; y < height; ++y)
        {
            QRgb *data = (QRgb*) image.scanLine(y);
            for (int x = 0 ; x < width ; ++x) {
                QRgb col = data[x];
                unsigned int colorDiff = (qBlue(col) - qRed(col));
                unsigned char gray = qGreen(col);
                unsigned char red = gray + qt_div_255(sourceRed * colorDiff);
                unsigned char green = gray + qt_div_255(sourceGreen * colorDiff);
                unsigned char blue = gray + qt_div_255(sourceBlue * colorDiff);
                unsigned char alpha = qt_div_255(qAlpha(col) * qAlpha(source));
                data[x] = qRgba(red, green, blue, alpha);
            }
        }
        if (rotation != 0) {
            QTransform transform;
            transform.translate(-image.width()/2, -image.height()/2);
            transform.rotate(rotation);
            transform.translate(image.width()/2, image.height()/2);
            image = image.transformed(transform);
        }

        pixmap = QPixmap::fromImage(image);
        QPixmapCache::insert(pixmapName, pixmap);
    }
    return pixmap;
}

// The default button and handle gradient
static QLinearGradient qt_fusion_gradient(const QRect &rect, const QBrush &baseColor, Direction direction = TopDown)
{
    int x = rect.center().x();
    int y = rect.center().y();
    QLinearGradient gradient;
    switch (direction) {
    case FromLeft:
        gradient = QLinearGradient(rect.left(), y, rect.right(), y);
        break;
    case FromRight:
        gradient = QLinearGradient(rect.right(), y, rect.left(), y);
        break;
    case BottomUp:
        gradient = QLinearGradient(x, rect.bottom(), x, rect.top());
        break;
    case TopDown:
    default:
        gradient = QLinearGradient(x, rect.top(), x, rect.bottom());
        break;
    }
    if (baseColor.gradient())
        gradient.setStops(baseColor.gradient()->stops());
    else {
        QColor gradientStartColor = baseColor.color().lighter(124);
        QColor gradientStopColor = baseColor.color().lighter(102);
        gradient.setColorAt(0, gradientStartColor);
        gradient.setColorAt(1, gradientStopColor);
        //          Uncomment for adding shiny shading
        //            QColor midColor1 = mergedColors(gradientStartColor, gradientStopColor, 55);
        //            QColor midColor2 = mergedColors(gradientStartColor, gradientStopColor, 45);
        //            gradient.setColorAt(0.5, midColor1);
        //            gradient.setColorAt(0.501, midColor2);
    }
    return gradient;
}

static void qt_fusion_draw_mdibutton(QPainter *painter, const QStyleOptionTitleBar *option, const QRect &tmp, bool hover, bool sunken)
{
    QColor dark;
    dark.setHsv(option->palette.button().color().hue(),
                qMin(255, (int)(option->palette.button().color().saturation())),
                qMin(255, (int)(option->palette.button().color().value()*0.7)));

    QColor highlight = option->palette.highlight().color();

    bool active = (option->titleBarState & QStyle::State_Active);
    QColor titleBarHighlight(255, 255, 255, 60);

    if (sunken)
        painter->fillRect(tmp.adjusted(1, 1, -1, -1), option->palette.highlight().color().darker(120));
    else if (hover)
        painter->fillRect(tmp.adjusted(1, 1, -1, -1), QColor(255, 255, 255, 20));

    QColor mdiButtonGradientStartColor;
    QColor mdiButtonGradientStopColor;

    mdiButtonGradientStartColor = QColor(0, 0, 0, 40);
    mdiButtonGradientStopColor = QColor(255, 255, 255, 60);

    if (sunken)
        titleBarHighlight = highlight.darker(130);

    QLinearGradient gradient(tmp.center().x(), tmp.top(), tmp.center().x(), tmp.bottom());
    gradient.setColorAt(0, mdiButtonGradientStartColor);
    gradient.setColorAt(1, mdiButtonGradientStopColor);
    QColor mdiButtonBorderColor(active ? option->palette.highlight().color().darker(180): dark.darker(110));

    painter->setPen(QPen(mdiButtonBorderColor, 1));
    const QLine lines[4] = {
        QLine(tmp.left() + 2, tmp.top(), tmp.right() - 2, tmp.top()),
        QLine(tmp.left() + 2, tmp.bottom(), tmp.right() - 2, tmp.bottom()),
        QLine(tmp.left(), tmp.top() + 2, tmp.left(), tmp.bottom() - 2),
        QLine(tmp.right(), tmp.top() + 2, tmp.right(), tmp.bottom() - 2)
    };
    painter->drawLines(lines, 4);
    const QPoint points[4] = {
        QPoint(tmp.left() + 1, tmp.top() + 1),
        QPoint(tmp.right() - 1, tmp.top() + 1),
        QPoint(tmp.left() + 1, tmp.bottom() - 1),
        QPoint(tmp.right() - 1, tmp.bottom() - 1)
    };
    painter->drawPoints(points, 4);

    painter->setPen(titleBarHighlight);
    painter->drawLine(tmp.left() + 2, tmp.top() + 1, tmp.right() - 2, tmp.top() + 1);
    painter->drawLine(tmp.left() + 1, tmp.top() + 2, tmp.left() + 1, tmp.bottom() - 2);

    painter->setPen(QPen(gradient, 1));
    painter->drawLine(tmp.right() + 1, tmp.top() + 2, tmp.right() + 1, tmp.bottom() - 2);
    painter->drawPoint(tmp.right() , tmp.top() + 1);

    painter->drawLine(tmp.left() + 2, tmp.bottom() + 1, tmp.right() - 2, tmp.bottom() + 1);
    painter->drawPoint(tmp.left() + 1, tmp.bottom());
    painter->drawPoint(tmp.right() - 1, tmp.bottom());
    painter->drawPoint(tmp.right() , tmp.bottom() - 1);
}


CarlaStyle::CarlaStyle()
    : QCommonStyle(),
      d(new CarlaStylePrivate(this))
{
    setObjectName(QLatin1String("CarlaStyle"));

#if 0
    fPalSystem = app->palette();

    fPalBlack.setColor(QPalette::Disabled, QPalette::Window, QColor(14, 14, 14));
    fPalBlack.setColor(QPalette::Active,   QPalette::Window, QColor(17, 17, 17));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Window, QColor(17, 17, 17));
    fPalBlack.setColor(QPalette::Disabled, QPalette::WindowText, QColor(83, 83, 83));
    fPalBlack.setColor(QPalette::Active,   QPalette::WindowText, QColor(240, 240, 240));
    fPalBlack.setColor(QPalette::Inactive, QPalette::WindowText, QColor(240, 240, 240));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Base, QColor(6, 6, 6));
    fPalBlack.setColor(QPalette::Active,   QPalette::Base, QColor(7, 7, 7));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Base, QColor(7, 7, 7));
    fPalBlack.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(12, 12, 12));
    fPalBlack.setColor(QPalette::Active,   QPalette::AlternateBase, QColor(14, 14, 14));
    fPalBlack.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(14, 14, 14));
    fPalBlack.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(4, 4, 4));
    fPalBlack.setColor(QPalette::Active,   QPalette::ToolTipBase, QColor(4, 4, 4));
    fPalBlack.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(4, 4, 4));
    fPalBlack.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(230, 230, 230));
    fPalBlack.setColor(QPalette::Active,   QPalette::ToolTipText, QColor(230, 230, 230));
    fPalBlack.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(230, 230, 230));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Text, QColor(74, 74, 74));
    fPalBlack.setColor(QPalette::Active,   QPalette::Text, QColor(230, 230, 230));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Text, QColor(230, 230, 230));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Button, QColor(24, 24, 24));
    fPalBlack.setColor(QPalette::Active,   QPalette::Button, QColor(28, 28, 28));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Button, QColor(28, 28, 28));
    fPalBlack.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(90, 90, 90));
    fPalBlack.setColor(QPalette::Active,   QPalette::ButtonText, QColor(240, 240, 240));
    fPalBlack.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(240, 240, 240));
    fPalBlack.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
    fPalBlack.setColor(QPalette::Active,   QPalette::BrightText, QColor(255, 255, 255));
    fPalBlack.setColor(QPalette::Inactive, QPalette::BrightText, QColor(255, 255, 255));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Light, QColor(191, 191, 191));
    fPalBlack.setColor(QPalette::Active,   QPalette::Light, QColor(191, 191, 191));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Light, QColor(191, 191, 191));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Midlight, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Active,   QPalette::Midlight, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Midlight, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Dark, QColor(129, 129, 129));
    fPalBlack.setColor(QPalette::Active,   QPalette::Dark, QColor(129, 129, 129));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Dark, QColor(129, 129, 129));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Mid, QColor(94, 94, 94));
    fPalBlack.setColor(QPalette::Active,   QPalette::Mid, QColor(94, 94, 94));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Mid, QColor(94, 94, 94));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Shadow, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Active,   QPalette::Shadow, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Shadow, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Highlight, QColor(14, 14, 14));
    fPalBlack.setColor(QPalette::Active,   QPalette::Highlight, QColor(60, 60, 60));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Highlight, QColor(34, 34, 34));
    fPalBlack.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(83, 83, 83));
    fPalBlack.setColor(QPalette::Active,   QPalette::HighlightedText, QColor(255, 255, 255));
    fPalBlack.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(240, 240, 240));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Link, QColor(34, 34, 74));
    fPalBlack.setColor(QPalette::Active,   QPalette::Link, QColor(100, 100, 230));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Link, QColor(100, 100, 230));
    fPalBlack.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(74, 34, 74));
    fPalBlack.setColor(QPalette::Active,   QPalette::LinkVisited, QColor(230, 100, 230));
    fPalBlack.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(230, 100, 230));

    fPalBlue.setColor(QPalette::Disabled, QPalette::Window, QColor(32, 35, 39));
    fPalBlue.setColor(QPalette::Active,   QPalette::Window, QColor(37, 40, 45));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Window, QColor(37, 40, 45));
    fPalBlue.setColor(QPalette::Disabled, QPalette::WindowText, QColor(89, 95, 104));
    fPalBlue.setColor(QPalette::Active,   QPalette::WindowText, QColor(223, 237, 255));
    fPalBlue.setColor(QPalette::Inactive, QPalette::WindowText, QColor(223, 237, 255));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Base, QColor(48, 53, 60));
    fPalBlue.setColor(QPalette::Active,   QPalette::Base, QColor(55, 61, 69));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Base, QColor(55, 61, 69));
    fPalBlue.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(60, 64, 67));
    fPalBlue.setColor(QPalette::Active,   QPalette::AlternateBase, QColor(69, 73, 77));
    fPalBlue.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(69, 73, 77));
    fPalBlue.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(182, 193, 208));
    fPalBlue.setColor(QPalette::Active,   QPalette::ToolTipBase, QColor(182, 193, 208));
    fPalBlue.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(182, 193, 208));
    fPalBlue.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(42, 44, 48));
    fPalBlue.setColor(QPalette::Active,   QPalette::ToolTipText, QColor(42, 44, 48));
    fPalBlue.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(42, 44, 48));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Text, QColor(96, 103, 113));
    fPalBlue.setColor(QPalette::Active,   QPalette::Text, QColor(210, 222, 240));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Text, QColor(210, 222, 240));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Button, QColor(51, 55, 62));
    fPalBlue.setColor(QPalette::Active,   QPalette::Button, QColor(59, 63, 71));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Button, QColor(59, 63, 71));
    fPalBlue.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(98, 104, 114));
    fPalBlue.setColor(QPalette::Active,   QPalette::ButtonText, QColor(210, 222, 240));
    fPalBlue.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(210, 222, 240));
    fPalBlue.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
    fPalBlue.setColor(QPalette::Active,   QPalette::BrightText, QColor(255, 255, 255));
    fPalBlue.setColor(QPalette::Inactive, QPalette::BrightText, QColor(255, 255, 255));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Light, QColor(59, 64, 72));
    fPalBlue.setColor(QPalette::Active,   QPalette::Light, QColor(63, 68, 76));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Light, QColor(63, 68, 76));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Midlight, QColor(48, 52, 59));
    fPalBlue.setColor(QPalette::Active,   QPalette::Midlight, QColor(51, 56, 63));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Midlight, QColor(51, 56, 63));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Dark, QColor(18, 19, 22));
    fPalBlue.setColor(QPalette::Active,   QPalette::Dark, QColor(20, 22, 25));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Dark, QColor(20, 22, 25));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Mid, QColor(28, 30, 34));
    fPalBlue.setColor(QPalette::Active,   QPalette::Mid, QColor(32, 35, 39));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Mid, QColor(32, 35, 39));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Shadow, QColor(13, 14, 16));
    fPalBlue.setColor(QPalette::Active,   QPalette::Shadow, QColor(15, 16, 18));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Shadow, QColor(15, 16, 18));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Highlight, QColor(32, 35, 39));
    fPalBlue.setColor(QPalette::Active,   QPalette::Highlight, QColor(14, 14, 17));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Highlight, QColor(27, 28, 33));
    fPalBlue.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(89, 95, 104));
    fPalBlue.setColor(QPalette::Active,   QPalette::HighlightedText, QColor(217, 234, 253));
    fPalBlue.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(223, 237, 255));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Link, QColor(79, 100, 118));
    fPalBlue.setColor(QPalette::Active,   QPalette::Link, QColor(156, 212, 255));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Link, QColor(156, 212, 255));
    fPalBlue.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(51, 74, 118));
    fPalBlue.setColor(QPalette::Active,   QPalette::LinkVisited, QColor(64, 128, 255));
    fPalBlue.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(64, 128, 255));
#endif
}

CarlaStyle::~CarlaStyle()
{
    delete d;
}

void printPalette(const QPalette& pal)
{
#define PAL "fPalBlue"

#define PAL_PRINT(ROLE) \
{ \
    QColor color1(pal.color(QPalette::Disabled, ROLE)); \
    QColor color2(pal.color(QPalette::Active,   ROLE)); \
    QColor color3(pal.color(QPalette::Inactive, ROLE)); \
    printf(PAL ".setColor(QPalette::Disabled, " #ROLE ", QColor(%i, %i, %i));\n", color1.red(), color1.green(), color1.blue()); \
    printf(PAL ".setColor(QPalette::Active,   " #ROLE ", QColor(%i, %i, %i));\n", color2.red(), color2.green(), color2.blue()); \
    printf(PAL ".setColor(QPalette::Inactive, " #ROLE ", QColor(%i, %i, %i));\n", color3.red(), color3.green(), color3.blue()); \
}

    PAL_PRINT(QPalette::Window)
    PAL_PRINT(QPalette::WindowText)
    PAL_PRINT(QPalette::Base)
    PAL_PRINT(QPalette::AlternateBase)
    PAL_PRINT(QPalette::ToolTipBase)
    PAL_PRINT(QPalette::ToolTipText)
    PAL_PRINT(QPalette::Text)
    PAL_PRINT(QPalette::Button)
    PAL_PRINT(QPalette::ButtonText)
    PAL_PRINT(QPalette::BrightText)
    PAL_PRINT(QPalette::Light)
    PAL_PRINT(QPalette::Midlight)
    PAL_PRINT(QPalette::Dark)
    PAL_PRINT(QPalette::Mid)
    PAL_PRINT(QPalette::Shadow)
    PAL_PRINT(QPalette::Highlight)
    PAL_PRINT(QPalette::HighlightedText)
    PAL_PRINT(QPalette::Link)
    PAL_PRINT(QPalette::LinkVisited)

#undef PAL
}

/*!
    \fn void CarlaStyle::drawItemText(QPainter *painter, const QRect &rectangle, int alignment, const QPalette &palette,
                                    bool enabled, const QString& text, QPalette::ColorRole textRole) const

    Draws the given \a text in the specified \a rectangle using the
    provided \a painter and \a palette.

    Text is drawn using the painter's pen. If an explicit \a textRole
    is specified, then the text is drawn using the \a palette's color
    for the specified role.  The \a enabled value indicates whether or
    not the item is enabled; when reimplementing, this value should
    influence how the item is drawn.

    The text is aligned and wrapped according to the specified \a
    alignment.

    \sa Qt::Alignment
*/
void CarlaStyle::drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                                bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
    if (text.isEmpty())
        return;

    QPen savedPen = painter->pen();
    if (textRole != QPalette::NoRole) {
        painter->setPen(QPen(pal.brush(textRole), savedPen.widthF()));
    }
    if (!enabled) {
        QPen pen = painter->pen();
        painter->setPen(pen);
    }
    painter->drawText(rect, alignment, text);
    painter->setPen(savedPen);
}

/*!
    \reimp
*/
void CarlaStyle::drawPrimitive(PrimitiveElement elem,
                                 const QStyleOption *option,
                                 QPainter *painter, const QWidget *widget) const
{
    Q_ASSERT(option);

    QRect rect = option->rect;
    int state = option->state;

    QColor outline = d->outline(option->palette);
    QColor highlightedOutline = d->highlightedOutline(option->palette);

    QColor tabFrameColor = d->tabFrameColor(option->palette);

    switch (elem) {

    // No frame drawn
    case PE_FrameGroupBox:
    {
        QPixmap pixmap(QLatin1String(":/bitmaps/style/groupbox.png"));
        int topMargin = qMax(pixelMetric(PM_ExclusiveIndicatorHeight), option->fontMetrics.height()) + groupBoxTopMargin;
        QRect frame = option->rect.adjusted(0, topMargin, 0, 0);
        qDrawBorderPixmap(painter, frame, QMargins(6, 6, 6, 6), pixmap);
        break;
    }
    case PE_IndicatorBranch: {
        if (!(option->state & State_Children))
            break;
        if (option->state & State_Open)
            drawPrimitive(PE_IndicatorArrowDown, option, painter, widget);
        else
            drawPrimitive(PE_IndicatorArrowRight, option, painter, widget);
        break;
    }
    case PE_FrameTabBarBase:
        if (const QStyleOptionTabBarBase *tbb
                = qstyleoption_cast<const QStyleOptionTabBarBase *>(option)) {
            painter->save();
            painter->setPen(QPen(outline.lighter(110), 0));
            switch (tbb->shape) {
            case QTabBar::RoundedNorth: {
                QRegion region(tbb->rect);
                region -= tbb->selectedTabRect;
                painter->drawLine(tbb->rect.topLeft(), tbb->rect.topRight());
                painter->setClipRegion(region);
                painter->setPen(option->palette.light().color());
                painter->drawLine(tbb->rect.topLeft() + QPoint(0, 1), tbb->rect.topRight() + QPoint(0, 1));
            }
                break;
            case QTabBar::RoundedWest:
                painter->drawLine(tbb->rect.left(), tbb->rect.top(), tbb->rect.left(), tbb->rect.bottom());
                break;
            case QTabBar::RoundedSouth:
                painter->drawLine(tbb->rect.left(), tbb->rect.bottom(),
                                  tbb->rect.right(), tbb->rect.bottom());
                break;
            case QTabBar::RoundedEast:
                painter->drawLine(tbb->rect.topRight(), tbb->rect.bottomRight());
                break;
            case QTabBar::TriangularNorth:
            case QTabBar::TriangularEast:
            case QTabBar::TriangularWest:
            case QTabBar::TriangularSouth:
                painter->restore();
                QCommonStyle::drawPrimitive(elem, option, painter, widget);
                return;
            }
            painter->restore();
        }
        return;
    case PE_PanelScrollAreaCorner: {
        painter->save();
        QColor alphaOutline = outline;
        alphaOutline.setAlpha(180);
        painter->setPen(alphaOutline);
        painter->setBrush(option->palette.brush(QPalette::Window));
        painter->drawRect(option->rect);
        painter->restore();
    } break;
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowRight:
    case PE_IndicatorArrowLeft:
    {
        if (option->rect.width() <= 1 || option->rect.height() <= 1)
            break;
        QColor arrowColor = option->palette.foreground().color();
        QPixmap arrow;
        int rotation = 0;
        switch (elem) {
        case PE_IndicatorArrowDown:
            rotation = 180;
            break;
        case PE_IndicatorArrowRight:
            rotation = 90;
            break;
        case PE_IndicatorArrowLeft:
            rotation = -90;
            break;
        default:
            break;
        }
        arrow = colorizedImage(QLatin1String(":/bitmaps/style/arrow.png"), arrowColor, rotation);
        QRect rect = option->rect;
        QRect arrowRect;
        int imageMax = qMin(arrow.height(), arrow.width());
        int rectMax = qMin(rect.height(), rect.width());
        int size = qMin(imageMax, rectMax);

        arrowRect.setWidth(size);
        arrowRect.setHeight(size);
        if (arrow.width() > arrow.height())
            arrowRect.setHeight(arrow.height() * size / arrow.width());
        else
            arrowRect.setWidth(arrow.width() * size / arrow.height());

        arrowRect.moveTopLeft(rect.center() - arrowRect.center());
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        painter->drawPixmap(arrowRect, arrow);
        painter->restore();
    }
        break;
    case PE_IndicatorViewItemCheck:
    {
        QStyleOptionButton button;
        button.QStyleOption::operator=(*option);
        button.state &= ~State_MouseOver;
        proxy()->drawPrimitive(PE_IndicatorCheckBox, &button, painter, widget);
    }
        return;
    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            QRect r = header->rect;
            QPixmap arrow;
            QColor arrowColor = header->palette.foreground().color();
            QPoint offset = QPoint(0, -1);

            if (header->sortIndicator & QStyleOptionHeader::SortUp) {
                arrow = colorizedImage(QLatin1String(":/bitmaps/style/arrow.png"), arrowColor);
            } else if (header->sortIndicator & QStyleOptionHeader::SortDown) {
                arrow = colorizedImage(QLatin1String(":/bitmaps/style/arrow.png"), arrowColor, 180);
            } if (!arrow.isNull()) {
                r.setSize(QSize(arrow.width()/2, arrow.height()/2));
                r.moveCenter(header->rect.center());
                painter->drawPixmap(r.translated(offset), arrow);
            }
        }
        break;
    case PE_IndicatorButtonDropDown:
        proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
        break;

    case PE_IndicatorToolBarSeparator:
    {
        QRect rect = option->rect;
        const int margin = 6;
        QColor separator_color = option->palette.text().color();
        separator_color.setAlpha(50);
        painter->setPen(QPen(separator_color));
        if (option->state & State_Horizontal) {
            const int offset = rect.width()/2;

            painter->drawLine(rect.bottomLeft().x() + offset,
                              rect.bottomLeft().y() - margin,
                              rect.topLeft().x() + offset,
                              rect.topLeft().y() + margin);
            painter->setPen(QPen(option->palette.background().color().lighter(110)));
            painter->drawLine(rect.bottomLeft().x() + offset + 1,
                              rect.bottomLeft().y() - margin,
                              rect.topLeft().x() + offset + 1,
                              rect.topLeft().y() + margin);
        } else { //Draw vertical separator
            const int offset = rect.height()/2;
            painter->drawLine(rect.topLeft().x() + margin ,
                              rect.topLeft().y() + offset,
                              rect.topRight().x() - margin,
                              rect.topRight().y() + offset);
            painter->setPen(QPen(option->palette.background().color().lighter(110)));
            painter->drawLine(rect.topLeft().x() + margin ,
                              rect.topLeft().y() + offset + 1,
                              rect.topRight().x() - margin,
                              rect.topRight().y() + offset + 1);
        }
    }
        break;
    case PE_Frame:
        if (widget && widget->inherits("QComboBoxPrivateContainer")){
            QStyleOption copy = *option;
            copy.state |= State_Raised;
            proxy()->drawPrimitive(PE_PanelMenu, &copy, painter, widget);
            break;
        }
        painter->save();
        painter->setPen(outline.lighter(108));
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->restore();
        break;
    case PE_FrameMenu:
        painter->save();
    {
        painter->setPen(QPen(outline, 1));
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        QColor frameLight = option->palette.background().color().lighter(160);
        QColor frameShadow = option->palette.background().color().darker(110);

        //paint beveleffect
        QRect frame = option->rect.adjusted(1, 1, -1, -1);
        painter->setPen(frameLight);
        painter->drawLine(frame.topLeft(), frame.bottomLeft());
        painter->drawLine(frame.topLeft(), frame.topRight());

        painter->setPen(frameShadow);
        painter->drawLine(frame.topRight(), frame.bottomRight());
        painter->drawLine(frame.bottomLeft(), frame.bottomRight());
    }
        painter->restore();
        break;
    case PE_FrameDockWidget:

        painter->save();
    {
        QColor softshadow = option->palette.background().color().darker(120);

        QRect rect= option->rect;
        painter->setPen(softshadow);
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->setPen(QPen(option->palette.light(), 0));
        painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1), QPoint(rect.left() + 1, rect.bottom() - 1));
        painter->setPen(QPen(option->palette.background().color().darker(120), 0));
        painter->drawLine(QPoint(rect.left() + 1, rect.bottom() - 1), QPoint(rect.right() - 2, rect.bottom() - 1));
        painter->drawLine(QPoint(rect.right() - 1, rect.top() + 1), QPoint(rect.right() - 1, rect.bottom() - 1));

    }
        painter->restore();
        break;
    case PE_PanelButtonTool:
        painter->save();
        if ((option->state & State_Enabled || option->state & State_On) || !(option->state & State_AutoRaise)) {
            if (widget && widget->inherits("QDockWidgetTitleButton")) {
                if (option->state & State_MouseOver)
                    proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
            } else {
                proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
            }
        }
        painter->restore();
        break;
    case PE_IndicatorDockWidgetResizeHandle:
    {
        QStyleOption dockWidgetHandle = *option;
        bool horizontal = option->state & State_Horizontal;
        if (horizontal)
            dockWidgetHandle.state &= ~State_Horizontal;
        else
            dockWidgetHandle.state |= State_Horizontal;
        proxy()->drawControl(CE_Splitter, &dockWidgetHandle, painter, widget);
    }
        break;
    case PE_FrameWindow:
        painter->save();
    {
        QRect rect= option->rect;
        painter->setPen(QPen(outline.darker(150), 0));
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->setPen(QPen(option->palette.light(), 0));
        painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1),
                          QPoint(rect.left() + 1, rect.bottom() - 1));
        painter->setPen(QPen(option->palette.background().color().darker(120), 0));
        painter->drawLine(QPoint(rect.left() + 1, rect.bottom() - 1),
                          QPoint(rect.right() - 2, rect.bottom() - 1));
        painter->drawLine(QPoint(rect.right() - 1, rect.top() + 1),
                          QPoint(rect.right() - 1, rect.bottom() - 1));
    }
        painter->restore();
        break;
    case PE_FrameLineEdit:
    {
        QRect r = rect;
        bool hasFocus = option->state & State_HasFocus;

        painter->save();

        painter->setRenderHint(QPainter::Antialiasing, true);
        //  ### highdpi painter bug.
        painter->translate(0.5, 0.5);

        // Draw Outline
        painter->setPen( QPen(hasFocus ? highlightedOutline : highlightedOutline.darker(160), 0));
        painter->setBrush(option->palette.base());
        painter->drawRoundedRect(r.adjusted(0, 0, -1, -1), 2, 2);

        if (hasFocus) {
            QColor softHighlight = highlightedOutline;
            softHighlight.setAlpha(40);
            painter->setPen(softHighlight);
            painter->drawRoundedRect(r.adjusted(1, 1, -2, -2), 1.7, 1.7);
        }
        // Draw inner shadow
        painter->setPen(d->topShadow());
        painter->drawLine(QPoint(r.left() + 2, r.top() + 1), QPoint(r.right() - 2, r.top() + 1));

        painter->restore();

    }
        break;
    case PE_IndicatorCheckBox:
        painter->save();
        if (const QStyleOptionButton *checkbox = qstyleoption_cast<const QStyleOptionButton*>(option)) {
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);
            rect = rect.adjusted(0, 0, -1, -1);

            const QColor& baseColor = option->palette.base().color();

            QColor pressedColor = mergedColors(baseColor, option->palette.foreground().color(), 85);
            painter->setBrush(Qt::NoBrush);

            // Gradient fill
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());

            if (state & State_Sunken)
            {
                gradient.setColorAt(0, pressedColor);
                gradient.setColorAt(0.15, pressedColor);
                gradient.setColorAt(1, pressedColor);
            }
            else
            {
                gradient.setColorAt(0, baseColor.blackF() > 0.4 ? baseColor.lighter(115) : baseColor.darker(115));
                gradient.setColorAt(0.15, baseColor);
                gradient.setColorAt(1, baseColor);
            }

            painter->setBrush((state & State_Sunken) ? QBrush(pressedColor) : gradient);

            if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange)
                painter->setPen(QPen(highlightedOutline, 1));
            else
                painter->setPen(QPen(outline.lighter(110), 1));

            painter->drawRect(rect);

            QColor checkMarkColor = option->palette.text().color().darker(120);

            if (checkbox->state & State_NoChange) {
                gradient = QLinearGradient(rect.topLeft(), rect.bottomLeft());
                checkMarkColor.setAlpha(80);
                gradient.setColorAt(0, checkMarkColor);
                checkMarkColor.setAlpha(140);
                gradient.setColorAt(1, checkMarkColor);
                checkMarkColor.setAlpha(180);
                painter->setPen(QPen(checkMarkColor, 1));
                painter->setBrush(gradient);
                painter->drawRect(rect.adjusted(3, 3, -3, -3));

            } else if (checkbox->state & (State_On)) {
                QPen checkPen = QPen(checkMarkColor, 1.8);
                checkMarkColor.setAlpha(210);
                painter->translate(-1, 0.5);
                painter->setPen(checkPen);
                painter->setBrush(Qt::NoBrush);
                painter->translate(0.2, 0.0);

                // Draw checkmark
                QPainterPath path;
                path.moveTo(5, rect.height() / 2.0);
                path.lineTo(rect.width() / 2.0 - 0, rect.height() - 3);
                path.lineTo(rect.width() - 2.5, 3);
                painter->drawPath(path.translated(rect.topLeft()));
            }
        }
        painter->restore();
        break;
    case PE_IndicatorRadioButton:
        painter->save();
    {
        QColor pressedColor = mergedColors(option->palette.base().color(), option->palette.foreground().color(), 85);
        painter->setBrush((state & State_Sunken) ? pressedColor : option->palette.base().color());
        painter->setRenderHint(QPainter::Antialiasing, true);
        QPainterPath circle;
        circle.addEllipse(rect.center() + QPoint(1.0, 1.0), 6.5, 6.5);
        painter->setPen(QPen(option->palette.background().color().darker(150), 1));
        if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange)
            painter->setPen(QPen(highlightedOutline, 1));
        painter->drawPath(circle);

        if (state & (State_On )) {
            circle = QPainterPath();
            circle.addEllipse(rect.center() + QPoint(1, 1), 2.8, 2.8);
            QColor checkMarkColor = option->palette.text().color().darker(120);
            checkMarkColor.setAlpha(200);
            painter->setPen(checkMarkColor);
            checkMarkColor.setAlpha(180);
            painter->setBrush(checkMarkColor);
            painter->drawPath(circle);
        }
    }
        painter->restore();
        break;
    case PE_IndicatorToolBarHandle:
    {
        //draw grips
        if (option->state & State_Horizontal) {
            for (int i = -3 ; i < 2 ; i += 3) {
                for (int j = -8 ; j < 10 ; j += 3) {
                    painter->fillRect(rect.center().x() + i, rect.center().y() + j, 2, 2, d->lightShade());
                    painter->fillRect(rect.center().x() + i, rect.center().y() + j, 1, 1, d->darkShade());
                }
            }
        } else { //vertical toolbar
            for (int i = -6 ; i < 12 ; i += 3) {
                for (int j = -3 ; j < 2 ; j += 3) {
                    painter->fillRect(rect.center().x() + i, rect.center().y() + j, 2, 2, d->lightShade());
                    painter->fillRect(rect.center().x() + i, rect.center().y() + j, 1, 1, d->darkShade());
                }
            }
        }
        break;
    }
    case PE_FrameDefaultButton:
        break;
    case PE_FrameFocusRect:
        if (const QStyleOptionFocusRect *fropt = qstyleoption_cast<const QStyleOptionFocusRect *>(option)) {
            //### check for d->alt_down
            if (!(fropt->state & State_KeyboardFocusChange))
                return;
            QRect rect = option->rect;

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);
            QColor fillcolor = highlightedOutline;
            fillcolor.setAlpha(80);
            painter->setPen(fillcolor.darker(120));
            fillcolor.setAlpha(30);
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0, fillcolor.lighter(160));
            gradient.setColorAt(1, fillcolor);
            painter->setBrush(gradient);
            painter->drawRoundedRect(option->rect.adjusted(0, 0, -1, -1), 1, 1);
            painter->restore();
        }
        break;
    case PE_PanelButtonCommand:
    {
        bool isDefault = false;
        bool isFlat = false;
        bool isDown = (option->state & State_Sunken) || (option->state & State_On);
        QRect r;

        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton*>(option)) {
            isDefault = (button->features & QStyleOptionButton::DefaultButton) && (button->state & State_Enabled);
            isFlat = (button->features & QStyleOptionButton::Flat);
        }

        if (isFlat && !isDown) {
            if (isDefault) {
                r = option->rect.adjusted(0, 1, 0, -1);
                painter->setPen(QPen(Qt::black, 0));
                const QLine lines[4] = {
                    QLine(QPoint(r.left() + 2, r.top()),
                    QPoint(r.right() - 2, r.top())),
                    QLine(QPoint(r.left(), r.top() + 2),
                    QPoint(r.left(), r.bottom() - 2)),
                    QLine(QPoint(r.right(), r.top() + 2),
                    QPoint(r.right(), r.bottom() - 2)),
                    QLine(QPoint(r.left() + 2, r.bottom()),
                    QPoint(r.right() - 2, r.bottom()))
                };
                painter->drawLines(lines, 4);
                const QPoint points[4] = {
                    QPoint(r.right() - 1, r.bottom() - 1),
                    QPoint(r.right() - 1, r.top() + 1),
                    QPoint(r.left() + 1, r.bottom() - 1),
                    QPoint(r.left() + 1, r.top() + 1)
                };
                painter->drawPoints(points, 4);
            }
            return;
        }

        BEGIN_STYLE_PIXMAPCACHE(QString::fromLatin1("pushbutton-%1").arg(isDefault))
                r = rect.adjusted(0, 1, -1, 0);

        bool isEnabled = option->state & State_Enabled;
        bool hasFocus = (option->state & State_HasFocus && option->state & State_KeyboardFocusChange);
        QColor buttonColor = d->buttonColor(option->palette);

        QColor darkOutline = outline;
        if (hasFocus || isDefault) {
            darkOutline = highlightedOutline;
        }

        if (isDefault)
            buttonColor = mergedColors(buttonColor, highlightedOutline.lighter(130), 90);

        p->setRenderHint(QPainter::Antialiasing, true);
        p->translate(0.5, -0.5);

        QLinearGradient gradient = qt_fusion_gradient(rect, (isEnabled && option->state & State_MouseOver ) ? buttonColor : buttonColor.darker(104));
        p->setBrush(isDown ? QBrush(buttonColor.darker(110)) : gradient);
        p->setPen(QPen(p->brush(), 1));
        p->drawRoundedRect(r.adjusted(1,1,-1,-1), 1.8, 1.8);
        p->setBrush(Qt::NoBrush);

        // Outline
        p->setPen(!isEnabled ? QPen(darkOutline.lighter(115)) : QPen(darkOutline, 1));
        p->drawRoundedRect(r, 2.5, 2.5);

        p->setPen(d->innerContrastLine());
        p->drawRoundedRect(r.adjusted(1, 1, -1, -1), 1.8, 1.8);

        END_STYLE_PIXMAPCACHE
        }
        break;
    case PE_FrameTabWidget:
        painter->save();
        painter->fillRect(option->rect.adjusted(0, 0, -1, -1), tabFrameColor);
        if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)) {
            QColor borderColor = outline.lighter(110);
            QRect rect = option->rect.adjusted(0, 0, -1, -1);

            // Shadow outline
            if (twf->shape != QTabBar::RoundedSouth) {
                rect.adjust(0, 0, 0, -1);
                QColor alphaShadow(Qt::black);
                alphaShadow.setAlpha(15);
                painter->setPen(alphaShadow);
                painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());            painter->setPen(borderColor);
            }

            // outline
            painter->setPen(outline);
            painter->drawRect(rect);

            // Inner frame highlight
            painter->setPen(d->innerContrastLine());
            painter->drawRect(rect.adjusted(1, 1, -1, -1));

        }
        painter->restore();
        break ;

    case PE_FrameStatusBarItem:
        break;
    case PE_IndicatorTabClose:
    {
        if (d->tabBarcloseButtonIcon.isNull())
            d->tabBarcloseButtonIcon = standardIcon(SP_DialogCloseButton, option, widget);
        if ((option->state & State_Enabled) && (option->state & State_MouseOver))
            proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
        QPixmap pixmap = d->tabBarcloseButtonIcon.pixmap(QSize(16, 16), QIcon::Normal, QIcon::On);
        proxy()->drawItemPixmap(painter, option->rect, Qt::AlignCenter, pixmap);
    }
        break;
    case PE_PanelMenu: {
        painter->save();
        const QBrush menuBackground = option->palette.base().color().lighter(108);
        QColor borderColor(32, 32, 32);
        qDrawPlainRect(painter, option->rect, borderColor, 1, &menuBackground);
        painter->restore();
    }
        break;

    default:
        QCommonStyle::drawPrimitive(elem, option, painter, widget);
        break;
    }
}

/*!
  \reimp
*/
void CarlaStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                               const QWidget *widget) const
{
    QRect rect = option->rect;
    QColor outline = d->outline(option->palette);
    QColor highlightedOutline = d->highlightedOutline(option->palette);
    QColor shadow = d->darkShade();

    switch (element) {
    case CE_ComboBoxLabel:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            QRect editRect = proxy()->subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, widget);
            painter->save();
            painter->setClipRect(editRect);
            if (!cb->currentIcon.isNull()) {
                QIcon::Mode mode = cb->state & State_Enabled ? QIcon::Normal
                                                             : QIcon::Disabled;
                QPixmap pixmap = cb->currentIcon.pixmap(cb->iconSize, mode);
                QRect iconRect(editRect);
                iconRect.setWidth(cb->iconSize.width() + 4);
                iconRect = alignedRect(cb->direction,
                                       Qt::AlignLeft | Qt::AlignVCenter,
                                       iconRect.size(), editRect);
                if (cb->editable)
                    painter->fillRect(iconRect, cb->palette.brush(QPalette::Base));
                proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);

                if (cb->direction == Qt::RightToLeft)
                    editRect.translate(-4 - cb->iconSize.width(), 0);
                else
                    editRect.translate(cb->iconSize.width() + 4, 0);
            }
            if (!cb->currentText.isEmpty() && !cb->editable) {
                proxy()->drawItemText(painter, editRect.adjusted(1, 0, -1, 0),
                                      visualAlignment(cb->direction, Qt::AlignLeft | Qt::AlignVCenter),
                                      cb->palette, cb->state & State_Enabled, cb->currentText,
                                      cb->editable ? QPalette::Text : QPalette::ButtonText);
            }
            painter->restore();
        }
        break;
    case CE_Splitter:
    {
        // Don't draw handle for single pixel splitters
        if (option->rect.width() > 1 && option->rect.height() > 1) {
            //draw grips
            if (option->state & State_Horizontal) {
                for (int j = -6 ; j< 12 ; j += 3) {
                    painter->fillRect(rect.center().x() + 1, rect.center().y() + j, 2, 2, d->lightShade());
                    painter->fillRect(rect.center().x() + 1, rect.center().y() + j, 1, 1, d->darkShade());
                }
            } else {
                for (int i = -6; i< 12 ; i += 3) {
                    painter->fillRect(rect.center().x() + i, rect.center().y(), 2, 2, d->lightShade());
                    painter->fillRect(rect.center().x() + i, rect.center().y(), 1, 1, d->darkShade());
                }
            }
        }
        break;
    }
    case CE_RubberBand:
        if (qstyleoption_cast<const QStyleOptionRubberBand *>(option)) {
            QColor highlight = option->palette.color(QPalette::Active, QPalette::Highlight);
            painter->save();
            QColor penColor = highlight.darker(120);
            penColor.setAlpha(180);
            painter->setPen(penColor);
            QColor dimHighlight(qMin(highlight.red()/2 + 110, 255),
                                qMin(highlight.green()/2 + 110, 255),
                                qMin(highlight.blue()/2 + 110, 255));
            dimHighlight.setAlpha(widget && widget->isTopLevel() ? 255 : 80);
            QLinearGradient gradient(rect.topLeft(), QPoint(rect.bottomLeft().x(), rect.bottomLeft().y()));
            gradient.setColorAt(0, dimHighlight.lighter(120));
            gradient.setColorAt(1, dimHighlight);
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);
            painter->setBrush(dimHighlight);
            painter->drawRoundedRect(option->rect.adjusted(0, 0, -1, -1), 1.3, 1.3);
            QColor innerLine = Qt::white;
            innerLine.setAlpha(40);
            painter->setPen(innerLine);
            painter->drawRoundedRect(option->rect.adjusted(1, 1, -2, -2), 0.7, 0.7);
            painter->restore();
            return;
        }
        break;
    case CE_SizeGrip:
        painter->save();
    {
        //draw grips
        for (int i = -6; i< 12 ; i += 3) {
            for (int j = -6 ; j< 12 ; j += 3) {
                if ((option->direction == Qt::LeftToRight && i > -j) || (option->direction == Qt::RightToLeft && j > i) ) {
                    painter->fillRect(rect.center().x() + i, rect.center().y() + j, 2, 2, d->lightShade());
                    painter->fillRect(rect.center().x() + i, rect.center().y() + j, 1, 1, d->darkShade());
                }
            }
        }
    }
        painter->restore();
        break;
    case CE_ToolBar:
        if (const QStyleOptionToolBar *toolBar = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
            // Reserve the beveled appearance only for mainwindow toolbars
            if (widget && !(qobject_cast<const QMainWindow*> (widget->parentWidget())))
                break;

            // Draws the light line above and the dark line below menu bars and
            // tool bars.
            QLinearGradient gradient(option->rect.topLeft(), option->rect.bottomLeft());
            if (!(option->state & State_Horizontal))
                gradient = QLinearGradient(rect.left(), rect.center().y(),
                                           rect.right(), rect.center().y());
            gradient.setColorAt(0, option->palette.window().color().lighter(104));
            gradient.setColorAt(1, option->palette.window().color());
            painter->fillRect(option->rect, gradient);

            QColor light = d->lightShade();
            QColor shadow = d->darkShade();

            QPen oldPen = painter->pen();
            if (toolBar->toolBarArea == Qt::TopToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::End
                        || toolBar->positionOfLine == QStyleOptionToolBar::OnlyOne) {
                    // The end and onlyone top toolbar lines draw a double
                    // line at the bottom to blend with the central
                    // widget.
                    painter->setPen(light);
                    painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
                    painter->setPen(shadow);
                    painter->drawLine(option->rect.left(), option->rect.bottom() - 1,
                                      option->rect.right(), option->rect.bottom() - 1);
                } else {
                    // All others draw a single dark line at the bottom.
                    painter->setPen(shadow);
                    painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
                }
                // All top toolbar lines draw a light line at the top.
                painter->setPen(light);
                painter->drawLine(option->rect.topLeft(), option->rect.topRight());
            } else if (toolBar->toolBarArea == Qt::BottomToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::End
                        || toolBar->positionOfLine == QStyleOptionToolBar::Middle) {
                    // The end and middle bottom tool bar lines draw a dark
                    // line at the bottom.
                    painter->setPen(shadow);
                    painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::Beginning
                        || toolBar->positionOfLine == QStyleOptionToolBar::OnlyOne) {
                    // The beginning and only one tool bar lines draw a
                    // double line at the bottom to blend with the
                    // status bar.
                    // ### The styleoption could contain whether the
                    // main window has a menu bar and a status bar, and
                    // possibly dock widgets.
                    painter->setPen(shadow);
                    painter->drawLine(option->rect.left(), option->rect.bottom() - 1,
                                      option->rect.right(), option->rect.bottom() - 1);
                    painter->setPen(light);
                    painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    painter->setPen(shadow);
                    painter->drawLine(option->rect.topLeft(), option->rect.topRight());
                    painter->setPen(light);
                    painter->drawLine(option->rect.left(), option->rect.top() + 1,
                                      option->rect.right(), option->rect.top() + 1);

                } else {
                    // All other bottom toolbars draw a light line at the top.
                    painter->setPen(light);
                    painter->drawLine(option->rect.topLeft(), option->rect.topRight());
                }
            }
            if (toolBar->toolBarArea == Qt::LeftToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::Middle
                        || toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    // The middle and left end toolbar lines draw a light
                    // line to the left.
                    painter->setPen(light);
                    painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    // All other left toolbar lines draw a dark line to the right
                    painter->setPen(shadow);
                    painter->drawLine(option->rect.right() - 1, option->rect.top(),
                                      option->rect.right() - 1, option->rect.bottom());
                    painter->setPen(light);
                    painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
                } else {
                    // All other left toolbar lines draw a dark line to the right
                    painter->setPen(shadow);
                    painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
                }
            } else if (toolBar->toolBarArea == Qt::RightToolBarArea) {
                if (toolBar->positionOfLine == QStyleOptionToolBar::Middle
                        || toolBar->positionOfLine == QStyleOptionToolBar::End) {
                    // Right middle and end toolbar lines draw the dark right line
                    painter->setPen(shadow);
                    painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
                }
                if (toolBar->positionOfLine == QStyleOptionToolBar::End
                        || toolBar->positionOfLine == QStyleOptionToolBar::OnlyOne) {
                    // The right end and single toolbar draws the dark
                    // line on its left edge
                    painter->setPen(shadow);
                    painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
                    // And a light line next to it
                    painter->setPen(light);
                    painter->drawLine(option->rect.left() + 1, option->rect.top(),
                                      option->rect.left() + 1, option->rect.bottom());
                } else {
                    // Other right toolbars draw a light line on its left edge
                    painter->setPen(light);
                    painter->drawLine(option->rect.topLeft(), option->rect.bottomLeft());
                }
            }
            painter->setPen(oldPen);
        }
        break;
    case CE_DockWidgetTitle:
        painter->save();
        if (const QStyleOptionDockWidget *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            bool verticalTitleBar = false;

            QRect titleRect = subElementRect(SE_DockWidgetTitleBarText, option, widget);
            if (verticalTitleBar) {
                QRect rect = dwOpt->rect;
                QRect r = rect;
                QSize s = r.size();
                s.transpose();
                r.setSize(s);
                titleRect = QRect(r.left() + rect.bottom()
                                  - titleRect.bottom(),
                                  r.top() + titleRect.left() - rect.left(),
                                  titleRect.height(), titleRect.width());

                painter->translate(r.left(), r.top() + r.width());
                painter->rotate(-90);
                painter->translate(-r.left(), -r.top());
            }

            if (!dwOpt->title.isEmpty()) {
                QString titleText
                        = painter->fontMetrics().elidedText(dwOpt->title,
                                                            Qt::ElideRight, titleRect.width());
                proxy()->drawItemText(painter,
                                      titleRect,
                                      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, dwOpt->palette,
                                      dwOpt->state & State_Enabled, titleText,
                                      QPalette::WindowText);
            }
        }
        painter->restore();
        break;
    case CE_HeaderSection:
        painter->save();
        // Draws the header in tables.
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            QString pixmapName = uniqueName(QLatin1String("headersection"), option, option->rect.size());
            pixmapName += QString::number(- int(header->position));
            pixmapName += QString::number(- int(header->orientation));

            QPixmap cache;
            if (!QPixmapCache::find(pixmapName, cache)) {
                cache = styleCachePixmap(rect.size());
                cache.fill(Qt::transparent);
                QRect pixmapRect(0, 0, rect.width(), rect.height());
                QPainter cachePainter(&cache);
                QColor buttonColor = d->buttonColor(option->palette);
                QColor gradientStopColor;
                QColor gradientStartColor = buttonColor.lighter(104);
                gradientStopColor = buttonColor.darker(102);
                QLinearGradient gradient(pixmapRect.topLeft(), pixmapRect.bottomLeft());

                if (option->palette.background().gradient()) {
                    gradient.setStops(option->palette.background().gradient()->stops());
                } else {
                    QColor midColor1 = mergedColors(gradientStartColor, gradientStopColor, 60);
                    QColor midColor2 = mergedColors(gradientStartColor, gradientStopColor, 40);
                    gradient.setColorAt(0, gradientStartColor);
                    gradient.setColorAt(0.5, midColor1);
                    gradient.setColorAt(0.501, midColor2);
                    gradient.setColorAt(0.92, gradientStopColor);
                    gradient.setColorAt(1, gradientStopColor.darker(104));
                }
                cachePainter.fillRect(pixmapRect, gradient);
                cachePainter.setPen(d->innerContrastLine());
                cachePainter.setBrush(Qt::NoBrush);
                cachePainter.drawLine(pixmapRect.topLeft(), pixmapRect.topRight());
                cachePainter.setPen(d->outline(option->palette));
                cachePainter.drawLine(pixmapRect.bottomLeft(), pixmapRect.bottomRight());

                if (header->orientation == Qt::Horizontal &&
                        header->position != QStyleOptionHeader::End &&
                        header->position != QStyleOptionHeader::OnlyOneSection) {
                    cachePainter.setPen(QColor(0, 0, 0, 40));
                    cachePainter.drawLine(pixmapRect.topRight(), pixmapRect.bottomRight() + QPoint(0, -1));
                    cachePainter.setPen(d->innerContrastLine());
                    cachePainter.drawLine(pixmapRect.topRight() + QPoint(-1, 0), pixmapRect.bottomRight() + QPoint(-1, -1));
                } else if (header->orientation == Qt::Vertical) {
                    cachePainter.setPen(d->outline(option->palette));
                    cachePainter.drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
                }
                cachePainter.end();
                QPixmapCache::insert(pixmapName, cache);
            }
            painter->drawPixmap(rect.topLeft(), cache);
        }
        painter->restore();
        break;
    case CE_ProgressBarGroove:
        painter->save();
    {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->translate(0.5, 0.5);

        QColor shadowAlpha = Qt::black;
        shadowAlpha.setAlpha(16);
        painter->setPen(shadowAlpha);
        painter->drawLine(rect.topLeft() - QPoint(0, 1), rect.topRight() - QPoint(0, 1));

        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(outline, 1));
        painter->drawRoundedRect(rect.adjusted(0, 0, -1, -1), 2.5, 2.5);
        painter->setBrush(option->palette.base());
        painter->setPen(QPen(option->palette.base(), 1));
        painter->drawRoundedRect(rect.adjusted(1, 1, -2, -2), 1.8, 1.8);

        // Inner shadow
        painter->setPen(d->topShadow());
        painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1),
                          QPoint(rect.right() - 1, rect.top() + 1));
    }
        painter->restore();
        break;
    case CE_ProgressBarContents:
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->translate(0.5, 0.5);
        if (const QStyleOptionProgressBarV2 *bar = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option)) {
            bool vertical = false;
            bool inverted = false;
            bool indeterminate = (bar->minimum == 0 && bar->maximum == 0);
            bool complete = bar->progress == bar->maximum;

            // Get extra style options if version 2
            vertical = (bar->orientation == Qt::Vertical);
            inverted = bar->invertedAppearance;

            // If the orientation is vertical, we use a transform to rotate
            // the progress bar 90 degrees clockwise.  This way we can use the
            // same rendering code for both orientations.
            if (vertical) {
                rect = QRect(rect.left(), rect.top(), rect.height(), rect.width()); // flip width and height
                QTransform m = QTransform::fromTranslate(rect.height()-1, -1.0);
                m.rotate(90.0);
                painter->setTransform(m, true);
            }

            int maxWidth = rect.width();
            int minWidth = 0;
            qreal progress = qMax(bar->progress, bar->minimum); // workaround for bug in QProgressBar
            int progressBarWidth = (progress - bar->minimum) * qreal(maxWidth) / qMax(qreal(1.0), qreal(bar->maximum) - bar->minimum);
            int width = indeterminate ? maxWidth : qMax(minWidth, progressBarWidth);

            bool reverse = (!vertical && (bar->direction == Qt::RightToLeft)) || vertical;
            if (inverted)
                reverse = !reverse;

            int step = 0;
            QRect progressBar;
            QColor highlight = d->highlight(option->palette);
            QColor highlightedoutline = highlight.darker(140);
            if (qGray(outline.rgb()) > qGray(highlightedoutline.rgb()))
                outline = highlightedoutline;

            if (!indeterminate) {
                QColor innerShadow(Qt::black);
                innerShadow.setAlpha(35);
                painter->setPen(innerShadow);
                if (!reverse) {
                    progressBar.setRect(rect.left(), rect.top(), width - 1, rect.height() - 1);
                    if (!complete) {
                        painter->drawLine(progressBar.topRight() + QPoint(2, 1), progressBar.bottomRight() + QPoint(2, 0));
                        painter->setPen(QPen(highlight.darker(140), 0));
                        painter->drawLine(progressBar.topRight() + QPoint(1, 1), progressBar.bottomRight() + QPoint(1, 0));
                    }
                } else {
                    progressBar.setRect(rect.right() - width - 1, rect.top(), width + 2, rect.height() - 1);
                    if (!complete) {
                        painter->drawLine(progressBar.topLeft() + QPoint(-2, 1), progressBar.bottomLeft() + QPoint(-2, 0));
                        painter->setPen(QPen(highlight.darker(140), 0));
                        painter->drawLine(progressBar.topLeft() + QPoint(-1, 1), progressBar.bottomLeft() + QPoint(-1, 0));
                    }
                }
            } else {
                progressBar.setRect(rect.left(), rect.top(), rect.width() - 1, rect.height() - 1);
            }

            if (indeterminate || bar->progress > bar->minimum) {

                QColor highlightedGradientStartColor = highlight.lighter(120);
                QColor highlightedGradientStopColor  = highlight;
                QLinearGradient gradient(rect.topLeft(), QPoint(rect.bottomLeft().x(), rect.bottomLeft().y()));
                gradient.setColorAt(0, highlightedGradientStartColor);
                gradient.setColorAt(1, highlightedGradientStopColor);

                painter->setBrush(gradient);
                painter->setPen(QPen(painter->brush(), 1));

                painter->save();
                if (!complete && !indeterminate)
                    painter->setClipRect(progressBar.adjusted(0, 0, -1, 0));
                QRect fillRect = progressBar.adjusted( indeterminate || complete || !reverse ? 1 : -1, 1,
                                                       indeterminate || complete || reverse ? -1 : 1, -1);
                painter->drawRoundedRect(fillRect, 1.8, 1.8);
                painter->restore();

                painter->setBrush(Qt::NoBrush);
                painter->setPen(QColor(255, 255, 255, 50));
                painter->drawRoundedRect(progressBar.adjusted(1, 1, -1, -1), 1.8, 1.8);

                if (!indeterminate) {
                    d->stopAnimation(widget);
                } else {
                    highlightedGradientStartColor.setAlpha(120);
                    painter->setPen(QPen(highlightedGradientStartColor, 9.0));
                    painter->setClipRect(progressBar.adjusted(1, 1, -1, -1));
#ifndef QT_NO_ANIMATION
                if (CarlaProgressStyleAnimation *animation = qobject_cast<CarlaProgressStyleAnimation*>(d->animation(widget)))
                    step = animation->animationStep() % 22;
                else
                    d->startAnimation(new CarlaProgressStyleAnimation(d->animationFps(), const_cast<QWidget*>(widget)));
#endif
                for (int x = progressBar.left() - rect.height(); x < rect.right() ; x += 22)
                    painter->drawLine(x + step, progressBar.bottom() + 1,
                                      x + rect.height() + step, progressBar.top() - 2);
                }
            }
        }
        painter->restore();
        break;
    case CE_ProgressBarLabel:
        if (const QStyleOptionProgressBarV2 *bar = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option)) {
            QRect leftRect;
            QRect rect = bar->rect;
            QColor textColor = option->palette.text().color();
            QColor alternateTextColor = d->highlightedText(option->palette);

            painter->save();
            bool vertical = false, inverted = false;
            vertical = (bar->orientation == Qt::Vertical);
            inverted = bar->invertedAppearance;
            if (vertical)
                rect = QRect(rect.left(), rect.top(), rect.height(), rect.width()); // flip width and height
            const int progressIndicatorPos = (bar->progress - qreal(bar->minimum)) * rect.width() /
                    qMax(qreal(1.0), qreal(bar->maximum) - bar->minimum);
            if (progressIndicatorPos >= 0 && progressIndicatorPos <= rect.width())
                leftRect = QRect(rect.left(), rect.top(), progressIndicatorPos, rect.height());
            if (vertical)
                leftRect.translate(rect.width() - progressIndicatorPos, 0);

            bool flip = (!vertical && (((bar->direction == Qt::RightToLeft) && !inverted) ||
                                       ((bar->direction == Qt::LeftToRight) && inverted)));

            QRegion rightRect = rect;
            rightRect = rightRect.subtracted(leftRect);
            painter->setClipRegion(rightRect);
            painter->setPen(flip ? alternateTextColor : textColor);
            painter->drawText(rect, bar->text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter | Qt::AlignVCenter));
            if (!leftRect.isNull()) {
                painter->setPen(flip ? textColor : alternateTextColor);
                painter->setClipRect(leftRect);
                painter->drawText(rect, bar->text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter | Qt::AlignVCenter));
            }
            painter->restore();
        }
        break;
    case CE_MenuBarItem:
        painter->save();
        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            QStyleOptionMenuItem item = *mbi;
            item.rect = mbi->rect.adjusted(0, 1, 0, -3);
            QColor highlightOutline = option->palette.highlight().color().darker(125);
            painter->fillRect(rect, option->palette.window());

            QCommonStyle::drawControl(element, &item, painter, widget);

            bool act = mbi->state & State_Selected && mbi->state & State_Sunken;
            bool dis = !(mbi->state & State_Enabled);

            QRect r = option->rect;
            if (act)
            {
                painter->setBrush(option->palette.highlight().color());
                painter->setPen(QPen(highlightOutline, 0));
                painter->drawRect(r.adjusted(0, 5, -1, -1));
                painter->drawRoundedRect(r.adjusted(0, 0, -1, -1), 2, 2);

                //draw text
                QPalette::ColorRole textRole = dis ? QPalette::Text : QPalette::HighlightedText;
                uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!styleHint(SH_UnderlineShortcut, mbi, widget))
                    alignment |= Qt::TextHideMnemonic;
                proxy()->drawItemText(painter, item.rect, alignment, mbi->palette, mbi->state & State_Enabled, mbi->text, textRole);
            }
            else
            {
                QColor shadow = mergedColors(option->palette.background().color().darker(120),
                                             outline.lighter(140), 60);
                painter->setPen(QPen(shadow));
                painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
            }
        }
        painter->restore();
        break;
    case CE_MenuItem:
        painter->save();
        // Draws one item in a popup menu.
        if (const QStyleOptionMenuItem* menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            QColor highlightOutline = highlightedOutline;
            QColor highlight = option->palette.highlight().color();
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                int w = 0;
                if (!menuItem->text.isEmpty()) {
                    painter->setFont(menuItem->font);
                    proxy()->drawItemText(painter, menuItem->rect.adjusted(5, 0, -5, 0), Qt::AlignLeft | Qt::AlignVCenter,
                                          menuItem->palette, menuItem->state & State_Enabled, menuItem->text,
                                          QPalette::Text);
                    w = menuItem->fontMetrics.width(menuItem->text) + 5;
                }
                painter->setPen(highlight);
                bool reverse = menuItem->direction == Qt::RightToLeft;
                painter->drawLine(menuItem->rect.left() + 5 + (reverse ? 0 : w), menuItem->rect.center().y(),
                                  menuItem->rect.right() - 5 - (reverse ? w : 0), menuItem->rect.center().y());
                painter->restore();
                break;
            }
            bool selected = menuItem->state & State_Selected && menuItem->state & State_Enabled;
            if (selected) {
                QRect r = option->rect;
                painter->fillRect(r, highlight);
                painter->setPen(QPen(highlightOutline, 0));
                painter->drawRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5));
            }
            bool checkable = menuItem->checkType != QStyleOptionMenuItem::NotCheckable;
            bool checked = menuItem->checked;
            bool sunken = menuItem->state & State_Sunken;
            bool enabled = menuItem->state & State_Enabled;

            bool ignoreCheckMark = false;
            int checkcol = qMax(menuItem->maxIconWidth, 20);

            if (qobject_cast<const QComboBox*>(widget))
                ignoreCheckMark = true; //ignore the checkmarks provided by the QComboMenuDelegate

            if (!ignoreCheckMark) {
                // Check
                QRect checkRect(option->rect.left() + 7, option->rect.center().y() - 6, 14, 14);
                checkRect = visualRect(menuItem->direction, menuItem->rect, checkRect);
                if (checkable) {
                    if (menuItem->checkType & QStyleOptionMenuItem::Exclusive) {
                        // Radio button
                        if (checked || sunken) {
                            painter->setRenderHint(QPainter::Antialiasing);
                            painter->setPen(Qt::NoPen);

                            QPalette::ColorRole textRole = !enabled ? QPalette::Text:
                                                                      selected ? QPalette::HighlightedText : QPalette::ButtonText;
                            painter->setBrush(option->palette.brush( option->palette.currentColorGroup(), textRole));
                            painter->drawEllipse(checkRect.adjusted(4, 4, -4, -4));
                        }
                    } else {
                        // Check box
                        if (menuItem->icon.isNull()) {
                            QStyleOptionButton box;
                            box.QStyleOption::operator=(*option);
                            box.rect = checkRect;
                            if (checked)
                                box.state |= State_On;
                            proxy()->drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
                            painter->setPen(QPen(highlight, 0));
                            painter->drawRect(checkRect);
                        }
                    }
                }
            } else { //ignore checkmark
                if (menuItem->icon.isNull())
                    checkcol = 0;
                else
                    checkcol = menuItem->maxIconWidth;
            }

            // Text and icon, ripped from windows style
            bool dis = !(menuItem->state & State_Enabled);
            bool act = menuItem->state & State_Selected;
            const QStyleOption *opt = option;
            const QStyleOptionMenuItem *menuitem = menuItem;

            QPainter *p = painter;
            QRect vCheckRect = visualRect(opt->direction, menuitem->rect,
                                          QRect(menuitem->rect.x() + 4, menuitem->rect.y(),
                                                checkcol, menuitem->rect.height()));
            if (!menuItem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                QPixmap pixmap;

                int smallIconSize = proxy()->pixelMetric(PM_SmallIconSize, option, widget);
                QSize iconSize(smallIconSize, smallIconSize);
                if (const QComboBox *combo = qobject_cast<const QComboBox*>(widget))
                    iconSize = combo->iconSize();
                if (checked)
                    pixmap = menuItem->icon.pixmap(iconSize, mode, QIcon::On);
                else
                    pixmap = menuItem->icon.pixmap(iconSize, mode);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
                const int pixw = pixmap.width() / pixmap.devicePixelRatioF();
                const int pixh = pixmap.height() / pixmap.devicePixelRatioF();
#else
                const int pixw = pixmap.width();
                const int pixh = pixmap.height();
#endif

                QRect pmr(0, 0, pixw, pixh);
                pmr.moveCenter(vCheckRect.center());
                painter->setPen(menuItem->palette.text().color());
                if (checkable && checked) {
                    QStyleOption opt = *option;
                    if (act) {
                        QColor activeColor = mergedColors(option->palette.background().color(),
                                                          option->palette.highlight().color());
                        opt.palette.setBrush(QPalette::Button, activeColor);
                    }
                    opt.state |= State_Sunken;
                    opt.rect = vCheckRect;
                    proxy()->drawPrimitive(PE_PanelButtonCommand, &opt, painter, widget);
                }
                painter->drawPixmap(pmr.topLeft(), pixmap);
            }
            if (selected) {
                painter->setPen(menuItem->palette.highlightedText().color());
            } else {
                painter->setPen(menuItem->palette.text().color());
            }
            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            int tab = menuitem->tabWidth;
            QColor discol;
            if (dis) {
                discol = menuitem->palette.text().color();
                p->setPen(discol);
            }
            int xm = windowsItemFrame + checkcol + windowsItemHMargin + 2;
            int xpos = menuitem->rect.x() + xm;

            QRect textRect(xpos, y + windowsItemVMargin, w - xm - windowsRightBorder - tab + 1, h - 2 * windowsItemVMargin);
            QRect vTextRect = visualRect(opt->direction, menuitem->rect, textRect);
            QString s = menuitem->text;
            if (!s.isEmpty()) {                     // draw text
                p->save();
                int t = s.indexOf(QLatin1Char('\t'));
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                text_flags |= Qt::AlignLeft;
                if (t >= 0) {
                    QRect vShortcutRect = visualRect(opt->direction, menuitem->rect,
                                                     QRect(textRect.topRight(), QPoint(menuitem->rect.right(), textRect.bottom())));
                    if (dis && !act && proxy()->styleHint(SH_EtchDisabledText, option, widget)) {
                        p->setPen(menuitem->palette.light().color());
                        p->drawText(vShortcutRect.adjusted(1, 1, 1, 1), text_flags, s.mid(t + 1));
                        p->setPen(discol);
                    }
                    p->drawText(vShortcutRect, text_flags, s.mid(t + 1));
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                // font may not have any "hard" flags set. We override
                // the point size so that when it is resolved against the device, this font will win.
                // This is mainly to handle cases where someone sets the font on the window
                // and then the combo inherits it and passes it onward. At that point the resolve mask
                // is very, very weak. This makes it stonger.
                font.setPointSizeF(QFontInfo(menuItem->font).pointSizeF());

                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);

                p->setFont(font);
                if (dis && !act && proxy()->styleHint(SH_EtchDisabledText, option, widget)) {
                    p->setPen(menuitem->palette.light().color());
                    p->drawText(vTextRect.adjusted(1, 1, 1, 1), text_flags, s.left(t));
                    p->setPen(discol);
                }
                p->drawText(vTextRect, text_flags, s.left(t));
                p->restore();
            }

            // Arrow
            if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow
                int dim = (menuItem->rect.height() - 4) / 2;
                PrimitiveElement arrow;
                arrow = option->direction == Qt::RightToLeft ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
                int xpos = menuItem->rect.left() + menuItem->rect.width() - 3 - dim;
                QRect  vSubMenuRect = visualRect(option->direction, menuItem->rect,
                                                 QRect(xpos, menuItem->rect.top() + menuItem->rect.height() / 2 - dim / 2, dim, dim));
                QStyleOptionMenuItem newMI = *menuItem;
                newMI.rect = vSubMenuRect;
                newMI.state = !enabled ? State_None : State_Enabled;
                if (selected)
                    newMI.palette.setColor(QPalette::Foreground,
                                           newMI.palette.highlightedText().color());
                proxy()->drawPrimitive(arrow, &newMI, painter, widget);
            }
        }
        painter->restore();
        break;
    case CE_MenuHMargin:
    case CE_MenuVMargin:
        break;
    case CE_MenuEmptyArea:
        break;
    case CE_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            proxy()->drawControl(CE_PushButtonBevel, btn, painter, widget);
            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);
            proxy()->drawControl(CE_PushButtonLabel, &subopt, painter, widget);
        }
        break;
    case CE_PushButtonLabel:
        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            QRect ir = button->rect;
            uint tf = Qt::AlignVCenter;
            if (styleHint(SH_UnderlineShortcut, button, widget))
                tf |= Qt::TextShowMnemonic;
            else
                tf |= Qt::TextHideMnemonic;

            if (!button->icon.isNull()) {
                //Center both icon and text
                QPoint point;

                QIcon::Mode mode = button->state & State_Enabled ? QIcon::Normal
                                                                 : QIcon::Disabled;
                if (mode == QIcon::Normal && button->state & State_HasFocus)
                    mode = QIcon::Active;
                QIcon::State state = QIcon::Off;
                if (button->state & State_On)
                    state = QIcon::On;

                QPixmap pixmap = button->icon.pixmap(button->iconSize, mode, state);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
                int w = pixmap.width() / pixmap.devicePixelRatio();
                int h = pixmap.height() / pixmap.devicePixelRatio();
#else
                int w = pixmap.width();
                int h = pixmap.height();
#endif

                if (!button->text.isEmpty())
                    w += button->fontMetrics.boundingRect(option->rect, tf, button->text).width() + 2;

                point = QPoint(ir.x() + ir.width() / 2 - w / 2,
                               ir.y() + ir.height() / 2 - h / 2);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
                w = pixmap.width() / pixmap.devicePixelRatio();
#else
                w = pixmap.width();
#endif

                if (button->direction == Qt::RightToLeft)
                    point.rx() += w;

                painter->drawPixmap(visualPos(button->direction, button->rect, point), pixmap);

                if (button->direction == Qt::RightToLeft)
                    ir.translate(-point.x() - 2, 0);
                else
                    ir.translate(point.x() + w, 0);

                // left-align text if there is
                if (!button->text.isEmpty())
                    tf |= Qt::AlignLeft;

            } else {
                tf |= Qt::AlignHCenter;
            }

            if (button->features & QStyleOptionButton::HasMenu)
                ir = ir.adjusted(0, 0, -proxy()->pixelMetric(PM_MenuButtonIndicator, button, widget), 0);
            proxy()->drawItemText(painter, ir, tf, button->palette, (button->state & State_Enabled),
                                  button->text, QPalette::ButtonText);
        }
        break;
    case CE_MenuBarEmptyArea:
        painter->save();
    {
        painter->fillRect(rect, option->palette.window());
        if (widget && qobject_cast<const QMainWindow *>(widget->parentWidget())) {
            QColor shadow = mergedColors(option->palette.background().color().darker(120),
                                         outline.lighter(140), 60);
            painter->setPen(QPen(shadow));
            painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
        }
    }
        painter->restore();
        break;
    case CE_TabBarTabShape:
        painter->save();
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {

            bool rtlHorTabs = (tab->direction == Qt::RightToLeft
                               && (tab->shape == QTabBar::RoundedNorth
                                   || tab->shape == QTabBar::RoundedSouth));
            bool selected = tab->state & State_Selected;
            bool lastTab = ((!rtlHorTabs && tab->position == QStyleOptionTab::End)
                            || (rtlHorTabs
                                && tab->position == QStyleOptionTab::Beginning));
            bool onlyOne = tab->position == QStyleOptionTab::OnlyOneTab;
            int tabOverlap = pixelMetric(PM_TabBarTabOverlap, option, widget);
            rect = option->rect.adjusted(0, 0, (onlyOne || lastTab) ? 0 : tabOverlap, 0);

#if 0
            QRect r2(rect);
            int x1 = r2.left();
            int x2 = r2.right();
            int y1 = r2.top();
            int y2 = r2.bottom();
#endif

            painter->setPen(d->innerContrastLine());

            QTransform rotMatrix;
            bool flip = false;
            painter->setPen(shadow);

            switch (tab->shape) {
            case QTabBar::RoundedNorth:
                break;
            case QTabBar::RoundedSouth:
                rotMatrix.rotate(180);
                rotMatrix.translate(0, -rect.height() + 1);
                rotMatrix.scale(-1, 1);
                painter->setTransform(rotMatrix, true);
                break;
            case QTabBar::RoundedWest:
                rotMatrix.rotate(180 + 90);
                rotMatrix.scale(-1, 1);
                flip = true;
                painter->setTransform(rotMatrix, true);
                break;
            case QTabBar::RoundedEast:
                rotMatrix.rotate(90);
                rotMatrix.translate(0, - rect.width() + 1);
                flip = true;
                painter->setTransform(rotMatrix, true);
                break;
            default:
                painter->restore();
                QCommonStyle::drawControl(element, tab, painter, widget);
                return;
            }

            if (flip) {
                QRect tmp = rect;
                rect = QRect(tmp.y(), tmp.x(), tmp.height(), tmp.width());
#if 0
                int temp = x1;
                x1 = y1;
                y1 = temp;
                temp = x2;
                x2 = y2;
                y2 = temp;
#endif
            }

            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);

            QColor tabFrameColor = d->tabFrameColor(option->palette);

            QLinearGradient fillGradient(rect.topLeft(), rect.bottomLeft());
            QLinearGradient outlineGradient(rect.topLeft(), rect.bottomLeft());
            QPen outlinePen = outline.lighter(110);
            if (selected) {
                fillGradient.setColorAt(0, tabFrameColor.lighter(104));
                //                QColor highlight = option->palette.highlight().color();
                //                if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange) {
                //                    fillGradient.setColorAt(0, highlight.lighter(130));
                //                    outlineGradient.setColorAt(0, highlight.darker(130));
                //                    fillGradient.setColorAt(0.14, highlight);
                //                    outlineGradient.setColorAt(0.14, highlight.darker(130));
                //                    fillGradient.setColorAt(0.1401, tabFrameColor);
                //                    outlineGradient.setColorAt(0.1401, highlight.darker(130));
                //                }
                fillGradient.setColorAt(1, tabFrameColor);
                outlineGradient.setColorAt(1, outline);
                outlinePen = QPen(outlineGradient, 1);
            } else {
                fillGradient.setColorAt(0, tabFrameColor.darker(108));
                fillGradient.setColorAt(0.85, tabFrameColor.darker(108));
                fillGradient.setColorAt(1, tabFrameColor.darker(116));
            }

            QRect drawRect = rect.adjusted(0, selected ? 0 : 2, 0, 3);
            painter->setPen(outlinePen);
            painter->save();
            painter->setClipRect(rect.adjusted(-1, -1, 1, selected ? -2 : -3));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(drawRect.adjusted(0, 0, -1, -1), 2.5, 2.5);

            painter->setBrush(fillGradient);
            painter->setPen(QPen(QBrush(fillGradient), 1));
            drawRect.adjust(1, 1, -2, -1);
            painter->drawRoundedRect(drawRect, 1.8, 1.8);

            painter->setBrush(Qt::NoBrush);
            painter->setPen(d->innerContrastLine());
            painter->drawRoundedRect(drawRect, 1.8, 1.8);

            painter->restore();

            if (selected) {
                painter->fillRect(rect.left() + 1, rect.bottom() - 1, rect.width() - 2, rect.bottom() - 1, tabFrameColor);
                painter->fillRect(QRect(rect.bottomRight() + QPoint(-2, -1), QSize(1, 1)), d->innerContrastLine());
                painter->fillRect(QRect(rect.bottomLeft() + QPoint(0, -1), QSize(1, 1)), d->innerContrastLine());
                painter->fillRect(QRect(rect.bottomRight() + QPoint(-1, -1), QSize(1, 1)), d->innerContrastLine());
            }
        }
        painter->restore();
        break;
    default:
        QCommonStyle::drawControl(element,option,painter,widget);
        break;
    }
}

/*!
  \reimp
*/
QPalette CarlaStyle::standardPalette () const
{
    QPalette palette = QCommonStyle::standardPalette();
    palette.setBrush(QPalette::Active, QPalette::Highlight, QColor(48, 140, 198));
    palette.setBrush(QPalette::Inactive, QPalette::Highlight, QColor(145, 141, 126));
    palette.setBrush(QPalette::Disabled, QPalette::Highlight, QColor(145, 141, 126));

    QColor backGround(239, 235, 231);

    QColor light = backGround.lighter(150);
    QColor base = Qt::white;
    QColor dark = QColor(170, 156, 143).darker(110);
    dark = backGround.darker(150);
    QColor darkDisabled = QColor(209, 200, 191).darker(110);

    //### Find the correct disabled text color
    palette.setBrush(QPalette::Disabled, QPalette::Text, QColor(190, 190, 190));

    palette.setBrush(QPalette::Window, backGround);
    palette.setBrush(QPalette::Mid, backGround.darker(130));
    palette.setBrush(QPalette::Light, light);

    palette.setBrush(QPalette::Active, QPalette::Base, base);
    palette.setBrush(QPalette::Inactive, QPalette::Base, base);
    palette.setBrush(QPalette::Disabled, QPalette::Base, backGround);

    palette.setBrush(QPalette::Midlight, palette.mid().color().lighter(110));

    palette.setBrush(QPalette::All, QPalette::Dark, dark);
    palette.setBrush(QPalette::Disabled, QPalette::Dark, darkDisabled);

    QColor button = backGround;

    palette.setBrush(QPalette::Button, button);

    QColor shadow = dark.darker(135);
    palette.setBrush(QPalette::Shadow, shadow);
    palette.setBrush(QPalette::Disabled, QPalette::Shadow, shadow.lighter(150));
    palette.setBrush(QPalette::HighlightedText, QColor(QRgb(0xffffffff)));

    return palette;
}

/*!
  \reimp
*/
void CarlaStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                      QPainter *painter, const QWidget *widget) const
{
    QColor buttonColor = d->buttonColor(option->palette);
    QColor gradientStartColor = buttonColor.lighter(118);
    QColor gradientStopColor = buttonColor;
    QColor outline = d->outline(option->palette);

    QColor alphaCornerColor;
    if (widget) {
        // ### backgroundrole/foregroundrole should be part of the style option
        alphaCornerColor = mergedColors(option->palette.color(widget->backgroundRole()), outline);
    } else {
        alphaCornerColor = mergedColors(option->palette.background().color(), outline);
    }

    switch (control) {
    case CC_GroupBox:
        painter->save();
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            // Draw frame
            QRect textRect = proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxLabel, widget);
            QRect checkBoxRect = proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxCheckBox, widget);

            if (groupBox->subControls & QStyle::SC_GroupBoxFrame) {
                QStyleOptionFrameV3 frame;
                frame.QStyleOption::operator=(*groupBox);
                frame.features = groupBox->features;
                frame.lineWidth = groupBox->lineWidth;
                frame.midLineWidth = groupBox->midLineWidth;
                frame.rect = proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxFrame, widget);
                proxy()->drawPrimitive(PE_FrameGroupBox, &frame, painter, widget);
            }

            // Draw title
            if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {
                // groupBox->textColor gets the incorrect palette here
                painter->setPen(QPen(option->palette.windowText(), 1));
                int alignment = int(groupBox->textAlignment);
                if (!proxy()->styleHint(QStyle::SH_UnderlineShortcut, option, widget))
                    alignment |= Qt::TextHideMnemonic;

                proxy()->drawItemText(painter, textRect,  Qt::TextShowMnemonic | Qt::AlignLeft | alignment,
                                      groupBox->palette, groupBox->state & State_Enabled, groupBox->text, QPalette::NoRole);

                if (groupBox->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*groupBox);
                    fropt.rect = textRect.adjusted(-2, -1, 2, 1);
                    proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
            }

            // Draw checkbox
            if (groupBox->subControls & SC_GroupBoxCheckBox) {
                QStyleOptionButton box;
                box.QStyleOption::operator=(*groupBox);
                box.rect = checkBoxRect;
                proxy()->drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
            }
        }
        painter->restore();
        break;
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QPixmap cache;
            QString pixmapName = uniqueName(QLatin1String("spinbox"), spinBox, spinBox->rect.size());
            if (!QPixmapCache::find(pixmapName, cache)) {

                cache = styleCachePixmap(spinBox->rect.size());
                cache.fill(Qt::transparent);

                QRect pixmapRect(0, 0, spinBox->rect.width(), spinBox->rect.height());
                QRect rect = pixmapRect;
                QRect r = rect;
                QPainter cachePainter(&cache);
                QColor arrowColor = spinBox->palette.foreground().color();
                arrowColor.setAlpha(220);

                const bool isEnabled = (spinBox->state & State_Enabled);
                const bool hover = isEnabled && (spinBox->state & State_MouseOver);
                const bool sunken = (spinBox->state & State_Sunken);
                const bool upIsActive = (spinBox->activeSubControls == SC_SpinBoxUp);
                const bool downIsActive = (spinBox->activeSubControls == SC_SpinBoxDown);
                const bool hasFocus = (option->state & State_HasFocus);

                QStyleOptionSpinBox spinBoxCopy = *spinBox;
                spinBoxCopy.rect = pixmapRect;
                QRect upRect = proxy()->subControlRect(CC_SpinBox, &spinBoxCopy, SC_SpinBoxUp, widget);
                QRect downRect = proxy()->subControlRect(CC_SpinBox, &spinBoxCopy, SC_SpinBoxDown, widget);

                const bool oddHeight = (1+upRect.bottom() != downRect.top());

                if (spinBox->frame) {
                    // Button group outer bounds
                    const int bty = upRect.top();
                    const int bby = 1+downRect.bottom() - 1;
                    const int brx = 1+upRect.right() - 1;

                    cachePainter.save();
                    cachePainter.setRenderHint(QPainter::Antialiasing, true);
                    cachePainter.translate(0.5, 0.5);

                    // Fill background
                    const QBrush & brush = option->palette.base();
                    cachePainter.setPen(brush.color());
                    cachePainter.setBrush(brush);
                    cachePainter.drawRoundedRect(r.adjusted(1, 1, -2, -2), 1.8, 1.8);

                    // Draw inner shadow
                    cachePainter.setPen(d->topShadow());
                    cachePainter.drawLine(QPoint(r.left() + 2, r.top() + 1), QPoint(1+r.right() - 2, r.top() + 1));

                    // Draw button gradient
                    QColor buttonColor = d->buttonColor(option->palette);
                    QRectF updownRect(upRect.topLeft(), downRect.bottomRight() + QPoint(1,1));
                    updownRect.adjust(-0.5, -0.5, 0.5-1, 0.5-1);
                    QLinearGradient gradient = qt_fusion_gradient(updownRect.toAlignedRect(), (isEnabled && option->state & State_MouseOver ) ? buttonColor : buttonColor.darker(104));

                    // Draw button gradient
                    cachePainter.setPen(QPen(gradient, 1));
                    cachePainter.setBrush(gradient);

                    cachePainter.save();
                    cachePainter.setClipRect(updownRect);
                    cachePainter.drawRoundedRect(QRect(0, bty, brx, bby-bty), 1.8, 1.8);
                    cachePainter.setPen(QPen(d->innerContrastLine()));
                    cachePainter.setBrush(Qt::NoBrush);
                    cachePainter.drawRoundedRect(QRect(0, bty, brx, bby-bty), 1.8, 1.8);
                    cachePainter.drawLine(upRect.left(), bty + 1, upRect.left(), bby - 1);
                    if (hover) {
                        const int y = oddHeight ? (downRect.top() - 1) : (upIsActive ? (1+upRect.bottom() - 1) : downRect.top());
                        cachePainter.setOpacity(0.4);
                        cachePainter.drawLine(downRect.left() + 1, y, brx - 1, y);
                        cachePainter.setOpacity(1.0);
                    }
                    cachePainter.restore();
                    cachePainter.setPen(Qt::NoPen);

                    // Buttons mouse over background
                    if ((spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled) && upIsActive) {
                        QPointF clipTLeft(0, upRect.top());
                        QPointF clipBRight(1+downRect.right() - 1, 1+downRect.bottom() - 1);
                        QRectF clipRect(clipTLeft, clipBRight);

                        cachePainter.save();

                        clipRect.adjust(-0.5,-0.5, 0.5, 0.5);
                        QPainterPath clipPath;
                        clipPath.addRoundedRect(clipRect, 2.0, 2.0);
                        cachePainter.setClipPath(clipPath);

                        const int cy_fix = oddHeight ? 0 : -1;
                        if (sunken)
                            cachePainter.fillRect(QRectF(upRect).adjusted(-0.5, -0.5, 0.5-1, 0.5+cy_fix), gradientStopColor.darker(110));
                        else if (hover)
                            cachePainter.fillRect(QRectF(upRect).adjusted(-0.5, -0.5, 0.5-1, 0.5+cy_fix), d->innerContrastLine());

                        cachePainter.restore();
                    }

                    if ((spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled) && downIsActive) {
                        QPointF clipTLeft(0, upRect.top());
                        QPointF clipBRight(1+downRect.right() - 1, 1+downRect.bottom() - 1);
                        QRectF clipRect(clipTLeft, clipBRight);

                        cachePainter.save();

                        clipRect.adjust(-0.5,-0.5, 0.5, 0.5);
                        QPainterPath clipPath;
                        clipPath.addRoundedRect(clipRect, 2.0, 2.0);
                        cachePainter.setClipPath(clipPath);

                        const int cy_fix = oddHeight ? 0 : 1;
                        if (sunken)
                            cachePainter.fillRect(QRectF(downRect).adjusted(-0.5, -0.5-1+cy_fix, 0.5-1, 0.5-1-cy_fix), gradientStopColor.darker(110));
                        else if (hover)
                            cachePainter.fillRect(QRectF(downRect).adjusted(-0.5, -0.5-1+cy_fix, 0.5-1, 0.5-1-cy_fix), d->innerContrastLine());

                        cachePainter.restore();
                    }

                    // Common highlight border
                    QColor highlightOutline = d->highlightedOutline(option->palette);
                    cachePainter.setPen(hasFocus ? highlightOutline : highlightOutline.darker(160));
                    cachePainter.setBrush(Qt::NoBrush);
                    cachePainter.drawRoundedRect(r.adjusted(0, 0, -1, -1), 2.5, 2.5);
                    if (hasFocus) {
                        QColor softHighlight = option->palette.highlight().color();
                        softHighlight.setAlpha(40);
                        cachePainter.setPen(softHighlight);
                        cachePainter.drawRoundedRect(r.adjusted(1, 1, -2, -2), 1.8, 1.8);
                    }
                    cachePainter.restore();
                }

                // outline the up/down buttons
                cachePainter.setPen(outline);
                if (spinBox->direction == Qt::RightToLeft) {
                    cachePainter.drawLine(1+upRect.right() + 1, upRect.top(), 1+upRect.right() + 1, 1+downRect.bottom() - 1);
                } else {
                    cachePainter.drawLine(upRect.left() - 1, upRect.top(), upRect.left() - 1, 1+downRect.bottom() - 1);
                }

                if (upIsActive && sunken) {
                    cachePainter.setPen(gradientStopColor.darker(130));
                    const int left = upRect.left();
                    const int right = 1+upRect.right() - 1;
                    const int top = upRect.top();
                    const int bottom = oddHeight ? (1+upRect.bottom()) : (1+upRect.bottom() - 1);
                    const QPoint points[] = {QPoint(right,bottom), QPoint(left,bottom), QPoint(left,top), QPoint(right-1,top)};
                    cachePainter.drawPolyline(points, 4);
                }

                if (downIsActive && sunken) {
                    cachePainter.setPen(gradientStopColor.darker(130));
                    const int left = downRect.left();
                    const int right = 1+downRect.right() - 1;
                    const int top = oddHeight ? (downRect.top() - 1) : downRect.top();
                    const int bottom = 1+downRect.bottom() - 1;
                    const QPoint points[] = {QPoint(left,bottom), QPoint(left,top), QPoint(right,top)};
                    cachePainter.drawPolyline(points, 3);
                }

                QColor disabledColor = mergedColors(arrowColor, option->palette.button().color());
                if (spinBox->buttonSymbols == QAbstractSpinBox::PlusMinus) {
                    // plus/minus
                    int centerX, centerY;

                    centerX = upRect.center().x();
                    centerY = upRect.center().y();
                    cachePainter.setPen((spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled) ? arrowColor : disabledColor);
                    cachePainter.drawLine(centerX - 1, centerY, centerX + 3, centerY);
                    cachePainter.drawLine(centerX + 1, centerY - 2, centerX + 1, centerY + 2);

                    centerX = downRect.center().x();
                    centerY = downRect.center().y();
                    cachePainter.setPen((spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled) ? arrowColor : disabledColor);
                    cachePainter.drawLine(centerX - 1, centerY, centerX + 3, centerY);

                } else if (spinBox->buttonSymbols == QAbstractSpinBox::UpDownArrows){
                    // arrows
                    painter->setRenderHint(QPainter::SmoothPixmapTransform);

                    QPixmap upArrow = colorizedImage(QLatin1String(":/bitmaps/style/arrow.png"),
                                                     (spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled) ? arrowColor : disabledColor);

                    QPixmap downArrow = colorizedImage(QLatin1String(":/bitmaps/style/arrow.png"),
                                                       (spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled) ? arrowColor : disabledColor, 180);

                    int y1, y2;
                    const int imgW = (upArrow.width() + downArrow.width()) / 2;
                    const int imgH = (upArrow.height() + downArrow.height()) / 2;
                    const float f = 1.0 / ceil(imgW / upRect.width() * 2);
                    const int w = imgW * f;
                    const int h = imgH * f;

                    const int x = upRect.center().x() - w / 2;
                    y1 = upRect.center().y();
                    y2 = downRect.center().y();
                    const int dy1 = 1+upRect.bottom() - y1;
                    const int dy2 = y2 - downRect.top();
                    const int dy = dy1 < dy2 ? dy1 : dy2;
                    y1 = (1+upRect.bottom() - dy - 1) - ceil(h / 4.0);
                    y2 = (downRect.top() + dy + 1) - floor(h / 4.0 * 3);

                    cachePainter.drawPixmap(QRectF(x, y1, w, h), upArrow, upArrow.rect());
                    cachePainter.drawPixmap(QRectF(x, y2, w, h), downArrow, downArrow.rect());
                }

                cachePainter.end();
                QPixmapCache::insert(pixmapName, cache);
            }
            painter->drawPixmap(spinBox->rect.topLeft(), cache);
        }
        break;
    case CC_TitleBar:
        painter->save();
        if (const QStyleOptionTitleBar *titleBar = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            const int buttonMargin = 5;
            bool active = (titleBar->titleBarState & State_Active);
            QRect fullRect = titleBar->rect;
            QPalette palette = option->palette;
            QColor highlight = option->palette.highlight().color();

            QColor titleBarFrameBorder(active ? highlight.darker(180): outline.darker(110));
            QColor titleBarHighlight(active ? highlight.lighter(120): palette.background().color().lighter(120));
            QColor textColor(active ? 0xffffff : 0xff000000);
            QColor textAlphaColor(active ? 0xffffff : 0xff000000 );

            {
                // Fill title bar gradient
                QColor titlebarColor = QColor(active ? highlight: palette.background().color());
                QLinearGradient gradient(option->rect.center().x(), option->rect.top(),
                                         option->rect.center().x(), option->rect.bottom());

                gradient.setColorAt(0, titlebarColor.lighter(114));
                gradient.setColorAt(0.5, titlebarColor.lighter(102));
                gradient.setColorAt(0.51, titlebarColor.darker(104));
                gradient.setColorAt(1, titlebarColor);
                painter->fillRect(option->rect.adjusted(1, 1, -1, 0), gradient);

                // Frame and rounded corners
                painter->setPen(titleBarFrameBorder);

                // top outline
                painter->drawLine(fullRect.left() + 5, fullRect.top(), fullRect.right() - 5, fullRect.top());
                painter->drawLine(fullRect.left(), fullRect.top() + 4, fullRect.left(), fullRect.bottom());
                const QPoint points[5] = {
                    QPoint(fullRect.left() + 4, fullRect.top() + 1),
                    QPoint(fullRect.left() + 3, fullRect.top() + 1),
                    QPoint(fullRect.left() + 2, fullRect.top() + 2),
                    QPoint(fullRect.left() + 1, fullRect.top() + 3),
                    QPoint(fullRect.left() + 1, fullRect.top() + 4)
                };
                painter->drawPoints(points, 5);

                painter->drawLine(fullRect.right(), fullRect.top() + 4, fullRect.right(), fullRect.bottom());
                const QPoint points2[5] = {
                    QPoint(fullRect.right() - 3, fullRect.top() + 1),
                    QPoint(fullRect.right() - 4, fullRect.top() + 1),
                    QPoint(fullRect.right() - 2, fullRect.top() + 2),
                    QPoint(fullRect.right() - 1, fullRect.top() + 3),
                    QPoint(fullRect.right() - 1, fullRect.top() + 4)
                };
                painter->drawPoints(points2, 5);

                // draw bottomline
                painter->drawLine(fullRect.right(), fullRect.bottom(), fullRect.left(), fullRect.bottom());

                // top highlight
                painter->setPen(titleBarHighlight);
                painter->drawLine(fullRect.left() + 6, fullRect.top() + 1, fullRect.right() - 6, fullRect.top() + 1);
            }
            // draw title
            QRect textRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarLabel, widget);
            painter->setPen(active? (titleBar->palette.text().color().lighter(120)) :
                                    titleBar->palette.text().color() );
            // Note workspace also does elliding but it does not use the correct font
            QString title = painter->fontMetrics().elidedText(titleBar->text, Qt::ElideRight, textRect.width() - 14);
            painter->drawText(textRect.adjusted(1, 1, 1, 1), title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
            painter->setPen(Qt::white);
            if (active)
                painter->drawText(textRect, title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
            // min button
            if ((titleBar->subControls & SC_TitleBarMinButton) && (titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                    !(titleBar->titleBarState& Qt::WindowMinimized)) {
                QRect minButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMinButton, widget);
                if (minButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarMinButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarMinButton) && (titleBar->state & State_Sunken);
                    qt_fusion_draw_mdibutton(painter, titleBar, minButtonRect, hover, sunken);
                    QRect minButtonIconRect = minButtonRect.adjusted(buttonMargin ,buttonMargin , -buttonMargin, -buttonMargin);
                    painter->setPen(textColor);
                    painter->drawLine(minButtonIconRect.center().x() - 2, minButtonIconRect.center().y() + 3,
                                      minButtonIconRect.center().x() + 3, minButtonIconRect.center().y() + 3);
                    painter->drawLine(minButtonIconRect.center().x() - 2, minButtonIconRect.center().y() + 4,
                                      minButtonIconRect.center().x() + 3, minButtonIconRect.center().y() + 4);
                    painter->setPen(textAlphaColor);
                    painter->drawLine(minButtonIconRect.center().x() - 3, minButtonIconRect.center().y() + 3,
                                      minButtonIconRect.center().x() - 3, minButtonIconRect.center().y() + 4);
                    painter->drawLine(minButtonIconRect.center().x() + 4, minButtonIconRect.center().y() + 3,
                                      minButtonIconRect.center().x() + 4, minButtonIconRect.center().y() + 4);
                }
            }
            // max button
            if ((titleBar->subControls & SC_TitleBarMaxButton) && (titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                    !(titleBar->titleBarState & Qt::WindowMaximized)) {
                QRect maxButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMaxButton, widget);
                if (maxButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarMaxButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarMaxButton) && (titleBar->state & State_Sunken);
                    qt_fusion_draw_mdibutton(painter, titleBar, maxButtonRect, hover, sunken);

                    QRect maxButtonIconRect = maxButtonRect.adjusted(buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);

                    painter->setPen(textColor);
                    painter->drawRect(maxButtonIconRect.adjusted(0, 0, -1, -1));
                    painter->drawLine(maxButtonIconRect.left() + 1, maxButtonIconRect.top() + 1,
                                      maxButtonIconRect.right() - 1, maxButtonIconRect.top() + 1);
                    painter->setPen(textAlphaColor);
                    const QPoint points[4] = {
                        maxButtonIconRect.topLeft(),
                        maxButtonIconRect.topRight(),
                        maxButtonIconRect.bottomLeft(),
                        maxButtonIconRect.bottomRight()
                    };
                    painter->drawPoints(points, 4);
                }
            }

            // close button
            if ((titleBar->subControls & SC_TitleBarCloseButton) && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
                QRect closeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarCloseButton, widget);
                if (closeButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarCloseButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarCloseButton) && (titleBar->state & State_Sunken);
                    qt_fusion_draw_mdibutton(painter, titleBar, closeButtonRect, hover, sunken);
                    QRect closeIconRect = closeButtonRect.adjusted(buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);
                    painter->setPen(textAlphaColor);
                    const QLine lines[4] = {
                        QLine(closeIconRect.left() + 1, closeIconRect.top(),
                        closeIconRect.right(), closeIconRect.bottom() - 1),
                        QLine(closeIconRect.left(), closeIconRect.top() + 1,
                        closeIconRect.right() - 1, closeIconRect.bottom()),
                        QLine(closeIconRect.right() - 1, closeIconRect.top(),
                        closeIconRect.left(), closeIconRect.bottom() - 1),
                        QLine(closeIconRect.right(), closeIconRect.top() + 1,
                        closeIconRect.left() + 1, closeIconRect.bottom())
                    };
                    painter->drawLines(lines, 4);
                    const QPoint points[4] = {
                        closeIconRect.topLeft(),
                        closeIconRect.topRight(),
                        closeIconRect.bottomLeft(),
                        closeIconRect.bottomRight()
                    };
                    painter->drawPoints(points, 4);

                    painter->setPen(textColor);
                    painter->drawLine(closeIconRect.left() + 1, closeIconRect.top() + 1,
                                      closeIconRect.right() - 1, closeIconRect.bottom() - 1);
                    painter->drawLine(closeIconRect.left() + 1, closeIconRect.bottom() - 1,
                                      closeIconRect.right() - 1, closeIconRect.top() + 1);
                }
            }

            // normalize button
            if ((titleBar->subControls & SC_TitleBarNormalButton) &&
                    (((titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                      (titleBar->titleBarState & Qt::WindowMinimized)) ||
                     ((titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                      (titleBar->titleBarState & Qt::WindowMaximized)))) {
                QRect normalButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarNormalButton, widget);
                if (normalButtonRect.isValid()) {

                    bool hover = (titleBar->activeSubControls & SC_TitleBarNormalButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarNormalButton) && (titleBar->state & State_Sunken);
                    QRect normalButtonIconRect = normalButtonRect.adjusted(buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);
                    qt_fusion_draw_mdibutton(painter, titleBar, normalButtonRect, hover, sunken);

                    QRect frontWindowRect = normalButtonIconRect.adjusted(0, 3, -3, 0);
                    painter->setPen(textColor);
                    painter->drawRect(frontWindowRect.adjusted(0, 0, -1, -1));
                    painter->drawLine(frontWindowRect.left() + 1, frontWindowRect.top() + 1,
                                      frontWindowRect.right() - 1, frontWindowRect.top() + 1);
                    painter->setPen(textAlphaColor);
                    const QPoint points[4] = {
                        frontWindowRect.topLeft(),
                        frontWindowRect.topRight(),
                        frontWindowRect.bottomLeft(),
                        frontWindowRect.bottomRight()
                    };
                    painter->drawPoints(points, 4);

                    QRect backWindowRect = normalButtonIconRect.adjusted(3, 0, 0, -3);
                    QRegion clipRegion = backWindowRect;
                    clipRegion -= frontWindowRect;
                    painter->save();
                    painter->setClipRegion(clipRegion);
                    painter->setPen(textColor);
                    painter->drawRect(backWindowRect.adjusted(0, 0, -1, -1));
                    painter->drawLine(backWindowRect.left() + 1, backWindowRect.top() + 1,
                                      backWindowRect.right() - 1, backWindowRect.top() + 1);
                    painter->setPen(textAlphaColor);
                    const QPoint points2[4] = {
                        backWindowRect.topLeft(),
                        backWindowRect.topRight(),
                        backWindowRect.bottomLeft(),
                        backWindowRect.bottomRight()
                    };
                    painter->drawPoints(points2, 4);
                    painter->restore();
                }
            }

            // context help button
            if (titleBar->subControls & SC_TitleBarContextHelpButton
                    && (titleBar->titleBarFlags & Qt::WindowContextHelpButtonHint)) {
                QRect contextHelpButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarContextHelpButton, widget);
                if (contextHelpButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarContextHelpButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarContextHelpButton) && (titleBar->state & State_Sunken);
                    qt_fusion_draw_mdibutton(painter, titleBar, contextHelpButtonRect, hover, sunken);
                    QImage image(qt_titlebar_context_help);
                    QColor alpha = textColor;
                    alpha.setAlpha(128);
                    image.setColor(1, textColor.rgba());
                    image.setColor(2, alpha.rgba());
                    painter->setRenderHint(QPainter::SmoothPixmapTransform);
                    painter->drawImage(contextHelpButtonRect.adjusted(4, 4, -4, -4), image);
                }
            }

            // shade button
            if (titleBar->subControls & SC_TitleBarShadeButton && (titleBar->titleBarFlags & Qt::WindowShadeButtonHint)) {
                QRect shadeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarShadeButton, widget);
                if (shadeButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarShadeButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarShadeButton) && (titleBar->state & State_Sunken);
                    qt_fusion_draw_mdibutton(painter, titleBar, shadeButtonRect, hover, sunken);
                    QPixmap arrow = colorizedImage(QLatin1String(":/bitmaps/style/arrow.png"), textColor);
                    painter->drawPixmap(shadeButtonRect.adjusted(5, 7, -5, -7), arrow);
                }
            }

            // unshade button
            if (titleBar->subControls & SC_TitleBarUnshadeButton && (titleBar->titleBarFlags & Qt::WindowShadeButtonHint)) {
                QRect unshadeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarUnshadeButton, widget);
                if (unshadeButtonRect.isValid()) {
                    bool hover = (titleBar->activeSubControls & SC_TitleBarUnshadeButton) && (titleBar->state & State_MouseOver);
                    bool sunken = (titleBar->activeSubControls & SC_TitleBarUnshadeButton) && (titleBar->state & State_Sunken);
                    qt_fusion_draw_mdibutton(painter, titleBar, unshadeButtonRect, hover, sunken);
                    QPixmap arrow = colorizedImage(QLatin1String(":/bitmaps/style/arrow.png"), textColor, 180);
                    painter->drawPixmap(unshadeButtonRect.adjusted(5, 7, -5, -7), arrow);
                }
            }

            if ((titleBar->subControls & SC_TitleBarSysMenu) && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
                QRect iconRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarSysMenu, widget);
                if (iconRect.isValid()) {
                    if (!titleBar->icon.isNull()) {
                        titleBar->icon.paint(painter, iconRect);
                    } else {
                        QStyleOption tool(0);
                        tool.palette = titleBar->palette;
                        QPixmap pm = standardIcon(SP_TitleBarMenuButton, &tool, widget).pixmap(16, 16);
                        tool.rect = iconRect;
                        painter->save();
                        proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pm);
                        painter->restore();
                    }
                }
            }
        }
        painter->restore();
        break;
    case CC_ScrollBar:
        painter->save();
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            bool horizontal = scrollBar->orientation == Qt::Horizontal;
            bool sunken = scrollBar->state & State_Sunken;

            QRect scrollBarSubLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSubLine, widget);
            QRect scrollBarAddLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarAddLine, widget);
            QRect scrollBarSlider = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSlider, widget);
            QRect scrollBarGroove = proxy()->subControlRect(control, scrollBar, SC_ScrollBarGroove, widget);

            QRect rect = option->rect;
            QColor alphaOutline = outline;
            alphaOutline.setAlpha(180);

            QColor arrowColor = option->palette.foreground().color();
            arrowColor.setAlpha(220);

            // Paint groove
            if (scrollBar->subControls & SC_ScrollBarGroove) {
                QLinearGradient gradient(rect.center().x(), rect.top(),
                                         rect.center().x(), rect.bottom());
                if (!horizontal)
                    gradient = QLinearGradient(rect.left(), rect.center().y(),
                                               rect.right(), rect.center().y());
                gradient.setColorAt(0, buttonColor.darker(107));
                gradient.setColorAt(0.1, buttonColor.darker(105));
                gradient.setColorAt(0.9, buttonColor.darker(105));
                gradient.setColorAt(1, buttonColor.darker(107));

                painter->fillRect(option->rect, gradient);
                painter->setPen(Qt::NoPen);
                painter->setPen(alphaOutline);
                if (horizontal)
                    painter->drawLine(rect.topLeft(), rect.topRight());
                else
                    painter->drawLine(rect.topLeft(), rect.bottomLeft());

                QColor subtleEdge = alphaOutline;
                subtleEdge.setAlpha(40);
                painter->setPen(subtleEdge);
                painter->setBrush(Qt::NoBrush);
                painter->save();
                painter->setClipRect(scrollBarGroove.adjusted(1, 0, -1, -3));
                painter->drawRect(scrollBarGroove.adjusted(1, 0, -1, -1));
                painter->restore();
            }

            QRect pixmapRect = scrollBarSlider;
            QLinearGradient gradient(pixmapRect.center().x(), pixmapRect.top(),
                                     pixmapRect.center().x(), pixmapRect.bottom());
            if (!horizontal)
                gradient = QLinearGradient(pixmapRect.left(), pixmapRect.center().y(),
                                           pixmapRect.right(), pixmapRect.center().y());

            QLinearGradient highlightedGradient = gradient;

            QColor midColor2 = mergedColors(gradientStartColor, gradientStopColor, 40);
            gradient.setColorAt(0, d->buttonColor(option->palette).lighter(108));
            gradient.setColorAt(1, d->buttonColor(option->palette));

            highlightedGradient.setColorAt(0, gradientStartColor.darker(102));
            highlightedGradient.setColorAt(1, gradientStopColor.lighter(102));

            // Paint slider
            if (scrollBar->subControls & SC_ScrollBarSlider) {
                QRect pixmapRect = scrollBarSlider;
                painter->setPen(QPen(alphaOutline, 0));
                if (option->state & State_Sunken && scrollBar->activeSubControls & SC_ScrollBarSlider)
                    painter->setBrush(midColor2);
                else if (option->state & State_MouseOver && scrollBar->activeSubControls & SC_ScrollBarSlider)
                    painter->setBrush(highlightedGradient);
                else
                    painter->setBrush(gradient);

                painter->drawRect(pixmapRect.adjusted(horizontal ? -1 : 0, horizontal ? 0 : -1, horizontal ? 0 : 1, horizontal ? 1 : 0));

                painter->setPen(d->innerContrastLine());
                painter->drawRect(scrollBarSlider.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, -1, -1));

                // Outer shadow
                //                  painter->setPen(subtleEdge);
                //                  if (horizontal) {
                ////                    painter->drawLine(scrollBarSlider.topLeft() + QPoint(-2, 0), scrollBarSlider.bottomLeft() + QPoint(2, 0));
                ////                    painter->drawLine(scrollBarSlider.topRight() + QPoint(-2, 0), scrollBarSlider.bottomRight() + QPoint(2, 0));
                //                  } else {
                ////                    painter->drawLine(pixmapRect.topLeft() + QPoint(0, -2), pixmapRect.bottomLeft() + QPoint(0, -2));
                ////                    painter->drawLine(pixmapRect.topRight() + QPoint(0, 2), pixmapRect.bottomRight() + QPoint(0, 2));
                //                  }
            }

            // The SubLine (up/left) buttons
            if (scrollBar->subControls & SC_ScrollBarSubLine) {
                if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken)
                    painter->setBrush(gradientStopColor);
                else if ((scrollBar->activeSubControls & SC_ScrollBarSubLine))
                    painter->setBrush(highlightedGradient);
                else
                    painter->setBrush(gradient);

                painter->setPen(Qt::NoPen);
                painter->drawRect(scrollBarSubLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, 0, 0));
                painter->setPen(QPen(alphaOutline, 1));
                if (option->state & State_Horizontal) {
                    if (option->direction == Qt::RightToLeft) {
                        pixmapRect.setLeft(scrollBarSubLine.left());
                        painter->drawLine(pixmapRect.topLeft(), pixmapRect.bottomLeft());
                    } else {
                        pixmapRect.setRight(scrollBarSubLine.right());
                        painter->drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
                    }
                } else {
                    pixmapRect.setBottom(scrollBarSubLine.bottom());
                    painter->drawLine(pixmapRect.bottomLeft(), pixmapRect.bottomRight());
                }

                painter->setBrush(Qt::NoBrush);
                painter->setPen(d->innerContrastLine());
                painter->drawRect(scrollBarSubLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0 ,  horizontal ? -2 : -1, horizontal ? -1 : -2));

                // Arrows
                int rotation = 0;
                if (option->state & State_Horizontal)
                    rotation = option->direction == Qt::LeftToRight ? -90 : 90;
                QRect upRect = scrollBarSubLine.translated(horizontal ? -2 : -1, 0);
                QPixmap arrowPixmap = colorizedImage(QLatin1String(":/bitmaps/style/arrow.png"), arrowColor, rotation);
                painter->drawPixmap(QRect(upRect.center().x() - arrowPixmap.width() / 4  + 2,
                                          upRect.center().y() - arrowPixmap.height() / 4 + 1,
                                          arrowPixmap.width()/2, arrowPixmap.height()/2), arrowPixmap);
            }

            // The AddLine (down/right) button
            if (scrollBar->subControls & SC_ScrollBarAddLine) {
                if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken)
                    painter->setBrush(gradientStopColor);
                else if ((scrollBar->activeSubControls & SC_ScrollBarAddLine))
                    painter->setBrush(midColor2);
                else
                    painter->setBrush(gradient);

                painter->setPen(Qt::NoPen);
                painter->drawRect(scrollBarAddLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, 0, 0));
                painter->setPen(QPen(alphaOutline, 1));
                if (option->state & State_Horizontal) {
                    if (option->direction == Qt::LeftToRight) {
                        pixmapRect.setLeft(scrollBarAddLine.left());
                        painter->drawLine(pixmapRect.topLeft(), pixmapRect.bottomLeft());
                    } else {
                        pixmapRect.setRight(scrollBarAddLine.right());
                        painter->drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
                    }
                } else {
                    pixmapRect.setTop(scrollBarAddLine.top());
                    painter->drawLine(pixmapRect.topLeft(), pixmapRect.topRight());
                }

                painter->setPen(d->innerContrastLine());
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(scrollBarAddLine.adjusted(1, 1, -1, -1));

                int rotation = 180;
                if (option->state & State_Horizontal)
                    rotation = option->direction == Qt::LeftToRight ? 90 : -90;
                QRect downRect = scrollBarAddLine.translated(-1, 1);
                QPixmap arrowPixmap = colorizedImage(QLatin1String(":/bitmaps/style/arrow.png"), arrowColor, rotation);
                painter->drawPixmap(QRect(downRect.center().x() - arrowPixmap.width() / 4 + 2,
                                          downRect.center().y() - arrowPixmap.height() / 4,
                                          arrowPixmap.width()/2, arrowPixmap.height()/2), arrowPixmap);
            }

        }
        painter->restore();
        break;;
    case CC_ComboBox:
        painter->save();
        if (const QStyleOptionComboBox *comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            bool hasFocus = option->state & State_HasFocus && option->state & State_KeyboardFocusChange;
            bool sunken = comboBox->state & State_On; // play dead, if combobox has no items
            bool isEnabled = (comboBox->state & State_Enabled);
            QPixmap cache;
            QString pixmapName = uniqueName(QLatin1String("combobox"), option, comboBox->rect.size());
            if (sunken)
                pixmapName += QLatin1String("-sunken");
            if (comboBox->editable)
                pixmapName += QLatin1String("-editable");
            if (isEnabled)
                pixmapName += QLatin1String("-enabled");

            if (!QPixmapCache::find(pixmapName, cache)) {
                cache = styleCachePixmap(comboBox->rect.size());
                cache.fill(Qt::transparent);
                QPainter cachePainter(&cache);
                QRect pixmapRect(0, 0, comboBox->rect.width(), comboBox->rect.height());
                QStyleOptionComboBox comboBoxCopy = *comboBox;
                comboBoxCopy.rect = pixmapRect;

                QRect rect = pixmapRect;
                QRect downArrowRect = proxy()->subControlRect(CC_ComboBox, &comboBoxCopy,
                                                              SC_ComboBoxArrow, widget);
                // Draw a line edit
                if (comboBox->editable) {
                    QStyleOptionFrame  buttonOption;
                    buttonOption.QStyleOption::operator=(*comboBox);
                    buttonOption.rect = rect;
                    buttonOption.state = (comboBox->state & (State_Enabled | State_MouseOver | State_HasFocus))
                            | State_KeyboardFocusChange; // Always show hig

                    if (sunken) {
                        buttonOption.state |= State_Sunken;
                        buttonOption.state &= ~State_MouseOver;
                    }

                    proxy()->drawPrimitive(PE_FrameLineEdit, &buttonOption, &cachePainter, widget);

                    // Draw button clipped
                    cachePainter.save();
                    cachePainter.setClipRect(downArrowRect.adjusted(0, 0, 1, 0));
                    buttonOption.rect.setLeft(comboBox->direction == Qt::LeftToRight ?
                                                  downArrowRect.left() - 6: downArrowRect.right() + 6);
                    proxy()->drawPrimitive(PE_PanelButtonCommand, &buttonOption, &cachePainter, widget);
                    cachePainter.restore();
                    cachePainter.setPen( QPen(hasFocus ? option->palette.highlight() : outline.lighter(110), 0));

                    if (!sunken) {
                        int borderSize = 1;
                        if (comboBox->direction == Qt::RightToLeft) {
                            cachePainter.drawLine(QPoint(downArrowRect.right() - 1, downArrowRect.top() + borderSize ),
                                                  QPoint(downArrowRect.right() - 1, downArrowRect.bottom() - borderSize));
                        } else {
                            cachePainter.drawLine(QPoint(downArrowRect.left() , downArrowRect.top() + borderSize),
                                                  QPoint(downArrowRect.left() , downArrowRect.bottom() - borderSize));
                        }
                    } else {
                        if (comboBox->direction == Qt::RightToLeft) {
                            cachePainter.drawLine(QPoint(downArrowRect.right(), downArrowRect.top() + 2),
                                                  QPoint(downArrowRect.right(), downArrowRect.bottom() - 2));

                        } else {
                            cachePainter.drawLine(QPoint(downArrowRect.left(), downArrowRect.top() + 2),
                                                  QPoint(downArrowRect.left(), downArrowRect.bottom() - 2));
                        }
                    }
                } else {
                    QStyleOptionButton buttonOption;
                    buttonOption.QStyleOption::operator=(*comboBox);
                    buttonOption.rect = rect;
                    buttonOption.state = comboBox->state & (State_Enabled | State_MouseOver | State_HasFocus | State_KeyboardFocusChange);
                    if (sunken) {
                        buttonOption.state |= State_Sunken;
                        buttonOption.state &= ~State_MouseOver;
                    }
                    proxy()->drawPrimitive(PE_PanelButtonCommand, &buttonOption, &cachePainter, widget);
                }
                if (comboBox->subControls & SC_ComboBoxArrow) {
                    // Draw the up/down arrow
                    QColor arrowColor = option->palette.buttonText().color();
                    arrowColor.setAlpha(220);
                    QPixmap downArrow = colorizedImage(QLatin1String(":/bitmaps/style/arrow.png"), arrowColor, 180);
                    cachePainter.drawPixmap(QRect(downArrowRect.center().x() - downArrow.width() / 4 + 1,
                                                  downArrowRect.center().y() - downArrow.height() / 4 + 1,
                                                  downArrow.width()/2, downArrow.height()/2), downArrow);
                }
                cachePainter.end();
                QPixmapCache::insert(pixmapName, cache);
            }
            painter->drawPixmap(comboBox->rect.topLeft(), cache);
        }
        painter->restore();
        break;
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QRect groove = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
            QRect handle = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);

            bool horizontal = slider->orientation == Qt::Horizontal;
            bool ticksAbove = slider->tickPosition & QSlider::TicksAbove;
            bool ticksBelow = slider->tickPosition & QSlider::TicksBelow;
            QColor activeHighlight = d->highlight(option->palette);
            QPixmap cache;
            QBrush oldBrush = painter->brush();
            QPen oldPen = painter->pen();
            QColor shadowAlpha(Qt::black);
            shadowAlpha.setAlpha(10);
            if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange)
                outline = d->highlightedOutline(option->palette);


            if ((option->subControls & SC_SliderGroove) && groove.isValid()) {
                QColor grooveColor;
                grooveColor.setHsv(buttonColor.hue(),
                                   qMin(255, (int)(buttonColor.saturation())),
                                   qMin(255, (int)(buttonColor.value()*0.9)));
                QString groovePixmapName = uniqueName(QLatin1String("slider_groove"), option, groove.size());
                QRect pixmapRect(0, 0, groove.width(), groove.height());

                // draw background groove
                if (!QPixmapCache::find(groovePixmapName, cache)) {
                    cache = styleCachePixmap(pixmapRect.size());
                    cache.fill(Qt::transparent);
                    QPainter groovePainter(&cache);
                    groovePainter.setRenderHint(QPainter::Antialiasing, true);
                    groovePainter.translate(0.5, 0.5);
                    QLinearGradient gradient;
                    if (horizontal) {
                        gradient.setStart(pixmapRect.center().x(), pixmapRect.top());
                        gradient.setFinalStop(pixmapRect.center().x(), pixmapRect.bottom());
                    }
                    else {
                        gradient.setStart(pixmapRect.left(), pixmapRect.center().y());
                        gradient.setFinalStop(pixmapRect.right(), pixmapRect.center().y());
                    }
                    groovePainter.setPen(QPen(outline, 0));
                    gradient.setColorAt(0, grooveColor.darker(110));
                    gradient.setColorAt(1, grooveColor.lighter(110));//palette.button().color().darker(115));
                    groovePainter.setBrush(gradient);
                    groovePainter.drawRoundedRect(pixmapRect.adjusted(1, 1, -2, -2), 1, 1);
                    groovePainter.end();
                    QPixmapCache::insert(groovePixmapName, cache);
                }
                painter->drawPixmap(groove.topLeft(), cache);

                // draw blue groove highlight
                QRect clipRect;
                groovePixmapName += QLatin1String("_blue");
                if (!QPixmapCache::find(groovePixmapName, cache)) {
                    cache = styleCachePixmap(pixmapRect.size());
                    cache.fill(Qt::transparent);
                    QPainter groovePainter(&cache);
                    QLinearGradient gradient;
                    if (horizontal) {
                        gradient.setStart(pixmapRect.center().x(), pixmapRect.top());
                        gradient.setFinalStop(pixmapRect.center().x(), pixmapRect.bottom());
                    }
                    else {
                        gradient.setStart(pixmapRect.left(), pixmapRect.center().y());
                        gradient.setFinalStop(pixmapRect.right(), pixmapRect.center().y());
                    }
                    QColor highlight = d->highlight(option->palette);
                    QColor highlightedoutline = highlight.darker(140);
                    if (qGray(outline.rgb()) > qGray(highlightedoutline.rgb()))
                        outline = highlightedoutline;


                    groovePainter.setRenderHint(QPainter::Antialiasing, true);
                    groovePainter.translate(0.5, 0.5);
                    groovePainter.setPen(QPen(outline, 0));
                    gradient.setColorAt(0, activeHighlight);
                    gradient.setColorAt(1, activeHighlight.lighter(130));
                    groovePainter.setBrush(gradient);
                    groovePainter.drawRoundedRect(pixmapRect.adjusted(1, 1, -2, -2), 1, 1);
                    groovePainter.setPen(d->innerContrastLine());
                    groovePainter.setBrush(Qt::NoBrush);
                    groovePainter.drawRoundedRect(pixmapRect.adjusted(2, 2, -3, -3), 1, 1);
                    groovePainter.end();
                    QPixmapCache::insert(groovePixmapName, cache);
                }
                if (horizontal) {
                    if (slider->upsideDown)
                        clipRect = QRect(handle.right(), groove.top(), groove.right() - handle.right(), groove.height());
                    else
                        clipRect = QRect(groove.left(), groove.top(), handle.left(), groove.height());
                } else {
                    if (slider->upsideDown)
                        clipRect = QRect(groove.left(), handle.bottom(), groove.width(), groove.height() - handle.bottom());
                    else
                        clipRect = QRect(groove.left(), groove.top(), groove.width(), handle.top() - groove.top());
                }
                painter->save();
                painter->setClipRect(clipRect.adjusted(0, 0, 1, 1));
                painter->drawPixmap(groove.topLeft(), cache);
                painter->restore();
            }

            if (option->subControls & SC_SliderTickmarks) {
                painter->setPen(outline);
                int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);
                int available = proxy()->pixelMetric(PM_SliderSpaceAvailable, slider, widget);
                int interval = slider->tickInterval;
                if (interval <= 0) {
                    interval = slider->singleStep;
                    if (QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, interval,
                                                        available)
                            - QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                              0, available) < 3)
                        interval = slider->pageStep;
                }
                if (interval <= 0)
                    interval = 1;

                int v = slider->minimum;
                int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
                while (v <= slider->maximum + 1) {
                    if (v == slider->maximum + 1 && interval == 1)
                        break;
                    const int v_ = qMin(v, slider->maximum);
                    int pos = sliderPositionFromValue(slider->minimum, slider->maximum,
                                                      v_, (horizontal
                                                           ? slider->rect.width()
                                                           : slider->rect.height()) - len,
                                                      slider->upsideDown) + len / 2;
                    int extra = 2 - ((v_ == slider->minimum || v_ == slider->maximum) ? 1 : 0);

                    if (horizontal) {
                        if (ticksAbove) {
                            painter->drawLine(pos, slider->rect.top() + extra,
                                              pos, slider->rect.top() + tickSize);
                        }
                        if (ticksBelow) {
                            painter->drawLine(pos, slider->rect.bottom() - extra,
                                              pos, slider->rect.bottom() - tickSize);
                        }
                    } else {
                        if (ticksAbove) {
                            painter->drawLine(slider->rect.left() + extra, pos,
                                              slider->rect.left() + tickSize, pos);
                        }
                        if (ticksBelow) {
                            painter->drawLine(slider->rect.right() - extra, pos,
                                              slider->rect.right() - tickSize, pos);
                        }
                    }
                    // in the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                        break;
                    v = nextInterval;
                }
            }

            // draw handle
            if ((option->subControls & SC_SliderHandle) ) {
                QString handlePixmapName = uniqueName(QLatin1String("slider_handle"), option, handle.size());
                if (!QPixmapCache::find(handlePixmapName, cache)) {
                    cache = styleCachePixmap(handle.size());
                    cache.fill(Qt::transparent);
                    QRect pixmapRect(0, 0, handle.width(), handle.height());
                    QPainter handlePainter(&cache);
                    QRect gradRect = pixmapRect.adjusted(2, 2, -2, -2);

                    // gradient fill
                    QRect r = pixmapRect.adjusted(1, 1, -2, -2);
                    QLinearGradient gradient = qt_fusion_gradient(gradRect, d->buttonColor(option->palette),horizontal ? TopDown : FromLeft);

                    handlePainter.setRenderHint(QPainter::Antialiasing, true);
                    handlePainter.translate(0.5, 0.5);

                    handlePainter.setPen(Qt::NoPen);
                    handlePainter.setBrush(QColor(0, 0, 0, 40));
                    handlePainter.drawRect(r.adjusted(-1, 2, 1, -2));

                    handlePainter.setPen(QPen(d->outline(option->palette), 1));
                    if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange)
                        handlePainter.setPen(QPen(d->highlightedOutline(option->palette), 1));

                    handlePainter.setBrush(gradient);
                    handlePainter.drawRoundedRect(r, 2, 2);
                    handlePainter.setBrush(Qt::NoBrush);
                    handlePainter.setPen(d->innerContrastLine());
                    handlePainter.drawRoundedRect(r.adjusted(1, 1, -1, -1), 2, 2);

                    QColor cornerAlpha = outline.darker(120);
                    cornerAlpha.setAlpha(80);

                    //handle shadow
                    handlePainter.setPen(shadowAlpha);
                    handlePainter.drawLine(QPoint(r.left() + 2, r.bottom() + 1), QPoint(r.right() - 2, r.bottom() + 1));
                    handlePainter.drawLine(QPoint(r.right() + 1, r.bottom() - 3), QPoint(r.right() + 1, r.top() + 4));
                    handlePainter.drawLine(QPoint(r.right() - 1, r.bottom()), QPoint(r.right() + 1, r.bottom() - 2));

                    handlePainter.end();
                    QPixmapCache::insert(handlePixmapName, cache);
                }

                painter->drawPixmap(handle.topLeft(), cache);
            }

            painter->setBrush(oldBrush);
            painter->setPen(oldPen);
        }
        break;
    case CC_Dial:
        if (const QStyleOptionSlider* dial = qstyleoption_cast<const QStyleOptionSlider *>(option))
            drawDial(dial, painter);
        break;
    default:
        QCommonStyle::drawComplexControl(control, option, painter, widget);
        break;
    }
}

/*!
  \reimp
*/
int CarlaStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch (metric)
    {
    case PM_SliderTickmarkOffset:
        return 4;
    case PM_HeaderMargin:
        return 2;
    case PM_ToolTipLabelFrameWidth:
        return 2;
    case PM_ButtonDefaultIndicator:
        return 0;
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        return 0;
    case PM_MessageBoxIconSize:
        return 48;
    case PM_ListViewIconSize:
        return 24;
    case PM_DialogButtonsSeparator:
    case PM_ScrollBarSliderMin:
        return 26;
    case PM_TitleBarHeight:
        return 24;
    case PM_ScrollBarExtent:
        return 14;
    case PM_SliderThickness:
        return 15;
    case PM_SliderLength:
        return 15;
    case PM_DockWidgetTitleMargin:
        return 1;
    case PM_DefaultFrameWidth:
        return 1;
    case PM_SpinBoxFrameWidth:
        return 2;
    case PM_MenuVMargin:
    case PM_MenuHMargin:
        return 0;
    case PM_MenuPanelWidth:
        return 0;
    case PM_MenuBarItemSpacing:
        return 6;
    case PM_MenuBarVMargin:
        return 0;
    case PM_MenuBarHMargin:
        return 0;
    case PM_MenuBarPanelWidth:
        return 0;
    case PM_ToolBarHandleExtent:
        return 9;
    case PM_ToolBarItemSpacing:
        return 1;
    case PM_ToolBarFrameWidth:
        return 2;
    case PM_ToolBarItemMargin:
        return 2;
    case PM_SmallIconSize:
        return 16;
    case PM_ButtonIconSize:
        return 16;
    case PM_DockWidgetTitleBarButtonMargin:
        return 2;
    case PM_MaximumDragDistance:
        return -1;
    case PM_TabCloseIndicatorWidth:
    case PM_TabCloseIndicatorHeight:
        return 20;
    case PM_TabBarTabVSpace:
        return 12;
    case PM_TabBarTabOverlap:
        return 1;
    case PM_TabBarBaseOverlap:
        return 2;
    case PM_SubMenuOverlap:
        return -1;
    case PM_DockWidgetHandleExtent:
    case PM_SplitterWidth:
        return 4;
    case PM_IndicatorHeight:
    case PM_IndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
    case PM_ExclusiveIndicatorWidth:
        return 14;
    case PM_ScrollView_ScrollBarSpacing:
        return 0;
    default:
        break;
    }
    return QCommonStyle::pixelMetric(metric, option, widget);
}

/*!
  \reimp
*/
QSize CarlaStyle::sizeFromContents(ContentsType type, const QStyleOption* option,
                                   const QSize& size, const QWidget* widget) const
{
    QSize newSize = QCommonStyle::sizeFromContents(type, option, size, widget);

    switch (type)
    {
    case CT_PushButton:
        if (const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton *>(option))
        {
            if (!btn->text.isEmpty() && newSize.width() < 80)
                newSize.setWidth(80);
            if (!btn->icon.isNull() && btn->iconSize.height() > 16)
                newSize -= QSize(0, 2);
        }
        break;

    case CT_GroupBox:
        if (option)
        {
            int topMargin = qMax(pixelMetric(PM_ExclusiveIndicatorHeight), option->fontMetrics.height()) + groupBoxTopMargin;
            newSize += QSize(10, topMargin); // Add some space below the groupbox
        }
        break;

    case CT_RadioButton:
    case CT_CheckBox:
        newSize += QSize(0, 1);
        break;

    case CT_ToolButton:
        newSize += QSize(3, 3);
        break;

    case CT_SpinBox:
        newSize += QSize(0, -3);
        break;

    case CT_ComboBox:
        newSize += QSize(2, 4);
        break;

    case CT_LineEdit:
        newSize += QSize(0, 4);
        break;

    case CT_MenuBarItem:
        newSize += QSize(8, 5);
        break;

    case CT_MenuItem:
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            int w = newSize.width();
            int maxpmw = menuItem->maxIconWidth;
            int tabSpacing = 20;
            if (menuItem->text.contains(QLatin1Char('\t')))
                w += tabSpacing;
            else if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu)
                w += 2 * CarlaStylePrivate::menuArrowHMargin;
            else if (menuItem->menuItemType == QStyleOptionMenuItem::DefaultItem) {
                QFontMetrics fm(menuItem->font);
                QFont fontBold = menuItem->font;
                fontBold.setBold(true);
                QFontMetrics fmBold(fontBold);
                w += fmBold.width(menuItem->text) - fm.width(menuItem->text);
            }
            int checkcol = qMax<int>(maxpmw, CarlaStylePrivate::menuCheckMarkWidth); // Windows always shows a check column
            w += checkcol;
            w += int(CarlaStylePrivate::menuRightBorder) + 10;
            newSize.setWidth(w);
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                if (!menuItem->text.isEmpty()) {
                    newSize.setHeight(menuItem->fontMetrics.height());
                }
            }
            else if (!menuItem->icon.isNull())
            {
                if (const QComboBox *combo = qobject_cast<const QComboBox*>(widget)) {
                    newSize.setHeight(qMax(combo->iconSize().height() + 2, newSize.height()));
                }
            }
            newSize.setWidth(newSize.width() + 12);
            newSize.setWidth(qMax(newSize.width(), 120));
        }
        break;

    case CT_SizeGrip:
        newSize += QSize(4, 4);
        break;

    case CT_MdiControls:
        if (const QStyleOptionComplex *styleOpt = qstyleoption_cast<const QStyleOptionComplex *>(option))
        {
            int width = 0;
            if (styleOpt->subControls & SC_MdiMinButton)
                width += 19 + 1;
            if (styleOpt->subControls & SC_MdiNormalButton)
                width += 19 + 1;
            if (styleOpt->subControls & SC_MdiCloseButton)
                width += 19 + 1;
            newSize = QSize(width, 19);
        }
        else
        {
            newSize = QSize(60, 19);
        }
        break;

    default:
        break;
    }
    return newSize;
}

void CarlaStyle::polish(QApplication* app)
{
    QCommonStyle::polish(app);
}

void CarlaStyle::polish(QPalette& pal)
{
    QCommonStyle::polish(pal);
}

/*!
  \reimp
*/
void CarlaStyle::polish(QWidget *widget)
{
    QCommonStyle::polish(widget);
    if (qobject_cast<QAbstractButton*>(widget)
            || qobject_cast<QComboBox *>(widget)
            || qobject_cast<QProgressBar *>(widget)
            || qobject_cast<QScrollBar *>(widget)
            || qobject_cast<QSplitterHandle *>(widget)
            || qobject_cast<QAbstractSlider *>(widget)
            || qobject_cast<QAbstractSpinBox *>(widget)
            || (widget->inherits("QDockSeparator"))
            || (widget->inherits("QDockWidgetSeparator"))
            ) {
        widget->setAttribute(Qt::WA_Hover, true);
    }
}


void CarlaStyle::unpolish(QApplication* app)
{
    QCommonStyle::unpolish(app);
}

/*!
  \reimp
*/
void CarlaStyle::unpolish(QWidget *widget)
{
    QCommonStyle::unpolish(widget);
    if (qobject_cast<QAbstractButton*>(widget)
            || qobject_cast<QComboBox *>(widget)
            || qobject_cast<QProgressBar *>(widget)
            || qobject_cast<QScrollBar *>(widget)
            || qobject_cast<QSplitterHandle *>(widget)
            || qobject_cast<QAbstractSlider *>(widget)
            || qobject_cast<QAbstractSpinBox *>(widget)
            || (widget->inherits("QDockSeparator"))
            || (widget->inherits("QDockWidgetSeparator"))
            ) {
        widget->setAttribute(Qt::WA_Hover, false);
    }
}

/*!
  \reimp
*/
QRect CarlaStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                   SubControl subControl, const QWidget *widget) const
{
    QRect rect = QCommonStyle::subControlRect(control, option, subControl, widget);

    switch (control) {
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);
            switch (subControl) {
            case SC_SliderHandle: {
                if (slider->orientation == Qt::Horizontal) {
                    rect.setHeight(proxy()->pixelMetric(PM_SliderThickness));
                    rect.setWidth(proxy()->pixelMetric(PM_SliderLength));
                    int centerY = slider->rect.center().y() - rect.height() / 2;
                    if (slider->tickPosition & QSlider::TicksAbove)
                        centerY += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        centerY -= tickSize;
                    rect.moveTop(centerY);
                } else {
                    rect.setWidth(proxy()->pixelMetric(PM_SliderThickness));
                    rect.setHeight(proxy()->pixelMetric(PM_SliderLength));
                    int centerX = slider->rect.center().x() - rect.width() / 2;
                    if (slider->tickPosition & QSlider::TicksAbove)
                        centerX += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        centerX -= tickSize;
                    rect.moveLeft(centerX);
                }
            }
                break;
            case SC_SliderGroove: {
                QPoint grooveCenter = slider->rect.center();
                if (slider->orientation == Qt::Horizontal) {
                    rect.setHeight(7);
                    if (slider->tickPosition & QSlider::TicksAbove)
                        grooveCenter.ry() += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        grooveCenter.ry() -= tickSize;
                } else {
                    rect.setWidth(7);
                    if (slider->tickPosition & QSlider::TicksAbove)
                        grooveCenter.rx() += tickSize;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        grooveCenter.rx() -= tickSize;
                }
                rect.moveCenter(grooveCenter);
                break;
            }
            default:
                break;
            }
        }
        break;
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QSize bs;
            const float center = spinbox->rect.height() / 2.0;
            const int fw = spinbox->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) : 0;
            const int y = 1;
            bs.setHeight(qMax(8, int(floor(center) - y)));
            bs.setWidth(14);
            int x, lx, rx;
            x = spinbox->rect.width() - y - bs.width();
            lx = fw;
            rx = x - fw;
            switch (subControl) {
            case SC_SpinBoxUp:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                rect = QRect(x, y, bs.width(), bs.height());
                break;
            case SC_SpinBoxDown:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();

                rect = QRect(x, ceil(center), bs.width(), bs.height());
                break;
            case SC_SpinBoxEditField:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons) {
                    rect = QRect(lx, fw, spinbox->rect.width() - 2*fw, spinbox->rect.height() - 2*fw);
                } else {
                    rect = QRect(lx, fw, rx - qMax(fw - 1, 0), spinbox->rect.height() - 2*fw);
                }
                break;
            case SC_SpinBoxFrame:
                rect = spinbox->rect;
            default:
                break;
            }
            rect = visualRect(spinbox->direction, spinbox->rect, rect);
        }
        break;

    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            rect = option->rect;
            if (subControl == SC_GroupBoxFrame)
                return rect.adjusted(0, 0, 0, 0);
            else if (subControl == SC_GroupBoxContents) {
                QRect frameRect = option->rect.adjusted(0, 0, 0, -groupBoxBottomMargin);
                int margin = 3;
                int leftMarginExtension = 0;
                int topMargin = qMax(pixelMetric(PM_ExclusiveIndicatorHeight), option->fontMetrics.height()) + groupBoxTopMargin;
                return frameRect.adjusted(leftMarginExtension + margin, margin + topMargin, -margin, -margin - groupBoxBottomMargin);
            }

            QSize textSize = option->fontMetrics.boundingRect(groupBox->text).size() + QSize(2, 2);
            int indicatorWidth = proxy()->pixelMetric(PM_IndicatorWidth, option, widget);
            int indicatorHeight = proxy()->pixelMetric(PM_IndicatorHeight, option, widget);
            rect = QRect();
            if (subControl == SC_GroupBoxCheckBox) {
                rect.setWidth(indicatorWidth);
                rect.setHeight(indicatorHeight);
                rect.moveTop(textSize.height() > indicatorHeight ? (textSize.height() - indicatorHeight) / 2 : 0);
                rect.moveLeft(1);
            } else if (subControl == SC_GroupBoxLabel) {
                rect.setSize(textSize);
                rect.moveTop(1);
                if (option->subControls & QStyle::SC_GroupBoxCheckBox)
                    rect.translate(indicatorWidth + 5, 0);
            }
            return visualRect(option->direction, option->rect, rect);
        }

        return rect;

    case CC_ComboBox:
        switch (subControl) {
        case SC_ComboBoxArrow:
            rect = visualRect(option->direction, option->rect, rect);
            rect.setRect(rect.right() - 18, rect.top() - 2,
                         19, rect.height() + 4);
            rect = visualRect(option->direction, option->rect, rect);
            break;
        case SC_ComboBoxEditField: {
            int frameWidth = 2;
            rect = visualRect(option->direction, option->rect, rect);
            rect.setRect(option->rect.left() + frameWidth, option->rect.top() + frameWidth,
                         option->rect.width() - 19 - 2 * frameWidth,
                         option->rect.height() - 2 * frameWidth);
            if (const QStyleOptionComboBox *box = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
                if (!box->editable) {
                    rect.adjust(2, 0, 0, 0);
                    if (box->state & (State_Sunken | State_On))
                        rect.translate(1, 1);
                }
            }
            rect = visualRect(option->direction, option->rect, rect);
            break;
        }
        default:
            break;
        }
        break;
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            SubControl sc = subControl;
            QRect &ret = rect;
            const int indent = 3;
            const int controlTopMargin = 3;
            const int controlBottomMargin = 3;
            const int controlWidthMargin = 2;
            const int controlHeight = tb->rect.height() - controlTopMargin - controlBottomMargin ;
            const int delta = controlHeight + controlWidthMargin;
            int offset = 0;

            bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
            bool isMaximized = tb->titleBarState & Qt::WindowMaximized;

            switch (sc) {
            case SC_TitleBarLabel:
                if (tb->titleBarFlags & (Qt::WindowTitleHint | Qt::WindowSystemMenuHint)) {
                    ret = tb->rect;
                    if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                        ret.adjust(delta, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowMinimizeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowMaximizeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowShadeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                }
                break;
            case SC_TitleBarContextHelpButton:
                if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                    offset += delta;
                // fall through
            case SC_TitleBarMinButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarMinButton)
                    break;
                // fall through
            case SC_TitleBarNormalButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarNormalButton)
                    break;
                // fall through
            case SC_TitleBarMaxButton:
                if (!isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarMaxButton)
                    break;
                // fall through
            case SC_TitleBarShadeButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarShadeButton)
                    break;
                // fall through
            case SC_TitleBarUnshadeButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarUnshadeButton)
                    break;
                // fall through
            case SC_TitleBarCloseButton:
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                    offset += delta;
                else if (sc == SC_TitleBarCloseButton)
                    break;
                ret.setRect(tb->rect.right() - indent - offset, tb->rect.top() + controlTopMargin,
                            controlHeight, controlHeight);
                break;
            case SC_TitleBarSysMenu:
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                    ret.setRect(tb->rect.left() + controlWidthMargin + indent, tb->rect.top() + controlTopMargin,
                                controlHeight, controlHeight);
                }
                break;
            default:
                break;
            }
            ret = visualRect(tb->direction, tb->rect, ret);
        }
        break;
    default:
        break;
    }

    return rect;
}

/*!
  \reimp
*/
QPixmap CarlaStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption* opt, const QWidget* widget) const
{
#if 0 // ndef QT_NO_IMAGEFORMAT_XPM
    switch (standardPixmap) {
    case SP_TitleBarNormalButton:
        return QPixmap((const char **)dock_widget_restore_xpm);
    case SP_TitleBarMinButton:
        return QPixmap((const char **)workspace_minimize);
    case SP_TitleBarCloseButton:
    case SP_DockWidgetCloseButton:
        return QPixmap((const char **)dock_widget_close_xpm);
    default:
        break;
    }
#endif //QT_NO_IMAGEFORMAT_XPM

    QPixmap pixmap = QCommonStyle::standardPixmap(standardPixmap, opt, widget);

    if(!pixmap.isNull())
        return pixmap;

#if 0 // ndef QT_NO_IMAGEFORMAT_XPM
    switch (standardPixmap) {
    case SP_TitleBarMenuButton:
        return QPixmap(qt_menu_xpm);
    case SP_TitleBarShadeButton:
        return QPixmap(qt_shade_xpm);
    case SP_TitleBarUnshadeButton:
        return QPixmap(qt_unshade_xpm);
    case SP_TitleBarMaxButton:
        return QPixmap(qt_maximize_xpm);
    case SP_TitleBarCloseButton:
        return QPixmap(qt_close_xpm);
    case SP_TitleBarContextHelpButton:
        return QPixmap(qt_help_xpm);
    case SP_MessageBoxInformation:
        return QPixmap(information_xpm);
    case SP_MessageBoxWarning:
        return QPixmap(warning_xpm);
    case SP_MessageBoxCritical:
        return QPixmap(critical_xpm);
    case SP_MessageBoxQuestion:
        return QPixmap(question_xpm);
    default:
        break;
    }
#endif //QT_NO_IMAGEFORMAT_XPM

    return QPixmap();
}

/*!
  \reimp
*/
int CarlaStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget,
                          QStyleHintReturn* returnData) const
{
    switch (hint)
    {
    case SH_Slider_SnapToValue:
    case SH_PrintDialog_RightAlignButtons:
    case SH_FontDialog_SelectAssociatedText:
    case SH_MenuBar_AltKeyNavigation:
    case SH_ComboBox_ListMouseTracking:
    case SH_ScrollBar_StopMouseOverSlider:
    case SH_ScrollBar_MiddleClickAbsolutePosition:
    case SH_TitleBar_AutoRaise:
    case SH_TitleBar_NoBorder:
    case SH_ItemView_ShowDecorationSelected:
    case SH_ItemView_ArrowKeysNavigateIntoChildren:
    case SH_ItemView_ChangeHighlightOnFocus:
    case SH_MenuBar_MouseTracking:
    case SH_Menu_MouseTracking:
        return 1;

    case SH_ComboBox_Popup:
    case SH_EtchDisabledText:
    case SH_ToolBox_SelectedPageTitleBold:
    case SH_ScrollView_FrameOnlyAroundContents:
    case SH_Menu_AllowActiveAndDisabled:
    case SH_MainWindow_SpaceBelowMenuBar:
    //case SH_DialogButtonBox_ButtonsHaveIcons:
    case SH_MessageBox_CenterButtons:
    case SH_RubberBand_Mask:
    case SH_UnderlineShortcut:
        return 0;

    case SH_Table_GridLineColor:
        return option ? option->palette.background().color().darker(120).rgb() : 0;

    case SH_MessageBox_TextInteractionFlags:
        return Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;

    case SH_WizardStyle:
        return QWizard::ClassicStyle;

    case SH_Menu_SubMenuPopupDelay:
        return 225; // default from GtkMenu

    case SH_WindowFrame_Mask:
        if (QStyleHintReturnMask* mask = qstyleoption_cast<QStyleHintReturnMask*>(returnData)) {
            //left rounded corner
            mask->region = option->rect;
            mask->region -= QRect(option->rect.left(), option->rect.top(), 5, 1);
            mask->region -= QRect(option->rect.left(), option->rect.top() + 1, 3, 1);
            mask->region -= QRect(option->rect.left(), option->rect.top() + 2, 2, 1);
            mask->region -= QRect(option->rect.left(), option->rect.top() + 3, 1, 2);

            //right rounded corner
            mask->region -= QRect(option->rect.right() - 4, option->rect.top(), 5, 1);
            mask->region -= QRect(option->rect.right() - 2, option->rect.top() + 1, 3, 1);
            mask->region -= QRect(option->rect.right() - 1, option->rect.top() + 2, 2, 1);
            mask->region -= QRect(option->rect.right() , option->rect.top() + 3, 1, 2);
            return 1;
        }
    default:
        break;
    }
    return QCommonStyle::styleHint(hint, option, widget, returnData);
}

/*! \reimp */
QRect CarlaStyle::subElementRect(SubElement sr, const QStyleOption *opt, const QWidget *w) const
{
    QRect r = QCommonStyle::subElementRect(sr, opt, w);
    switch (sr) {
    case SE_ProgressBarLabel:
    case SE_ProgressBarContents:
    case SE_ProgressBarGroove:
        return opt->rect;
    case SE_PushButtonFocusRect:
        r.adjust(0, 1, 0, -1);
        break;
    case SE_DockWidgetTitleBarText: {
        if (const QStyleOptionDockWidget *titlebar = qstyleoption_cast<const QStyleOptionDockWidget*>(opt)) {
            Q_UNUSED(titlebar);
            bool verticalTitleBar = false;
            if (verticalTitleBar) {
                r.adjust(0, 0, 0, -4);
            } else {
                if (opt->direction == Qt::LeftToRight)
                    r.adjust(4, 0, 0, 0);
                else
                    r.adjust(0, 0, -4, 0);
            }
        }

        break;
    }
    default:
        break;
    }
    return r;
}
