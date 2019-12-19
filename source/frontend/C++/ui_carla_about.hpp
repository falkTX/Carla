/********************************************************************************
** Form generated from reading UI file 'carla_about.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_ABOUT_H
#define UI_CARLA_ABOUT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CarlaAboutW
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *about;
    QGridLayout *gridLayout_3;
    QLabel *l_about;
    QSpacerItem *verticalSpacer;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *horizontalSpacer_2;
    QLabel *l_icons;
    QSpacerItem *verticalSpacer_4;
    QLabel *l_extended;
    QWidget *artwork;
    QGridLayout *gridLayout_4;
    QFrame *line_2;
    QLabel *label_11;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_15;
    QLabel *label_18;
    QLabel *label_19;
    QLabel *label_16;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_23;
    QLabel *label_20;
    QLabel *ico_example_gui;
    QLabel *ico_example_edit;
    QLabel *ico_example_file;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_8;
    QLabel *label_9;
    QLabel *label_10;
    QLabel *label_13;
    QFrame *line;
    QLabel *label_25;
    QSpacerItem *verticalSpacer_6;
    QHBoxLayout *horizontalLayout;
    QLabel *label_3;
    QLabel *label_5;
    QLabel *label_6;
    QLabel *label_14;
    QLabel *label_24;
    QLabel *label_12;
    QFrame *line_4;
    QFrame *line_3;
    QLabel *label_4;
    QFrame *line_5;
    QWidget *features;
    QGridLayout *gridLayout;
    QLabel *lid_au;
    QFrame *line_vst2;
    QLabel *lid_ladspa;
    QLabel *l_vst2;
    QSpacerItem *verticalSpacer_3;
    QLabel *lid_vst2;
    QLabel *lid_dssi;
    QLabel *l_dssi;
    QLabel *lid_lv2;
    QLabel *l_lv2;
    QLabel *l_ladspa;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *horizontalSpacer_4;
    QFrame *line_ladspa;
    QFrame *line_dssi;
    QFrame *line_lv2;
    QFrame *line_vst3;
    QLabel *lid_vst3;
    QLabel *l_vst3;
    QLabel *l_au;
    QWidget *osc;
    QGridLayout *gridLayout_2;
    QLabel *label_2;
    QLineEdit *le_osc_url_tcp;
    QLabel *label_7;
    QLabel *l_osc_cmds;
    QLabel *label;
    QLabel *l_example;
    QLabel *l_example_help;
    QSpacerItem *verticalSpacer_2;
    QLineEdit *le_osc_url_udp;
    QSpacerItem *verticalSpacer_5;
    QWidget *tab_license;
    QVBoxLayout *verticalLayout_2;
    QPlainTextEdit *plainTextEdit;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CarlaAboutW)
    {
        if (CarlaAboutW->objectName().isEmpty())
            CarlaAboutW->setObjectName(QString::fromUtf8("CarlaAboutW"));
        CarlaAboutW->resize(512, 519);
        verticalLayout = new QVBoxLayout(CarlaAboutW);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        tabWidget = new QTabWidget(CarlaAboutW);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        about = new QWidget();
        about->setObjectName(QString::fromUtf8("about"));
        gridLayout_3 = new QGridLayout(about);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        l_about = new QLabel(about);
        l_about->setObjectName(QString::fromUtf8("l_about"));

        gridLayout_3->addWidget(l_about, 2, 1, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_3->addItem(verticalSpacer, 7, 0, 1, 2);

        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_3->addItem(horizontalSpacer, 2, 0, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_2, 2, 2, 1, 1);

        l_icons = new QLabel(about);
        l_icons->setObjectName(QString::fromUtf8("l_icons"));
        l_icons->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/carla_about_white.png")));
        l_icons->setAlignment(Qt::AlignCenter);

        gridLayout_3->addWidget(l_icons, 1, 0, 1, 3);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_3->addItem(verticalSpacer_4, 0, 0, 1, 3);

        l_extended = new QLabel(about);
        l_extended->setObjectName(QString::fromUtf8("l_extended"));
        l_extended->setWordWrap(true);

        gridLayout_3->addWidget(l_extended, 3, 0, 1, 3);

        tabWidget->addTab(about, QString());
        artwork = new QWidget();
        artwork->setObjectName(QString::fromUtf8("artwork"));
        gridLayout_4 = new QGridLayout(artwork);
        gridLayout_4->setContentsMargins(12, 12, 12, 12);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        gridLayout_4->setHorizontalSpacing(16);
        line_2 = new QFrame(artwork);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setLineWidth(0);
        line_2->setMidLineWidth(1);
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);

        gridLayout_4->addWidget(line_2, 4, 0, 1, 3);

        label_11 = new QLabel(artwork);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_11->sizePolicy().hasHeightForWidth());
        label_11->setSizePolicy(sizePolicy);
        label_11->setWordWrap(true);

        gridLayout_4->addWidget(label_11, 2, 1, 2, 2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        label_15 = new QLabel(artwork);
        label_15->setObjectName(QString::fromUtf8("label_15"));
        label_15->setMinimumSize(QSize(71, 30));
        label_15->setMaximumSize(QSize(71, 30));
        label_15->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/logo_calf.png")));

        horizontalLayout_3->addWidget(label_15);

        label_18 = new QLabel(artwork);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        label_18->setMinimumSize(QSize(40, 40));
        label_18->setMaximumSize(QSize(40, 40));
        label_18->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/dial_09s.png")));

        horizontalLayout_3->addWidget(label_18);

        label_19 = new QLabel(artwork);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        label_19->setMinimumSize(QSize(32, 32));
        label_19->setMaximumSize(QSize(32, 32));
        label_19->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/dial_11e.png")));

        horizontalLayout_3->addWidget(label_19);


        gridLayout_4->addLayout(horizontalLayout_3, 8, 0, 1, 1);

        label_16 = new QLabel(artwork);
        label_16->setObjectName(QString::fromUtf8("label_16"));
        sizePolicy.setHeightForWidth(label_16->sizePolicy().hasHeightForWidth());
        label_16->setSizePolicy(sizePolicy);
        label_16->setWordWrap(true);

        gridLayout_4->addWidget(label_16, 8, 1, 1, 2);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        label_23 = new QLabel(artwork);
        label_23->setObjectName(QString::fromUtf8("label_23"));
        label_23->setMinimumSize(QSize(34, 34));
        label_23->setMaximumSize(QSize(34, 34));
        label_23->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/dial_03.png")));

        horizontalLayout_4->addWidget(label_23);

        label_20 = new QLabel(artwork);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        label_20->setMinimumSize(QSize(22, 22));
        label_20->setMaximumSize(QSize(22, 22));
        label_20->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/button_on.png")));

        horizontalLayout_4->addWidget(label_20);

        ico_example_gui = new QLabel(artwork);
        ico_example_gui->setObjectName(QString::fromUtf8("ico_example_gui"));
        ico_example_gui->setMinimumSize(QSize(22, 22));
        ico_example_gui->setMaximumSize(QSize(22, 22));
        ico_example_gui->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/button_gui_down-white.png")));

        horizontalLayout_4->addWidget(ico_example_gui);

        ico_example_edit = new QLabel(artwork);
        ico_example_edit->setObjectName(QString::fromUtf8("ico_example_edit"));
        ico_example_edit->setMinimumSize(QSize(22, 22));
        ico_example_edit->setMaximumSize(QSize(22, 22));
        ico_example_edit->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/button_edit_down-white.png")));

        horizontalLayout_4->addWidget(ico_example_edit);

        ico_example_file = new QLabel(artwork);
        ico_example_file->setObjectName(QString::fromUtf8("ico_example_file"));
        ico_example_file->setMinimumSize(QSize(22, 22));
        ico_example_file->setMaximumSize(QSize(22, 22));
        ico_example_file->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/button_file_down-white.png")));

        horizontalLayout_4->addWidget(ico_example_file);


        gridLayout_4->addLayout(horizontalLayout_4, 10, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label_8 = new QLabel(artwork);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setMinimumSize(QSize(48, 48));
        label_8->setMaximumSize(QSize(48, 48));
        label_8->setTextFormat(Qt::PlainText);
        label_8->setPixmap(QPixmap(QString::fromUtf8(":/scalable/folder.svgz")));
        label_8->setScaledContents(true);

        horizontalLayout_2->addWidget(label_8);

        label_9 = new QLabel(artwork);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setMinimumSize(QSize(48, 48));
        label_9->setMaximumSize(QSize(48, 48));
        label_9->setTextFormat(Qt::PlainText);
        label_9->setPixmap(QPixmap(QString::fromUtf8(":/scalable/warning.svgz")));
        label_9->setScaledContents(true);

        horizontalLayout_2->addWidget(label_9);

        label_10 = new QLabel(artwork);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setMinimumSize(QSize(48, 48));
        label_10->setMaximumSize(QSize(48, 48));
        label_10->setTextFormat(Qt::PlainText);
        label_10->setPixmap(QPixmap(QString::fromUtf8(":/scalable/wine.svgz")));
        label_10->setScaledContents(true);

        horizontalLayout_2->addWidget(label_10);


        gridLayout_4->addLayout(horizontalLayout_2, 2, 0, 2, 1);

        label_13 = new QLabel(artwork);
        label_13->setObjectName(QString::fromUtf8("label_13"));
        label_13->setPixmap(QPixmap(QString::fromUtf8(":/bitmaps/kbd_normal.png")));

        gridLayout_4->addWidget(label_13, 5, 0, 2, 1);

        line = new QFrame(artwork);
        line->setObjectName(QString::fromUtf8("line"));
        line->setLineWidth(0);
        line->setMidLineWidth(1);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        gridLayout_4->addWidget(line, 1, 0, 1, 3);

        label_25 = new QLabel(artwork);
        label_25->setObjectName(QString::fromUtf8("label_25"));
        label_25->setWordWrap(true);

        gridLayout_4->addWidget(label_25, 13, 0, 1, 3);

        verticalSpacer_6 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_4->addItem(verticalSpacer_6, 14, 0, 1, 3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label_3 = new QLabel(artwork);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setMinimumSize(QSize(48, 48));
        label_3->setMaximumSize(QSize(48, 48));
        label_3->setTextFormat(Qt::PlainText);
        label_3->setPixmap(QPixmap(QString::fromUtf8(":/scalable/carla.svg")));
        label_3->setScaledContents(true);

        horizontalLayout->addWidget(label_3);

        label_5 = new QLabel(artwork);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setMinimumSize(QSize(48, 48));
        label_5->setMaximumSize(QSize(48, 48));
        label_5->setTextFormat(Qt::PlainText);
        label_5->setPixmap(QPixmap(QString::fromUtf8(":/scalable/carla-control.svg")));
        label_5->setScaledContents(true);

        horizontalLayout->addWidget(label_5);

        label_6 = new QLabel(artwork);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(48);
        sizePolicy1.setVerticalStretch(48);
        sizePolicy1.setHeightForWidth(label_6->sizePolicy().hasHeightForWidth());
        label_6->setSizePolicy(sizePolicy1);
        label_6->setMaximumSize(QSize(48, 48));
        label_6->setTextFormat(Qt::PlainText);
        label_6->setPixmap(QPixmap(QString::fromUtf8(":/48x48/canvas.png")));

        horizontalLayout->addWidget(label_6);


        gridLayout_4->addLayout(horizontalLayout, 0, 0, 1, 1);

        label_14 = new QLabel(artwork);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        sizePolicy.setHeightForWidth(label_14->sizePolicy().hasHeightForWidth());
        label_14->setSizePolicy(sizePolicy);
        label_14->setWordWrap(true);

        gridLayout_4->addWidget(label_14, 10, 1, 1, 2);

        label_24 = new QLabel(artwork);
        label_24->setObjectName(QString::fromUtf8("label_24"));
        label_24->setWordWrap(true);

        gridLayout_4->addWidget(label_24, 12, 0, 1, 3);

        label_12 = new QLabel(artwork);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        sizePolicy.setHeightForWidth(label_12->sizePolicy().hasHeightForWidth());
        label_12->setSizePolicy(sizePolicy);
        label_12->setWordWrap(true);

        gridLayout_4->addWidget(label_12, 5, 1, 2, 2);

        line_4 = new QFrame(artwork);
        line_4->setObjectName(QString::fromUtf8("line_4"));
        line_4->setLineWidth(0);
        line_4->setMidLineWidth(1);
        line_4->setFrameShape(QFrame::HLine);
        line_4->setFrameShadow(QFrame::Sunken);

        gridLayout_4->addWidget(line_4, 9, 0, 1, 3);

        line_3 = new QFrame(artwork);
        line_3->setObjectName(QString::fromUtf8("line_3"));
        line_3->setLineWidth(0);
        line_3->setMidLineWidth(1);
        line_3->setFrameShape(QFrame::HLine);
        line_3->setFrameShadow(QFrame::Sunken);

        gridLayout_4->addWidget(line_3, 7, 0, 1, 3);

        label_4 = new QLabel(artwork);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        sizePolicy.setHeightForWidth(label_4->sizePolicy().hasHeightForWidth());
        label_4->setSizePolicy(sizePolicy);
        label_4->setWordWrap(true);

        gridLayout_4->addWidget(label_4, 0, 1, 1, 2);

        line_5 = new QFrame(artwork);
        line_5->setObjectName(QString::fromUtf8("line_5"));
        line_5->setLineWidth(0);
        line_5->setMidLineWidth(1);
        line_5->setFrameShape(QFrame::HLine);
        line_5->setFrameShadow(QFrame::Sunken);

        gridLayout_4->addWidget(line_5, 11, 0, 1, 3);

        tabWidget->addTab(artwork, QString());
        features = new QWidget();
        features->setObjectName(QString::fromUtf8("features"));
        gridLayout = new QGridLayout(features);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lid_au = new QLabel(features);
        lid_au->setObjectName(QString::fromUtf8("lid_au"));
        lid_au->setTextFormat(Qt::PlainText);
        lid_au->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout->addWidget(lid_au, 10, 1, 1, 1);

        line_vst2 = new QFrame(features);
        line_vst2->setObjectName(QString::fromUtf8("line_vst2"));
        line_vst2->setFrameShape(QFrame::HLine);
        line_vst2->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(line_vst2, 7, 0, 1, 4);

        lid_ladspa = new QLabel(features);
        lid_ladspa->setObjectName(QString::fromUtf8("lid_ladspa"));
        lid_ladspa->setTextFormat(Qt::PlainText);
        lid_ladspa->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout->addWidget(lid_ladspa, 0, 1, 1, 1);

        l_vst2 = new QLabel(features);
        l_vst2->setObjectName(QString::fromUtf8("l_vst2"));
        sizePolicy.setHeightForWidth(l_vst2->sizePolicy().hasHeightForWidth());
        l_vst2->setSizePolicy(sizePolicy);
        l_vst2->setTextFormat(Qt::PlainText);
        l_vst2->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout->addWidget(l_vst2, 6, 2, 1, 1);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer_3, 11, 1, 1, 2);

        lid_vst2 = new QLabel(features);
        lid_vst2->setObjectName(QString::fromUtf8("lid_vst2"));
        lid_vst2->setTextFormat(Qt::PlainText);
        lid_vst2->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout->addWidget(lid_vst2, 6, 1, 1, 1);

        lid_dssi = new QLabel(features);
        lid_dssi->setObjectName(QString::fromUtf8("lid_dssi"));
        lid_dssi->setTextFormat(Qt::PlainText);
        lid_dssi->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout->addWidget(lid_dssi, 2, 1, 1, 1);

        l_dssi = new QLabel(features);
        l_dssi->setObjectName(QString::fromUtf8("l_dssi"));
        sizePolicy.setHeightForWidth(l_dssi->sizePolicy().hasHeightForWidth());
        l_dssi->setSizePolicy(sizePolicy);
        l_dssi->setTextFormat(Qt::PlainText);
        l_dssi->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout->addWidget(l_dssi, 2, 2, 1, 1);

        lid_lv2 = new QLabel(features);
        lid_lv2->setObjectName(QString::fromUtf8("lid_lv2"));
        lid_lv2->setTextFormat(Qt::PlainText);
        lid_lv2->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout->addWidget(lid_lv2, 4, 1, 1, 1);

        l_lv2 = new QLabel(features);
        l_lv2->setObjectName(QString::fromUtf8("l_lv2"));
        sizePolicy.setHeightForWidth(l_lv2->sizePolicy().hasHeightForWidth());
        l_lv2->setSizePolicy(sizePolicy);
        l_lv2->setTextFormat(Qt::RichText);
        l_lv2->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout->addWidget(l_lv2, 4, 2, 1, 1);

        l_ladspa = new QLabel(features);
        l_ladspa->setObjectName(QString::fromUtf8("l_ladspa"));
        sizePolicy.setHeightForWidth(l_ladspa->sizePolicy().hasHeightForWidth());
        l_ladspa->setSizePolicy(sizePolicy);
        l_ladspa->setTextFormat(Qt::PlainText);
        l_ladspa->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout->addWidget(l_ladspa, 0, 2, 1, 1);

        horizontalSpacer_3 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_3, 0, 0, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_4, 0, 3, 1, 1);

        line_ladspa = new QFrame(features);
        line_ladspa->setObjectName(QString::fromUtf8("line_ladspa"));
        line_ladspa->setLineWidth(0);
        line_ladspa->setMidLineWidth(1);
        line_ladspa->setFrameShape(QFrame::HLine);
        line_ladspa->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(line_ladspa, 1, 0, 1, 4);

        line_dssi = new QFrame(features);
        line_dssi->setObjectName(QString::fromUtf8("line_dssi"));
        line_dssi->setLineWidth(0);
        line_dssi->setMidLineWidth(1);
        line_dssi->setFrameShape(QFrame::HLine);
        line_dssi->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(line_dssi, 3, 0, 1, 4);

        line_lv2 = new QFrame(features);
        line_lv2->setObjectName(QString::fromUtf8("line_lv2"));
        line_lv2->setLineWidth(0);
        line_lv2->setMidLineWidth(1);
        line_lv2->setFrameShape(QFrame::HLine);
        line_lv2->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(line_lv2, 5, 0, 1, 4);

        line_vst3 = new QFrame(features);
        line_vst3->setObjectName(QString::fromUtf8("line_vst3"));
        line_vst3->setFrameShape(QFrame::HLine);
        line_vst3->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(line_vst3, 9, 0, 1, 4);

        lid_vst3 = new QLabel(features);
        lid_vst3->setObjectName(QString::fromUtf8("lid_vst3"));
        lid_vst3->setTextFormat(Qt::PlainText);
        lid_vst3->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout->addWidget(lid_vst3, 8, 1, 1, 1);

        l_vst3 = new QLabel(features);
        l_vst3->setObjectName(QString::fromUtf8("l_vst3"));
        l_vst3->setTextFormat(Qt::PlainText);
        l_vst3->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout->addWidget(l_vst3, 8, 2, 1, 1);

        l_au = new QLabel(features);
        l_au->setObjectName(QString::fromUtf8("l_au"));
        l_au->setTextFormat(Qt::PlainText);
        l_au->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout->addWidget(l_au, 10, 2, 1, 1);

        tabWidget->addTab(features, QString());
        osc = new QWidget();
        osc->setObjectName(QString::fromUtf8("osc"));
        gridLayout_2 = new QGridLayout(osc);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        label_2 = new QLabel(osc);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_2, 0, 0, 1, 1);

        le_osc_url_tcp = new QLineEdit(osc);
        le_osc_url_tcp->setObjectName(QString::fromUtf8("le_osc_url_tcp"));
        le_osc_url_tcp->setFrame(false);
        le_osc_url_tcp->setReadOnly(true);

        gridLayout_2->addWidget(le_osc_url_tcp, 0, 1, 1, 1);

        label_7 = new QLabel(osc);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout_2->addWidget(label_7, 3, 0, 1, 1);

        l_osc_cmds = new QLabel(osc);
        l_osc_cmds->setObjectName(QString::fromUtf8("l_osc_cmds"));
        l_osc_cmds->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        l_osc_cmds->setTextInteractionFlags(Qt::TextSelectableByMouse);

        gridLayout_2->addWidget(l_osc_cmds, 3, 1, 1, 1);

        label = new QLabel(osc);
        label->setObjectName(QString::fromUtf8("label"));
        label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label, 4, 0, 1, 1);

        l_example = new QLabel(osc);
        l_example->setObjectName(QString::fromUtf8("l_example"));

        gridLayout_2->addWidget(l_example, 4, 1, 1, 1);

        l_example_help = new QLabel(osc);
        l_example_help->setObjectName(QString::fromUtf8("l_example_help"));
        l_example_help->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        gridLayout_2->addWidget(l_example_help, 5, 1, 1, 1);

        verticalSpacer_2 = new QSpacerItem(456, 35, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer_2, 6, 0, 1, 2);

        le_osc_url_udp = new QLineEdit(osc);
        le_osc_url_udp->setObjectName(QString::fromUtf8("le_osc_url_udp"));
        le_osc_url_udp->setFrame(false);
        le_osc_url_udp->setReadOnly(true);

        gridLayout_2->addWidget(le_osc_url_udp, 1, 1, 1, 1);

        verticalSpacer_5 = new QSpacerItem(20, 5, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout_2->addItem(verticalSpacer_5, 2, 0, 1, 2);

        tabWidget->addTab(osc, QString());
        tab_license = new QWidget();
        tab_license->setObjectName(QString::fromUtf8("tab_license"));
        verticalLayout_2 = new QVBoxLayout(tab_license);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        plainTextEdit = new QPlainTextEdit(tab_license);
        plainTextEdit->setObjectName(QString::fromUtf8("plainTextEdit"));
        plainTextEdit->setUndoRedoEnabled(false);
        plainTextEdit->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

        verticalLayout_2->addWidget(plainTextEdit);

        tabWidget->addTab(tab_license, QString());

        verticalLayout->addWidget(tabWidget);

        buttonBox = new QDialogButtonBox(CarlaAboutW);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(CarlaAboutW);
        QObject::connect(buttonBox, SIGNAL(accepted()), CarlaAboutW, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), CarlaAboutW, SLOT(reject()));

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(CarlaAboutW);
    } // setupUi

    void retranslateUi(QDialog *CarlaAboutW)
    {
        CarlaAboutW->setWindowTitle(QCoreApplication::translate("CarlaAboutW", "About Carla", nullptr));
        l_about->setText(QCoreApplication::translate("CarlaAboutW", "About text here", nullptr));
        l_icons->setText(QString());
        l_extended->setText(QCoreApplication::translate("CarlaAboutW", "Extended licensing here", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(about), QCoreApplication::translate("CarlaAboutW", "About", nullptr));
        label_11->setText(QCoreApplication::translate("CarlaAboutW", "Using KDE Oxygen icon set, designed by Oxygen Team.", nullptr));
        label_15->setText(QString());
        label_18->setText(QString());
        label_19->setText(QString());
        label_16->setText(QCoreApplication::translate("CarlaAboutW", "Contains some knobs, backgrounds and other small artwork from Calf Studio Gear, OpenAV and OpenOctave projects.", nullptr));
        label_23->setText(QString());
        label_20->setText(QString());
        ico_example_gui->setText(QString());
        ico_example_edit->setText(QString());
        ico_example_file->setText(QString());
        label_8->setText(QString());
        label_9->setText(QString());
        label_10->setText(QString());
        label_13->setText(QString());
        label_25->setText(QCoreApplication::translate("CarlaAboutW", "VST is a trademark of Steinberg Media Technologies GmbH.", nullptr));
        label_3->setText(QString());
        label_5->setText(QString());
        label_6->setText(QString());
        label_14->setText(QCoreApplication::translate("CarlaAboutW", "Special thanks to Ant\303\263nio Saraiva for a few extra icons and artwork!", nullptr));
        label_24->setText(QCoreApplication::translate("CarlaAboutW", "The LV2 logo has been designed by Thorsten Wilms, based on a concept from Peter Shorthose.", nullptr));
        label_12->setText(QCoreApplication::translate("CarlaAboutW", "MIDI Keyboard designed by Thorsten Wilms.", nullptr));
        label_4->setText(QCoreApplication::translate("CarlaAboutW", "Carla, Carla-Control and Patchbay icons designed by DoosC.", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(artwork), QCoreApplication::translate("CarlaAboutW", "Artwork", nullptr));
        lid_au->setText(QCoreApplication::translate("CarlaAboutW", "AU/AudioUnit:", nullptr));
        lid_ladspa->setText(QCoreApplication::translate("CarlaAboutW", "LADSPA:", nullptr));
        l_vst2->setText(QCoreApplication::translate("CarlaAboutW", "TextLabel", nullptr));
        lid_vst2->setText(QCoreApplication::translate("CarlaAboutW", "VST2:", nullptr));
        lid_dssi->setText(QCoreApplication::translate("CarlaAboutW", "DSSI:", nullptr));
        l_dssi->setText(QCoreApplication::translate("CarlaAboutW", "TextLabel", nullptr));
        lid_lv2->setText(QCoreApplication::translate("CarlaAboutW", "LV2:", nullptr));
        l_lv2->setText(QCoreApplication::translate("CarlaAboutW", "TextLabel", nullptr));
        l_ladspa->setText(QCoreApplication::translate("CarlaAboutW", "TextLabel", nullptr));
        lid_vst3->setText(QCoreApplication::translate("CarlaAboutW", "VST3:", nullptr));
        l_vst3->setText(QCoreApplication::translate("CarlaAboutW", "TextLabel", nullptr));
        l_au->setText(QCoreApplication::translate("CarlaAboutW", "TextLabel", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(features), QCoreApplication::translate("CarlaAboutW", "Features", nullptr));
        label_2->setText(QCoreApplication::translate("CarlaAboutW", "Host URLs:", nullptr));
        label_7->setText(QCoreApplication::translate("CarlaAboutW", "Valid commands:", nullptr));
        l_osc_cmds->setText(QCoreApplication::translate("CarlaAboutW", "valid osc commands here", nullptr));
        label->setText(QCoreApplication::translate("CarlaAboutW", "Example:", nullptr));
        l_example->setText(QCoreApplication::translate("CarlaAboutW", "TextLabel", nullptr));
        l_example_help->setText(QCoreApplication::translate("CarlaAboutW", "TextLabel", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(osc), QCoreApplication::translate("CarlaAboutW", "OSC", nullptr));
        plainTextEdit->setPlainText(QCoreApplication::translate("CarlaAboutW", "                    GNU GENERAL PUBLIC LICENSE\n"
"                       Version 2, June 1991\n"
"\n"
" Copyright (C) 1989, 1991 Free Software Foundation, Inc.,\n"
" 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n"
" Everyone is permitted to copy and distribute verbatim copies\n"
" of this license document, but changing it is not allowed.\n"
"\n"
"                            Preamble\n"
"\n"
"  The licenses for most software are designed to take away your\n"
"freedom to share and change it.  By contrast, the GNU General Public\n"
"License is intended to guarantee your freedom to share and change free\n"
"software--to make sure the software is free for all its users.  This\n"
"General Public License applies to most of the Free Software\n"
"Foundation's software and to any other program whose authors commit to\n"
"using it.  (Some other Free Software Foundation software is covered by\n"
"the GNU Lesser General Public License instead.)  You can apply it to\n"
"your programs, too.\n"
"\n"
"  When we "
                        "speak of free software, we are referring to freedom, not\n"
"price.  Our General Public Licenses are designed to make sure that you\n"
"have the freedom to distribute copies of free software (and charge for\n"
"this service if you wish), that you receive source code or can get it\n"
"if you want it, that you can change the software or use pieces of it\n"
"in new free programs; and that you know you can do these things.\n"
"\n"
"  To protect your rights, we need to make restrictions that forbid\n"
"anyone to deny you these rights or to ask you to surrender the rights.\n"
"These restrictions translate to certain responsibilities for you if you\n"
"distribute copies of the software, or if you modify it.\n"
"\n"
"  For example, if you distribute copies of such a program, whether\n"
"gratis or for a fee, you must give the recipients all the rights that\n"
"you have.  You must make sure that they, too, receive or can get the\n"
"source code.  And you must show them these terms so they know their\n"
"rights.\n"
"\n"
""
                        "  We protect your rights with two steps: (1) copyright the software, and\n"
"(2) offer you this license which gives you legal permission to copy,\n"
"distribute and/or modify the software.\n"
"\n"
"  Also, for each author's protection and ours, we want to make certain\n"
"that everyone understands that there is no warranty for this free\n"
"software.  If the software is modified by someone else and passed on, we\n"
"want its recipients to know that what they have is not the original, so\n"
"that any problems introduced by others will not reflect on the original\n"
"authors' reputations.\n"
"\n"
"  Finally, any free program is threatened constantly by software\n"
"patents.  We wish to avoid the danger that redistributors of a free\n"
"program will individually obtain patent licenses, in effect making the\n"
"program proprietary.  To prevent this, we have made it clear that any\n"
"patent must be licensed for everyone's free use or not licensed at all.\n"
"\n"
"  The precise terms and conditions for copying, dis"
                        "tribution and\n"
"modification follow.\n"
"\n"
"                    GNU GENERAL PUBLIC LICENSE\n"
"   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n"
"\n"
"  0. This License applies to any program or other work which contains\n"
"a notice placed by the copyright holder saying it may be distributed\n"
"under the terms of this General Public License.  The \"Program\", below,\n"
"refers to any such program or work, and a \"work based on the Program\"\n"
"means either the Program or any derivative work under copyright law:\n"
"that is to say, a work containing the Program or a portion of it,\n"
"either verbatim or with modifications and/or translated into another\n"
"language.  (Hereinafter, translation is included without limitation in\n"
"the term \"modification\".)  Each licensee is addressed as \"you\".\n"
"\n"
"Activities other than copying, distribution and modification are not\n"
"covered by this License; they are outside its scope.  The act of\n"
"running the Program is not restricted, a"
                        "nd the output from the Program\n"
"is covered only if its contents constitute a work based on the\n"
"Program (independent of having been made by running the Program).\n"
"Whether that is true depends on what the Program does.\n"
"\n"
"  1. You may copy and distribute verbatim copies of the Program's\n"
"source code as you receive it, in any medium, provided that you\n"
"conspicuously and appropriately publish on each copy an appropriate\n"
"copyright notice and disclaimer of warranty; keep intact all the\n"
"notices that refer to this License and to the absence of any warranty;\n"
"and give any other recipients of the Program a copy of this License\n"
"along with the Program.\n"
"\n"
"You may charge a fee for the physical act of transferring a copy, and\n"
"you may at your option offer warranty protection in exchange for a fee.\n"
"\n"
"  2. You may modify your copy or copies of the Program or any portion\n"
"of it, thus forming a work based on the Program, and copy and\n"
"distribute such modifications or wo"
                        "rk under the terms of Section 1\n"
"above, provided that you also meet all of these conditions:\n"
"\n"
"    a) You must cause the modified files to carry prominent notices\n"
"    stating that you changed the files and the date of any change.\n"
"\n"
"    b) You must cause any work that you distribute or publish, that in\n"
"    whole or in part contains or is derived from the Program or any\n"
"    part thereof, to be licensed as a whole at no charge to all third\n"
"    parties under the terms of this License.\n"
"\n"
"    c) If the modified program normally reads commands interactively\n"
"    when run, you must cause it, when started running for such\n"
"    interactive use in the most ordinary way, to print or display an\n"
"    announcement including an appropriate copyright notice and a\n"
"    notice that there is no warranty (or else, saying that you provide\n"
"    a warranty) and that users may redistribute the program under\n"
"    these conditions, and telling the user how to view a copy of this\n"
""
                        "    License.  (Exception: if the Program itself is interactive but\n"
"    does not normally print such an announcement, your work based on\n"
"    the Program is not required to print an announcement.)\n"
"\n"
"These requirements apply to the modified work as a whole.  If\n"
"identifiable sections of that work are not derived from the Program,\n"
"and can be reasonably considered independent and separate works in\n"
"themselves, then this License, and its terms, do not apply to those\n"
"sections when you distribute them as separate works.  But when you\n"
"distribute the same sections as part of a whole which is a work based\n"
"on the Program, the distribution of the whole must be on the terms of\n"
"this License, whose permissions for other licensees extend to the\n"
"entire whole, and thus to each and every part regardless of who wrote it.\n"
"\n"
"Thus, it is not the intent of this section to claim rights or contest\n"
"your rights to work written entirely by you; rather, the intent is to\n"
"exercise th"
                        "e right to control the distribution of derivative or\n"
"collective works based on the Program.\n"
"\n"
"In addition, mere aggregation of another work not based on the Program\n"
"with the Program (or with a work based on the Program) on a volume of\n"
"a storage or distribution medium does not bring the other work under\n"
"the scope of this License.\n"
"\n"
"  3. You may copy and distribute the Program (or a work based on it,\n"
"under Section 2) in object code or executable form under the terms of\n"
"Sections 1 and 2 above provided that you also do one of the following:\n"
"\n"
"    a) Accompany it with the complete corresponding machine-readable\n"
"    source code, which must be distributed under the terms of Sections\n"
"    1 and 2 above on a medium customarily used for software interchange; or,\n"
"\n"
"    b) Accompany it with a written offer, valid for at least three\n"
"    years, to give any third party, for a charge no more than your\n"
"    cost of physically performing source distribution, a co"
                        "mplete\n"
"    machine-readable copy of the corresponding source code, to be\n"
"    distributed under the terms of Sections 1 and 2 above on a medium\n"
"    customarily used for software interchange; or,\n"
"\n"
"    c) Accompany it with the information you received as to the offer\n"
"    to distribute corresponding source code.  (This alternative is\n"
"    allowed only for noncommercial distribution and only if you\n"
"    received the program in object code or executable form with such\n"
"    an offer, in accord with Subsection b above.)\n"
"\n"
"The source code for a work means the preferred form of the work for\n"
"making modifications to it.  For an executable work, complete source\n"
"code means all the source code for all modules it contains, plus any\n"
"associated interface definition files, plus the scripts used to\n"
"control compilation and installation of the executable.  However, as a\n"
"special exception, the source code distributed need not include\n"
"anything that is normally distribute"
                        "d (in either source or binary\n"
"form) with the major components (compiler, kernel, and so on) of the\n"
"operating system on which the executable runs, unless that component\n"
"itself accompanies the executable.\n"
"\n"
"If distribution of executable or object code is made by offering\n"
"access to copy from a designated place, then offering equivalent\n"
"access to copy the source code from the same place counts as\n"
"distribution of the source code, even though third parties are not\n"
"compelled to copy the source along with the object code.\n"
"\n"
"  4. You may not copy, modify, sublicense, or distribute the Program\n"
"except as expressly provided under this License.  Any attempt\n"
"otherwise to copy, modify, sublicense or distribute the Program is\n"
"void, and will automatically terminate your rights under this License.\n"
"However, parties who have received copies, or rights, from you under\n"
"this License will not have their licenses terminated so long as such\n"
"parties remain in full complia"
                        "nce.\n"
"\n"
"  5. You are not required to accept this License, since you have not\n"
"signed it.  However, nothing else grants you permission to modify or\n"
"distribute the Program or its derivative works.  These actions are\n"
"prohibited by law if you do not accept this License.  Therefore, by\n"
"modifying or distributing the Program (or any work based on the\n"
"Program), you indicate your acceptance of this License to do so, and\n"
"all its terms and conditions for copying, distributing or modifying\n"
"the Program or works based on it.\n"
"\n"
"  6. Each time you redistribute the Program (or any work based on the\n"
"Program), the recipient automatically receives a license from the\n"
"original licensor to copy, distribute or modify the Program subject to\n"
"these terms and conditions.  You may not impose any further\n"
"restrictions on the recipients' exercise of the rights granted herein.\n"
"You are not responsible for enforcing compliance by third parties to\n"
"this License.\n"
"\n"
"  7. If, as "
                        "a consequence of a court judgment or allegation of patent\n"
"infringement or for any other reason (not limited to patent issues),\n"
"conditions are imposed on you (whether by court order, agreement or\n"
"otherwise) that contradict the conditions of this License, they do not\n"
"excuse you from the conditions of this License.  If you cannot\n"
"distribute so as to satisfy simultaneously your obligations under this\n"
"License and any other pertinent obligations, then as a consequence you\n"
"may not distribute the Program at all.  For example, if a patent\n"
"license would not permit royalty-free redistribution of the Program by\n"
"all those who receive copies directly or indirectly through you, then\n"
"the only way you could satisfy both it and this License would be to\n"
"refrain entirely from distribution of the Program.\n"
"\n"
"If any portion of this section is held invalid or unenforceable under\n"
"any particular circumstance, the balance of the section is intended to\n"
"apply and the section as a "
                        "whole is intended to apply in other\n"
"circumstances.\n"
"\n"
"It is not the purpose of this section to induce you to infringe any\n"
"patents or other property right claims or to contest validity of any\n"
"such claims; this section has the sole purpose of protecting the\n"
"integrity of the free software distribution system, which is\n"
"implemented by public license practices.  Many people have made\n"
"generous contributions to the wide range of software distributed\n"
"through that system in reliance on consistent application of that\n"
"system; it is up to the author/donor to decide if he or she is willing\n"
"to distribute software through any other system and a licensee cannot\n"
"impose that choice.\n"
"\n"
"This section is intended to make thoroughly clear what is believed to\n"
"be a consequence of the rest of this License.\n"
"\n"
"  8. If the distribution and/or use of the Program is restricted in\n"
"certain countries either by patents or by copyrighted interfaces, the\n"
"original copyright hol"
                        "der who places the Program under this License\n"
"may add an explicit geographical distribution limitation excluding\n"
"those countries, so that distribution is permitted only in or among\n"
"countries not thus excluded.  In such case, this License incorporates\n"
"the limitation as if written in the body of this License.\n"
"\n"
"  9. The Free Software Foundation may publish revised and/or new versions\n"
"of the General Public License from time to time.  Such new versions will\n"
"be similar in spirit to the present version, but may differ in detail to\n"
"address new problems or concerns.\n"
"\n"
"Each version is given a distinguishing version number.  If the Program\n"
"specifies a version number of this License which applies to it and \"any\n"
"later version\", you have the option of following the terms and conditions\n"
"either of that version or of any later version published by the Free\n"
"Software Foundation.  If the Program does not specify a version number of\n"
"this License, you may choose any v"
                        "ersion ever published by the Free Software\n"
"Foundation.\n"
"\n"
"  10. If you wish to incorporate parts of the Program into other free\n"
"programs whose distribution conditions are different, write to the author\n"
"to ask for permission.  For software which is copyrighted by the Free\n"
"Software Foundation, write to the Free Software Foundation; we sometimes\n"
"make exceptions for this.  Our decision will be guided by the two goals\n"
"of preserving the free status of all derivatives of our free software and\n"
"of promoting the sharing and reuse of software generally.\n"
"\n"
"                            NO WARRANTY\n"
"\n"
"  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n"
"FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n"
"OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n"
"PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n"
"OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
""
                        "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n"
"TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n"
"PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n"
"REPAIR OR CORRECTION.\n"
"\n"
"  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n"
"WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n"
"REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n"
"INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n"
"OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n"
"TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n"
"YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n"
"PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n"
"POSSIBILITY OF SUCH DAMAGES.\n"
"\n"
"                     END OF TERMS AND CONDITIONS\n"
"", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_license), QCoreApplication::translate("CarlaAboutW", "License", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CarlaAboutW: public Ui_CarlaAboutW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_ABOUT_H
