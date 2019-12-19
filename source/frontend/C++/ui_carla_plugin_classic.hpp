/********************************************************************************
** Form generated from reading UI file 'carla_plugin_classic.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_PLUGIN_CLASSIC_H
#define UI_CARLA_PLUGIN_CLASSIC_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
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
    QHBoxLayout *horizontalLayout;
    QWidget *area_left;
    QHBoxLayout *horizontalLayout_3;
    PixmapButton *b_enable;
    PixmapButton *b_gui;
    PixmapButton *b_edit;
    QLabel *label_name;
    QSpacerItem *horizontalSpacer;
    QWidget *area_right;
    QHBoxLayout *horizontalLayout_2;
    LEDButton *led_control;
    LEDButton *led_midi;
    LEDButton *led_audio_in;
    LEDButton *led_audio_out;
    QVBoxLayout *verticalLayout_2;
    DigitalPeakMeter *peak_in;
    DigitalPeakMeter *peak_out;

    void setupUi(QFrame *PluginWidget)
    {
        if (PluginWidget->objectName().isEmpty())
            PluginWidget->setObjectName(QString::fromUtf8("PluginWidget"));
        PluginWidget->resize(497, 37);
        PluginWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        PluginWidget->setLineWidth(0);
        horizontalLayout = new QHBoxLayout(PluginWidget);
        horizontalLayout->setSpacing(2);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(4, 2, 4, 2);
        area_left = new QWidget(PluginWidget);
        area_left->setObjectName(QString::fromUtf8("area_left"));
        horizontalLayout_3 = new QHBoxLayout(area_left);
        horizontalLayout_3->setSpacing(1);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 1, 0, 4);
        b_enable = new PixmapButton(area_left);
        b_enable->setObjectName(QString::fromUtf8("b_enable"));
        b_enable->setMinimumSize(QSize(24, 24));
        b_enable->setMaximumSize(QSize(24, 24));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/bitmaps/button_off.png"), QSize(), QIcon::Normal, QIcon::Off);
        b_enable->setIcon(icon);
        b_enable->setIconSize(QSize(24, 24));
        b_enable->setCheckable(true);
        b_enable->setFlat(true);

        horizontalLayout_3->addWidget(b_enable);

        b_gui = new PixmapButton(area_left);
        b_gui->setObjectName(QString::fromUtf8("b_gui"));
        b_gui->setMinimumSize(QSize(24, 24));
        b_gui->setMaximumSize(QSize(24, 24));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/bitmaps/button_gui.png"), QSize(), QIcon::Normal, QIcon::Off);
        b_gui->setIcon(icon1);
        b_gui->setIconSize(QSize(24, 24));
        b_gui->setCheckable(true);
        b_gui->setFlat(true);

        horizontalLayout_3->addWidget(b_gui);

        b_edit = new PixmapButton(area_left);
        b_edit->setObjectName(QString::fromUtf8("b_edit"));
        b_edit->setMinimumSize(QSize(24, 24));
        b_edit->setMaximumSize(QSize(24, 24));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/bitmaps/button_edit.png"), QSize(), QIcon::Normal, QIcon::Off);
        b_edit->setIcon(icon2);
        b_edit->setIconSize(QSize(24, 24));
        b_edit->setCheckable(true);
        b_edit->setFlat(true);

        horizontalLayout_3->addWidget(b_edit);

        label_name = new QLabel(area_left);
        label_name->setObjectName(QString::fromUtf8("label_name"));

        horizontalLayout_3->addWidget(label_name);


        horizontalLayout->addWidget(area_left);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        area_right = new QWidget(PluginWidget);
        area_right->setObjectName(QString::fromUtf8("area_right"));
        horizontalLayout_2 = new QHBoxLayout(area_right);
        horizontalLayout_2->setSpacing(1);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(4, 4, 0, 1);
        led_control = new LEDButton(area_right);
        led_control->setObjectName(QString::fromUtf8("led_control"));
        led_control->setMinimumSize(QSize(14, 14));
        led_control->setMaximumSize(QSize(14, 14));
        led_control->setCheckable(true);

        horizontalLayout_2->addWidget(led_control);

        led_midi = new LEDButton(area_right);
        led_midi->setObjectName(QString::fromUtf8("led_midi"));
        led_midi->setMinimumSize(QSize(14, 14));
        led_midi->setMaximumSize(QSize(14, 14));
        led_midi->setCheckable(true);

        horizontalLayout_2->addWidget(led_midi);

        led_audio_in = new LEDButton(area_right);
        led_audio_in->setObjectName(QString::fromUtf8("led_audio_in"));
        led_audio_in->setMinimumSize(QSize(14, 14));
        led_audio_in->setMaximumSize(QSize(14, 14));
        led_audio_in->setCheckable(true);

        horizontalLayout_2->addWidget(led_audio_in);

        led_audio_out = new LEDButton(area_right);
        led_audio_out->setObjectName(QString::fromUtf8("led_audio_out"));
        led_audio_out->setMinimumSize(QSize(14, 14));
        led_audio_out->setMaximumSize(QSize(14, 14));
        led_audio_out->setCheckable(true);

        horizontalLayout_2->addWidget(led_audio_out);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(6, -1, -1, -1);
        peak_in = new DigitalPeakMeter(area_right);
        peak_in->setObjectName(QString::fromUtf8("peak_in"));
        peak_in->setMinimumSize(QSize(150, 0));
        peak_in->setMaximumSize(QSize(150, 16777215));

        verticalLayout_2->addWidget(peak_in);

        peak_out = new DigitalPeakMeter(area_right);
        peak_out->setObjectName(QString::fromUtf8("peak_out"));
        peak_out->setMinimumSize(QSize(150, 0));
        peak_out->setMaximumSize(QSize(150, 16777215));

        verticalLayout_2->addWidget(peak_out);


        horizontalLayout_2->addLayout(verticalLayout_2);


        horizontalLayout->addWidget(area_right);


        retranslateUi(PluginWidget);

        QMetaObject::connectSlotsByName(PluginWidget);
    } // setupUi

    void retranslateUi(QFrame *PluginWidget)
    {
        PluginWidget->setWindowTitle(QCoreApplication::translate("PluginWidget", "Frame", nullptr));
        b_enable->setText(QString());
        b_gui->setText(QString());
        b_edit->setText(QString());
        label_name->setText(QCoreApplication::translate("PluginWidget", "Plugin Name", nullptr));
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

#endif // UI_CARLA_PLUGIN_CLASSIC_H
