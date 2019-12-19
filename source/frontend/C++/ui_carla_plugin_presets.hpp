/********************************************************************************
** Form generated from reading UI file 'carla_plugin_presets.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_PLUGIN_PRESETS_H
#define UI_CARLA_PLUGIN_PRESETS_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "widgets/digitalpeakmeter.h"
#include "widgets/ledbutton.h"
#include "widgets/pixmapbutton.h"

QT_BEGIN_NAMESPACE

class Ui_PluginWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *layout_top;
    QWidget *w_buttons;
    QHBoxLayout *horizontalLayout;
    PixmapButton *b_enable;
    PixmapButton *b_gui;
    PixmapButton *b_edit;
    QSpacerItem *spacer_namesep1;
    QFrame *line_2;
    QSpacerItem *spacer_namesep2;
    QLabel *label_name;
    QSpacerItem *spacer_namesep3;
    QHBoxLayout *layout_presets;
    QLabel *label_presets;
    QComboBox *cb_presets;
    QHBoxLayout *layout_leds;
    LEDButton *led_control;
    LEDButton *led_midi;
    LEDButton *led_audio_in;
    LEDButton *led_audio_out;
    QFrame *line;
    QHBoxLayout *layout_bottom;
    QWidget *w_knobs_left;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer;
    QWidget *w_knobs_right;
    QHBoxLayout *horizontalLayout_3;
    QVBoxLayout *layout_peaks;
    DigitalPeakMeter *peak_in;
    DigitalPeakMeter *peak_out;

    void setupUi(QFrame *PluginWidget)
    {
        if (PluginWidget->objectName().isEmpty())
            PluginWidget->setObjectName(QString::fromUtf8("PluginWidget"));
        PluginWidget->resize(484, 65);
        PluginWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        verticalLayout = new QVBoxLayout(PluginWidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(4, 3, 6, 3);
        layout_top = new QHBoxLayout();
        layout_top->setSpacing(1);
        layout_top->setObjectName(QString::fromUtf8("layout_top"));
        w_buttons = new QWidget(PluginWidget);
        w_buttons->setObjectName(QString::fromUtf8("w_buttons"));
        w_buttons->setMinimumSize(QSize(72, 24));
        w_buttons->setMaximumSize(QSize(72, 24));
        horizontalLayout = new QHBoxLayout(w_buttons);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        b_enable = new PixmapButton(w_buttons);
        b_enable->setObjectName(QString::fromUtf8("b_enable"));
        b_enable->setMinimumSize(QSize(24, 24));
        b_enable->setMaximumSize(QSize(24, 24));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/bitmaps/button_off.png"), QSize(), QIcon::Normal, QIcon::Off);
        b_enable->setIcon(icon);
        b_enable->setIconSize(QSize(24, 24));
        b_enable->setCheckable(true);
        b_enable->setFlat(true);

        horizontalLayout->addWidget(b_enable);

        b_gui = new PixmapButton(w_buttons);
        b_gui->setObjectName(QString::fromUtf8("b_gui"));
        b_gui->setMinimumSize(QSize(24, 24));
        b_gui->setMaximumSize(QSize(24, 24));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/bitmaps/button_gui.png"), QSize(), QIcon::Normal, QIcon::Off);
        b_gui->setIcon(icon1);
        b_gui->setIconSize(QSize(24, 24));
        b_gui->setCheckable(true);
        b_gui->setFlat(true);

        horizontalLayout->addWidget(b_gui);

        b_edit = new PixmapButton(w_buttons);
        b_edit->setObjectName(QString::fromUtf8("b_edit"));
        b_edit->setMinimumSize(QSize(24, 24));
        b_edit->setMaximumSize(QSize(24, 24));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/bitmaps/button_edit.png"), QSize(), QIcon::Normal, QIcon::Off);
        b_edit->setIcon(icon2);
        b_edit->setIconSize(QSize(24, 24));
        b_edit->setCheckable(true);
        b_edit->setFlat(true);

        horizontalLayout->addWidget(b_edit);


        layout_top->addWidget(w_buttons);

        spacer_namesep1 = new QSpacerItem(6, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        layout_top->addItem(spacer_namesep1);

        line_2 = new QFrame(PluginWidget);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setMinimumSize(QSize(0, 20));
        line_2->setMaximumSize(QSize(16777215, 20));
        line_2->setLineWidth(0);
        line_2->setMidLineWidth(1);
        line_2->setFrameShape(QFrame::VLine);
        line_2->setFrameShadow(QFrame::Sunken);

        layout_top->addWidget(line_2);

        spacer_namesep2 = new QSpacerItem(8, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        layout_top->addItem(spacer_namesep2);

        label_name = new QLabel(PluginWidget);
        label_name->setObjectName(QString::fromUtf8("label_name"));
        label_name->setTextFormat(Qt::PlainText);
        label_name->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        layout_top->addWidget(label_name);

        spacer_namesep3 = new QSpacerItem(20, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        layout_top->addItem(spacer_namesep3);

        layout_presets = new QHBoxLayout();
        layout_presets->setSpacing(4);
        layout_presets->setObjectName(QString::fromUtf8("layout_presets"));
        layout_presets->setContentsMargins(4, -1, 4, -1);
        label_presets = new QLabel(PluginWidget);
        label_presets->setObjectName(QString::fromUtf8("label_presets"));
        label_presets->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        layout_presets->addWidget(label_presets);

        cb_presets = new QComboBox(PluginWidget);
        cb_presets->setObjectName(QString::fromUtf8("cb_presets"));
        cb_presets->setMinimumSize(QSize(125, 0));

        layout_presets->addWidget(cb_presets);


        layout_top->addLayout(layout_presets);

        layout_leds = new QHBoxLayout();
        layout_leds->setSpacing(1);
        layout_leds->setObjectName(QString::fromUtf8("layout_leds"));
        led_control = new LEDButton(PluginWidget);
        led_control->setObjectName(QString::fromUtf8("led_control"));
        led_control->setMinimumSize(QSize(14, 14));
        led_control->setMaximumSize(QSize(14, 14));
        led_control->setCheckable(true);

        layout_leds->addWidget(led_control);

        led_midi = new LEDButton(PluginWidget);
        led_midi->setObjectName(QString::fromUtf8("led_midi"));
        led_midi->setMinimumSize(QSize(14, 14));
        led_midi->setMaximumSize(QSize(14, 14));
        led_midi->setCheckable(true);

        layout_leds->addWidget(led_midi);

        led_audio_in = new LEDButton(PluginWidget);
        led_audio_in->setObjectName(QString::fromUtf8("led_audio_in"));
        led_audio_in->setMinimumSize(QSize(14, 14));
        led_audio_in->setMaximumSize(QSize(14, 14));
        led_audio_in->setCheckable(true);

        layout_leds->addWidget(led_audio_in);

        led_audio_out = new LEDButton(PluginWidget);
        led_audio_out->setObjectName(QString::fromUtf8("led_audio_out"));
        led_audio_out->setMinimumSize(QSize(14, 14));
        led_audio_out->setMaximumSize(QSize(14, 14));
        led_audio_out->setCheckable(true);

        layout_leds->addWidget(led_audio_out);


        layout_top->addLayout(layout_leds);


        verticalLayout->addLayout(layout_top);

        line = new QFrame(PluginWidget);
        line->setObjectName(QString::fromUtf8("line"));
        line->setLineWidth(0);
        line->setMidLineWidth(1);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout->addWidget(line);

        layout_bottom = new QHBoxLayout();
        layout_bottom->setSpacing(1);
        layout_bottom->setObjectName(QString::fromUtf8("layout_bottom"));
        layout_bottom->setContentsMargins(12, 4, 12, -1);
        w_knobs_left = new QWidget(PluginWidget);
        w_knobs_left->setObjectName(QString::fromUtf8("w_knobs_left"));
        horizontalLayout_4 = new QHBoxLayout(w_knobs_left);
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));

        layout_bottom->addWidget(w_knobs_left);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        layout_bottom->addItem(horizontalSpacer);

        w_knobs_right = new QWidget(PluginWidget);
        w_knobs_right->setObjectName(QString::fromUtf8("w_knobs_right"));
        horizontalLayout_3 = new QHBoxLayout(w_knobs_right);
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(12, 0, 0, 0);

        layout_bottom->addWidget(w_knobs_right);

        layout_peaks = new QVBoxLayout();
        layout_peaks->setSpacing(4);
        layout_peaks->setObjectName(QString::fromUtf8("layout_peaks"));
        layout_peaks->setContentsMargins(4, 0, 4, 2);
        peak_in = new DigitalPeakMeter(PluginWidget);
        peak_in->setObjectName(QString::fromUtf8("peak_in"));
        peak_in->setMinimumSize(QSize(150, 0));
        peak_in->setMaximumSize(QSize(150, 16777215));

        layout_peaks->addWidget(peak_in);

        peak_out = new DigitalPeakMeter(PluginWidget);
        peak_out->setObjectName(QString::fromUtf8("peak_out"));
        peak_out->setMinimumSize(QSize(150, 0));
        peak_out->setMaximumSize(QSize(150, 16777215));

        layout_peaks->addWidget(peak_out);


        layout_bottom->addLayout(layout_peaks);


        verticalLayout->addLayout(layout_bottom);


        retranslateUi(PluginWidget);

        QMetaObject::connectSlotsByName(PluginWidget);
    } // setupUi

    void retranslateUi(QFrame *PluginWidget)
    {
        PluginWidget->setWindowTitle(QCoreApplication::translate("PluginWidget", "Frame", nullptr));
        b_enable->setText(QString());
        b_gui->setText(QString());
        b_edit->setText(QString());
        label_name->setText(QCoreApplication::translate("PluginWidget", "PluginName", nullptr));
        label_presets->setText(QCoreApplication::translate("PluginWidget", "Preset:", nullptr));
        led_control->setText(QString());
        led_midi->setText(QString());
        led_audio_in->setText(QString());
        led_audio_out->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class PluginWidget: public Ui_PluginWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_PLUGIN_PRESETS_H
