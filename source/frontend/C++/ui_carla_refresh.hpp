/********************************************************************************
** Form generated from reading UI file 'carla_refresh.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_REFRESH_H
#define UI_CARLA_REFRESH_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_PluginRefreshW
{
public:
    QVBoxLayout *verticalLayout_5;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer;
    QGroupBox *group_types;
    QHBoxLayout *horizontalLayout_2;
    QVBoxLayout *verticalLayout;
    QCheckBox *ch_ladspa;
    QCheckBox *ch_dssi;
    QCheckBox *ch_lv2;
    QCheckBox *ch_vst;
    QCheckBox *ch_vst3;
    QCheckBox *ch_au;
    QFrame *line_3;
    QCheckBox *ch_sf2;
    QCheckBox *ch_sfz;
    QSpacerItem *horizontalSpacer_4;
    QSpacerItem *verticalSpacer_3;
    QFrame *sep_format;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *ch_native;
    QCheckBox *ch_posix32;
    QCheckBox *ch_posix64;
    QCheckBox *ch_win32;
    QCheckBox *ch_win64;
    QSpacerItem *verticalSpacer_2;
    QSpacerItem *horizontalSpacer_3;
    QVBoxLayout *verticalLayout_3;
    QGroupBox *group_tools;
    QGridLayout *gridLayout;
    QLabel *ico_native;
    QLabel *ico_posix32;
    QLabel *label_rdflib;
    QLabel *label_win64;
    QLabel *label_native;
    QLabel *ico_posix64;
    QLabel *ico_rdflib;
    QLabel *label_posix32;
    QLabel *ico_win32;
    QLabel *ico_win64;
    QLabel *label_posix64;
    QLabel *label_win32;
    QSpacerItem *verticalSpacer_5;
    QGroupBox *group_options;
    QVBoxLayout *verticalLayout_4;
    QCheckBox *ch_do_checks;
    QSpacerItem *verticalSpacer_4;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout;
    QProgressBar *progressBar;
    QPushButton *b_start;
    QPushButton *b_skip;
    QPushButton *b_close;

    void setupUi(QDialog *PluginRefreshW)
    {
        if (PluginRefreshW->objectName().isEmpty())
            PluginRefreshW->setObjectName(QString::fromUtf8("PluginRefreshW"));
        PluginRefreshW->resize(686, 330);
        verticalLayout_5 = new QVBoxLayout(PluginRefreshW);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer = new QSpacerItem(30, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer);

        group_types = new QGroupBox(PluginRefreshW);
        group_types->setObjectName(QString::fromUtf8("group_types"));
        group_types->setAlignment(Qt::AlignCenter);
        horizontalLayout_2 = new QHBoxLayout(group_types);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        ch_ladspa = new QCheckBox(group_types);
        ch_ladspa->setObjectName(QString::fromUtf8("ch_ladspa"));

        verticalLayout->addWidget(ch_ladspa);

        ch_dssi = new QCheckBox(group_types);
        ch_dssi->setObjectName(QString::fromUtf8("ch_dssi"));

        verticalLayout->addWidget(ch_dssi);

        ch_lv2 = new QCheckBox(group_types);
        ch_lv2->setObjectName(QString::fromUtf8("ch_lv2"));

        verticalLayout->addWidget(ch_lv2);

        ch_vst = new QCheckBox(group_types);
        ch_vst->setObjectName(QString::fromUtf8("ch_vst"));

        verticalLayout->addWidget(ch_vst);

        ch_vst3 = new QCheckBox(group_types);
        ch_vst3->setObjectName(QString::fromUtf8("ch_vst3"));

        verticalLayout->addWidget(ch_vst3);

        ch_au = new QCheckBox(group_types);
        ch_au->setObjectName(QString::fromUtf8("ch_au"));

        verticalLayout->addWidget(ch_au);

        line_3 = new QFrame(group_types);
        line_3->setObjectName(QString::fromUtf8("line_3"));
        line_3->setLineWidth(0);
        line_3->setMidLineWidth(1);
        line_3->setFrameShape(QFrame::HLine);
        line_3->setFrameShadow(QFrame::Sunken);

        verticalLayout->addWidget(line_3);

        ch_sf2 = new QCheckBox(group_types);
        ch_sf2->setObjectName(QString::fromUtf8("ch_sf2"));

        verticalLayout->addWidget(ch_sf2);

        ch_sfz = new QCheckBox(group_types);
        ch_sfz->setObjectName(QString::fromUtf8("ch_sfz"));

        verticalLayout->addWidget(ch_sfz);

        horizontalSpacer_4 = new QSpacerItem(40, 5, QSizePolicy::Expanding, QSizePolicy::Minimum);

        verticalLayout->addItem(horizontalSpacer_4);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_3);


        horizontalLayout_2->addLayout(verticalLayout);

        sep_format = new QFrame(group_types);
        sep_format->setObjectName(QString::fromUtf8("sep_format"));
        sep_format->setLineWidth(0);
        sep_format->setMidLineWidth(1);
        sep_format->setFrameShape(QFrame::VLine);
        sep_format->setFrameShadow(QFrame::Sunken);

        horizontalLayout_2->addWidget(sep_format);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        ch_native = new QCheckBox(group_types);
        ch_native->setObjectName(QString::fromUtf8("ch_native"));

        verticalLayout_2->addWidget(ch_native);

        ch_posix32 = new QCheckBox(group_types);
        ch_posix32->setObjectName(QString::fromUtf8("ch_posix32"));

        verticalLayout_2->addWidget(ch_posix32);

        ch_posix64 = new QCheckBox(group_types);
        ch_posix64->setObjectName(QString::fromUtf8("ch_posix64"));

        verticalLayout_2->addWidget(ch_posix64);

        ch_win32 = new QCheckBox(group_types);
        ch_win32->setObjectName(QString::fromUtf8("ch_win32"));

        verticalLayout_2->addWidget(ch_win32);

        ch_win64 = new QCheckBox(group_types);
        ch_win64->setObjectName(QString::fromUtf8("ch_win64"));

        verticalLayout_2->addWidget(ch_win64);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_2);


        horizontalLayout_2->addLayout(verticalLayout_2);


        horizontalLayout_3->addWidget(group_types);

        horizontalSpacer_3 = new QSpacerItem(20, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        group_tools = new QGroupBox(PluginRefreshW);
        group_tools->setObjectName(QString::fromUtf8("group_tools"));
        group_tools->setAlignment(Qt::AlignCenter);
        group_tools->setFlat(true);
        gridLayout = new QGridLayout(group_tools);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        ico_native = new QLabel(group_tools);
        ico_native->setObjectName(QString::fromUtf8("ico_native"));
        ico_native->setMaximumSize(QSize(16, 16));
        ico_native->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-ok-apply.svgz")));
        ico_native->setScaledContents(true);

        gridLayout->addWidget(ico_native, 0, 0, 1, 1);

        ico_posix32 = new QLabel(group_tools);
        ico_posix32->setObjectName(QString::fromUtf8("ico_posix32"));
        ico_posix32->setMaximumSize(QSize(16, 16));
        ico_posix32->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-ok-apply.svgz")));
        ico_posix32->setScaledContents(true);

        gridLayout->addWidget(ico_posix32, 1, 0, 1, 1);

        label_rdflib = new QLabel(group_tools);
        label_rdflib->setObjectName(QString::fromUtf8("label_rdflib"));

        gridLayout->addWidget(label_rdflib, 5, 1, 1, 1);

        label_win64 = new QLabel(group_tools);
        label_win64->setObjectName(QString::fromUtf8("label_win64"));

        gridLayout->addWidget(label_win64, 4, 1, 1, 1);

        label_native = new QLabel(group_tools);
        label_native->setObjectName(QString::fromUtf8("label_native"));

        gridLayout->addWidget(label_native, 0, 1, 1, 1);

        ico_posix64 = new QLabel(group_tools);
        ico_posix64->setObjectName(QString::fromUtf8("ico_posix64"));
        ico_posix64->setMaximumSize(QSize(16, 16));
        ico_posix64->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-ok-apply.svgz")));
        ico_posix64->setScaledContents(true);

        gridLayout->addWidget(ico_posix64, 2, 0, 1, 1);

        ico_rdflib = new QLabel(group_tools);
        ico_rdflib->setObjectName(QString::fromUtf8("ico_rdflib"));
        ico_rdflib->setMaximumSize(QSize(16, 16));
        ico_rdflib->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-ok-apply.svgz")));
        ico_rdflib->setScaledContents(true);

        gridLayout->addWidget(ico_rdflib, 5, 0, 1, 1);

        label_posix32 = new QLabel(group_tools);
        label_posix32->setObjectName(QString::fromUtf8("label_posix32"));

        gridLayout->addWidget(label_posix32, 1, 1, 1, 1);

        ico_win32 = new QLabel(group_tools);
        ico_win32->setObjectName(QString::fromUtf8("ico_win32"));
        ico_win32->setMaximumSize(QSize(16, 16));
        ico_win32->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-ok-apply.svgz")));
        ico_win32->setScaledContents(true);

        gridLayout->addWidget(ico_win32, 3, 0, 1, 1);

        ico_win64 = new QLabel(group_tools);
        ico_win64->setObjectName(QString::fromUtf8("ico_win64"));
        ico_win64->setMaximumSize(QSize(16, 16));
        ico_win64->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-ok-apply.svgz")));
        ico_win64->setScaledContents(true);

        gridLayout->addWidget(ico_win64, 4, 0, 1, 1);

        label_posix64 = new QLabel(group_tools);
        label_posix64->setObjectName(QString::fromUtf8("label_posix64"));

        gridLayout->addWidget(label_posix64, 2, 1, 1, 1);

        label_win32 = new QLabel(group_tools);
        label_win32->setObjectName(QString::fromUtf8("label_win32"));

        gridLayout->addWidget(label_win32, 3, 1, 1, 1);


        verticalLayout_3->addWidget(group_tools);

        verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_5);

        group_options = new QGroupBox(PluginRefreshW);
        group_options->setObjectName(QString::fromUtf8("group_options"));
        group_options->setAlignment(Qt::AlignCenter);
        group_options->setFlat(true);
        verticalLayout_4 = new QVBoxLayout(group_options);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        ch_do_checks = new QCheckBox(group_options);
        ch_do_checks->setObjectName(QString::fromUtf8("ch_do_checks"));

        verticalLayout_4->addWidget(ch_do_checks);


        verticalLayout_3->addWidget(group_options);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_4);


        horizontalLayout_3->addLayout(verticalLayout_3);

        horizontalSpacer_2 = new QSpacerItem(30, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);


        verticalLayout_5->addLayout(horizontalLayout_3);

        verticalSpacer = new QSpacerItem(20, 6, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        progressBar = new QProgressBar(PluginRefreshW);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(progressBar->sizePolicy().hasHeightForWidth());
        progressBar->setSizePolicy(sizePolicy);
        progressBar->setMaximum(100);
        progressBar->setValue(0);

        horizontalLayout->addWidget(progressBar);

        b_start = new QPushButton(PluginRefreshW);
        b_start->setObjectName(QString::fromUtf8("b_start"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/16x16/arrow-right.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_start->setIcon(icon);

        horizontalLayout->addWidget(b_start);

        b_skip = new QPushButton(PluginRefreshW);
        b_skip->setObjectName(QString::fromUtf8("b_skip"));

        horizontalLayout->addWidget(b_skip);

        b_close = new QPushButton(PluginRefreshW);
        b_close->setObjectName(QString::fromUtf8("b_close"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/16x16/window-close.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_close->setIcon(icon1);

        horizontalLayout->addWidget(b_close);


        verticalLayout_5->addLayout(horizontalLayout);


        retranslateUi(PluginRefreshW);
        QObject::connect(b_close, SIGNAL(clicked()), PluginRefreshW, SLOT(close()));

        QMetaObject::connectSlotsByName(PluginRefreshW);
    } // setupUi

    void retranslateUi(QDialog *PluginRefreshW)
    {
        PluginRefreshW->setWindowTitle(QCoreApplication::translate("PluginRefreshW", "Carla - Refresh", nullptr));
        group_types->setTitle(QCoreApplication::translate("PluginRefreshW", "Search for new...", nullptr));
        ch_ladspa->setText(QCoreApplication::translate("PluginRefreshW", "LADSPA", nullptr));
        ch_dssi->setText(QCoreApplication::translate("PluginRefreshW", "DSSI", nullptr));
        ch_lv2->setText(QCoreApplication::translate("PluginRefreshW", "LV2", nullptr));
        ch_vst->setText(QCoreApplication::translate("PluginRefreshW", "VST2", nullptr));
        ch_vst3->setText(QCoreApplication::translate("PluginRefreshW", "VST3", nullptr));
        ch_au->setText(QCoreApplication::translate("PluginRefreshW", "AU", nullptr));
        ch_sf2->setText(QCoreApplication::translate("PluginRefreshW", "SF2/3", nullptr));
        ch_sfz->setText(QCoreApplication::translate("PluginRefreshW", "SFZ", nullptr));
        ch_native->setText(QCoreApplication::translate("PluginRefreshW", "Native", nullptr));
        ch_posix32->setText(QCoreApplication::translate("PluginRefreshW", "POSIX 32bit", nullptr));
        ch_posix64->setText(QCoreApplication::translate("PluginRefreshW", "POSIX 64bit", nullptr));
        ch_win32->setText(QCoreApplication::translate("PluginRefreshW", "Windows 32bit", nullptr));
        ch_win64->setText(QCoreApplication::translate("PluginRefreshW", "Windows 64bit", nullptr));
        group_tools->setTitle(QCoreApplication::translate("PluginRefreshW", "Available tools:", nullptr));
        ico_native->setText(QString());
        ico_posix32->setText(QString());
        label_rdflib->setText(QCoreApplication::translate("PluginRefreshW", "python3-rdflib (LADSPA-RDF support)", nullptr));
        label_win64->setText(QCoreApplication::translate("PluginRefreshW", "carla-discovery-win64", nullptr));
        label_native->setText(QCoreApplication::translate("PluginRefreshW", "carla-discovery-native", nullptr));
        ico_posix64->setText(QString());
        ico_rdflib->setText(QString());
        label_posix32->setText(QCoreApplication::translate("PluginRefreshW", "carla-discovery-posix32", nullptr));
        ico_win32->setText(QString());
        ico_win64->setText(QString());
        label_posix64->setText(QCoreApplication::translate("PluginRefreshW", "carla-discovery-posix64", nullptr));
        label_win32->setText(QCoreApplication::translate("PluginRefreshW", "carla-discovery-win32", nullptr));
        group_options->setTitle(QCoreApplication::translate("PluginRefreshW", "Options:", nullptr));
#if QT_CONFIG(tooltip)
        ch_do_checks->setToolTip(QCoreApplication::translate("PluginRefreshW", "Carla will run small processing checks when scanning the plugins (to make sure they won't crash).\n"
"You can disable these checks to get a faster scanning time (at your own risk).", nullptr));
#endif // QT_CONFIG(tooltip)
        ch_do_checks->setText(QCoreApplication::translate("PluginRefreshW", "Run processing checks while scanning", nullptr));
        progressBar->setFormat(QCoreApplication::translate("PluginRefreshW", "Press 'Scan' to begin the search", nullptr));
        b_start->setText(QCoreApplication::translate("PluginRefreshW", "Scan", nullptr));
        b_skip->setText(QCoreApplication::translate("PluginRefreshW", ">> Skip", nullptr));
        b_close->setText(QCoreApplication::translate("PluginRefreshW", "Close", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PluginRefreshW: public Ui_PluginRefreshW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_REFRESH_H
