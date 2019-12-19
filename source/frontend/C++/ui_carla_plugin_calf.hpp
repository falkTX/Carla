/********************************************************************************
** Form generated from reading UI file 'carla_plugin_calf.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_PLUGIN_CALF_H
#define UI_CARLA_PLUGIN_CALF_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
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
    QHBoxLayout *horizontalLayout_5;
    PixmapButton *b_enable;
    QLabel *label_active;
    QSpacerItem *spacer_namesep1;
    QFrame *line_sep;
    QSpacerItem *spacer_namesep2;
    QLabel *label_name;
    QSpacerItem *spacer_namesep3;
    QVBoxLayout *layout_midi;
    QLabel *label_midi;
    LEDButton *led_midi;
    QVBoxLayout *layout_peak_in;
    QLabel *label_audio_in;
    DigitalPeakMeter *peak_in;
    QVBoxLayout *layout_peak_out;
    QLabel *label_audio_out;
    DigitalPeakMeter *peak_out;
    QFrame *line;
    QHBoxLayout *layout_bottom;
    QHBoxLayout *w_buttons2;
    PixmapButton *b_gui;
    PixmapButton *b_edit;
    PixmapButton *b_remove;
    QSpacerItem *spacer_knobs;
    QWidget *w_knobs;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_logo;

    void setupUi(QFrame *PluginWidget)
    {
        if (PluginWidget->objectName().isEmpty())
            PluginWidget->setObjectName(QString::fromUtf8("PluginWidget"));
        PluginWidget->resize(622, 90);
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
        w_buttons->setMinimumSize(QSize(72, 0));
        w_buttons->setMaximumSize(QSize(72, 16777215));
        horizontalLayout_5 = new QHBoxLayout(w_buttons);
        horizontalLayout_5->setSpacing(2);
        horizontalLayout_5->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        b_enable = new PixmapButton(w_buttons);
        b_enable->setObjectName(QString::fromUtf8("b_enable"));
        b_enable->setCheckable(true);

        horizontalLayout_5->addWidget(b_enable);

        label_active = new QLabel(w_buttons);
        label_active->setObjectName(QString::fromUtf8("label_active"));

        horizontalLayout_5->addWidget(label_active);


        layout_top->addWidget(w_buttons);

        spacer_namesep1 = new QSpacerItem(6, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        layout_top->addItem(spacer_namesep1);

        line_sep = new QFrame(PluginWidget);
        line_sep->setObjectName(QString::fromUtf8("line_sep"));
        line_sep->setMinimumSize(QSize(0, 20));
        line_sep->setMaximumSize(QSize(16777215, 20));
        line_sep->setLineWidth(0);
        line_sep->setMidLineWidth(1);
        line_sep->setFrameShape(QFrame::VLine);
        line_sep->setFrameShadow(QFrame::Sunken);

        layout_top->addWidget(line_sep);

        spacer_namesep2 = new QSpacerItem(8, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        layout_top->addItem(spacer_namesep2);

        label_name = new QLabel(PluginWidget);
        label_name->setObjectName(QString::fromUtf8("label_name"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        label_name->setFont(font);
        label_name->setAlignment(Qt::AlignCenter);

        layout_top->addWidget(label_name);

        spacer_namesep3 = new QSpacerItem(20, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        layout_top->addItem(spacer_namesep3);

        layout_midi = new QVBoxLayout();
        layout_midi->setSpacing(0);
        layout_midi->setObjectName(QString::fromUtf8("layout_midi"));
        layout_midi->setContentsMargins(-1, -1, 4, -1);
        label_midi = new QLabel(PluginWidget);
        label_midi->setObjectName(QString::fromUtf8("label_midi"));
        label_midi->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        layout_midi->addWidget(label_midi);

        led_midi = new LEDButton(PluginWidget);
        led_midi->setObjectName(QString::fromUtf8("led_midi"));
        led_midi->setMinimumSize(QSize(25, 25));
        led_midi->setMaximumSize(QSize(25, 25));
        led_midi->setIconSize(QSize(25, 25));
        led_midi->setCheckable(true);

        layout_midi->addWidget(led_midi);


        layout_top->addLayout(layout_midi);

        layout_peak_in = new QVBoxLayout();
        layout_peak_in->setSpacing(0);
        layout_peak_in->setObjectName(QString::fromUtf8("layout_peak_in"));
        label_audio_in = new QLabel(PluginWidget);
        label_audio_in->setObjectName(QString::fromUtf8("label_audio_in"));
        label_audio_in->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        layout_peak_in->addWidget(label_audio_in);

        peak_in = new DigitalPeakMeter(PluginWidget);
        peak_in->setObjectName(QString::fromUtf8("peak_in"));
        peak_in->setMinimumSize(QSize(150, 0));

        layout_peak_in->addWidget(peak_in);


        layout_top->addLayout(layout_peak_in);

        layout_peak_out = new QVBoxLayout();
        layout_peak_out->setSpacing(0);
        layout_peak_out->setObjectName(QString::fromUtf8("layout_peak_out"));
        label_audio_out = new QLabel(PluginWidget);
        label_audio_out->setObjectName(QString::fromUtf8("label_audio_out"));
        label_audio_out->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        layout_peak_out->addWidget(label_audio_out);

        peak_out = new DigitalPeakMeter(PluginWidget);
        peak_out->setObjectName(QString::fromUtf8("peak_out"));
        peak_out->setMinimumSize(QSize(150, 0));

        layout_peak_out->addWidget(peak_out);


        layout_top->addLayout(layout_peak_out);


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
        layout_bottom->setContentsMargins(6, 4, 4, -1);
        w_buttons2 = new QHBoxLayout();
        w_buttons2->setSpacing(3);
        w_buttons2->setObjectName(QString::fromUtf8("w_buttons2"));
        b_gui = new PixmapButton(PluginWidget);
        b_gui->setObjectName(QString::fromUtf8("b_gui"));
        b_gui->setCheckable(true);

        w_buttons2->addWidget(b_gui);

        b_edit = new PixmapButton(PluginWidget);
        b_edit->setObjectName(QString::fromUtf8("b_edit"));
        b_edit->setCheckable(true);

        w_buttons2->addWidget(b_edit);

        b_remove = new PixmapButton(PluginWidget);
        b_remove->setObjectName(QString::fromUtf8("b_remove"));

        w_buttons2->addWidget(b_remove);


        layout_bottom->addLayout(w_buttons2);

        spacer_knobs = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        layout_bottom->addItem(spacer_knobs);

        w_knobs = new QWidget(PluginWidget);
        w_knobs->setObjectName(QString::fromUtf8("w_knobs"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(w_knobs->sizePolicy().hasHeightForWidth());
        w_knobs->setSizePolicy(sizePolicy);
        horizontalLayout_4 = new QHBoxLayout(w_knobs);
        horizontalLayout_4->setSpacing(2);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(4, 0, 4, 0);

        layout_bottom->addWidget(w_knobs);

        label_logo = new QLabel(PluginWidget);
        label_logo->setObjectName(QString::fromUtf8("label_logo"));
        label_logo->setMinimumSize(QSize(71, 30));
        label_logo->setMaximumSize(QSize(71, 30));
        label_logo->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/logo_calf.png")));
        label_logo->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        layout_bottom->addWidget(label_logo);


        verticalLayout->addLayout(layout_bottom);


        retranslateUi(PluginWidget);

        QMetaObject::connectSlotsByName(PluginWidget);
    } // setupUi

    void retranslateUi(QFrame *PluginWidget)
    {
        PluginWidget->setWindowTitle(QCoreApplication::translate("PluginWidget", "Frame", nullptr));
        b_enable->setText(QCoreApplication::translate("PluginWidget", "Enable", nullptr));
        label_active->setText(QCoreApplication::translate("PluginWidget", "On/Off", nullptr));
        label_name->setText(QCoreApplication::translate("PluginWidget", "PluginName", nullptr));
        label_midi->setText(QCoreApplication::translate("PluginWidget", "MIDI", nullptr));
        led_midi->setText(QString());
        label_audio_in->setText(QCoreApplication::translate("PluginWidget", "AUDIO IN", nullptr));
        label_audio_out->setText(QCoreApplication::translate("PluginWidget", "AUDIO OUT", nullptr));
        b_gui->setText(QCoreApplication::translate("PluginWidget", "GUI", nullptr));
        b_edit->setText(QCoreApplication::translate("PluginWidget", "Edit", nullptr));
        b_remove->setText(QCoreApplication::translate("PluginWidget", "Remove", nullptr));
        label_logo->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class PluginWidget: public Ui_PluginWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_PLUGIN_CALF_H
