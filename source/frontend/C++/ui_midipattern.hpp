/********************************************************************************
** Form generated from reading UI file 'midipattern.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MIDIPATTERN_H
#define UI_MIDIPATTERN_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "widgets/pianoroll.h"

QT_BEGIN_NAMESPACE

class Ui_MidiPatternW
{
public:
    QAction *act_file_quit;
    QAction *act_edit_insert;
    QAction *act_edit_velocity;
    QAction *act_edit_select_all;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    ModeIndicator *modeIndicator;
    QLabel *timeSigLabel;
    QComboBox *timeSigBox;
    QSpacerItem *horizontalSpacer;
    QLabel *measureLabel;
    QComboBox *measureBox;
    QSpacerItem *horizontalSpacer_2;
    QLabel *defaultLengthLabel;
    QComboBox *defaultLengthBox;
    QSpacerItem *horizontalSpacer_3;
    QLabel *quantizeLabel;
    QComboBox *quantizeBox;
    QSpacerItem *horizontalSpacer_4;
    QSlider *hSlider;
    QHBoxLayout *horizontalLayout_2;
    QSlider *vSlider;
    PianoRollView *graphicsView;
    QMenuBar *menubar;
    QMenu *menu_File;
    QMenu *menu_Edit;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MidiPatternW)
    {
        if (MidiPatternW->objectName().isEmpty())
            MidiPatternW->setObjectName(QString::fromUtf8("MidiPatternW"));
        MidiPatternW->resize(755, 436);
        act_file_quit = new QAction(MidiPatternW);
        act_file_quit->setObjectName(QString::fromUtf8("act_file_quit"));
        act_edit_insert = new QAction(MidiPatternW);
        act_edit_insert->setObjectName(QString::fromUtf8("act_edit_insert"));
        act_edit_insert->setCheckable(true);
        act_edit_velocity = new QAction(MidiPatternW);
        act_edit_velocity->setObjectName(QString::fromUtf8("act_edit_velocity"));
        act_edit_velocity->setCheckable(true);
        act_edit_select_all = new QAction(MidiPatternW);
        act_edit_select_all->setObjectName(QString::fromUtf8("act_edit_select_all"));
        centralwidget = new QWidget(MidiPatternW);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        modeIndicator = new ModeIndicator(centralwidget);
        modeIndicator->setObjectName(QString::fromUtf8("modeIndicator"));
        modeIndicator->setMinimumSize(QSize(30, 20));
        modeIndicator->setMaximumSize(QSize(30, 20));

        horizontalLayout->addWidget(modeIndicator);

        timeSigLabel = new QLabel(centralwidget);
        timeSigLabel->setObjectName(QString::fromUtf8("timeSigLabel"));
        timeSigLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(timeSigLabel);

        timeSigBox = new QComboBox(centralwidget);
        timeSigBox->addItem(QString());
        timeSigBox->addItem(QString());
        timeSigBox->addItem(QString());
        timeSigBox->addItem(QString());
        timeSigBox->addItem(QString());
        timeSigBox->addItem(QString());
        timeSigBox->setObjectName(QString::fromUtf8("timeSigBox"));
        timeSigBox->setEditable(true);

        horizontalLayout->addWidget(timeSigBox);

        horizontalSpacer = new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        measureLabel = new QLabel(centralwidget);
        measureLabel->setObjectName(QString::fromUtf8("measureLabel"));
        measureLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(measureLabel);

        measureBox = new QComboBox(centralwidget);
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->addItem(QString());
        measureBox->setObjectName(QString::fromUtf8("measureBox"));

        horizontalLayout->addWidget(measureBox);

        horizontalSpacer_2 = new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        defaultLengthLabel = new QLabel(centralwidget);
        defaultLengthLabel->setObjectName(QString::fromUtf8("defaultLengthLabel"));

        horizontalLayout->addWidget(defaultLengthLabel);

        defaultLengthBox = new QComboBox(centralwidget);
        defaultLengthBox->addItem(QString());
        defaultLengthBox->addItem(QString());
        defaultLengthBox->addItem(QString());
        defaultLengthBox->addItem(QString());
        defaultLengthBox->addItem(QString());
        defaultLengthBox->addItem(QString());
        defaultLengthBox->addItem(QString());
        defaultLengthBox->addItem(QString());
        defaultLengthBox->addItem(QString());
        defaultLengthBox->addItem(QString());
        defaultLengthBox->setObjectName(QString::fromUtf8("defaultLengthBox"));

        horizontalLayout->addWidget(defaultLengthBox);

        horizontalSpacer_3 = new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);

        quantizeLabel = new QLabel(centralwidget);
        quantizeLabel->setObjectName(QString::fromUtf8("quantizeLabel"));

        horizontalLayout->addWidget(quantizeLabel);

        quantizeBox = new QComboBox(centralwidget);
        quantizeBox->addItem(QString());
        quantizeBox->addItem(QString());
        quantizeBox->addItem(QString());
        quantizeBox->addItem(QString());
        quantizeBox->addItem(QString());
        quantizeBox->addItem(QString());
        quantizeBox->addItem(QString());
        quantizeBox->addItem(QString());
        quantizeBox->addItem(QString());
        quantizeBox->addItem(QString());
        quantizeBox->setObjectName(QString::fromUtf8("quantizeBox"));

        horizontalLayout->addWidget(quantizeBox);

        horizontalSpacer_4 = new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_4);

        hSlider = new QSlider(centralwidget);
        hSlider->setObjectName(QString::fromUtf8("hSlider"));
        hSlider->setOrientation(Qt::Horizontal);

        horizontalLayout->addWidget(hSlider);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        vSlider = new QSlider(centralwidget);
        vSlider->setObjectName(QString::fromUtf8("vSlider"));
        vSlider->setOrientation(Qt::Vertical);

        horizontalLayout_2->addWidget(vSlider);

        graphicsView = new PianoRollView(centralwidget);
        graphicsView->setObjectName(QString::fromUtf8("graphicsView"));

        horizontalLayout_2->addWidget(graphicsView);


        verticalLayout->addLayout(horizontalLayout_2);

        MidiPatternW->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MidiPatternW);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 755, 20));
        menu_File = new QMenu(menubar);
        menu_File->setObjectName(QString::fromUtf8("menu_File"));
        menu_Edit = new QMenu(menubar);
        menu_Edit->setObjectName(QString::fromUtf8("menu_Edit"));
        MidiPatternW->setMenuBar(menubar);
        statusbar = new QStatusBar(MidiPatternW);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MidiPatternW->setStatusBar(statusbar);

        menubar->addAction(menu_File->menuAction());
        menubar->addAction(menu_Edit->menuAction());
        menu_File->addAction(act_file_quit);
        menu_Edit->addAction(act_edit_insert);
        menu_Edit->addAction(act_edit_velocity);
        menu_Edit->addAction(act_edit_select_all);

        retranslateUi(MidiPatternW);
        QObject::connect(act_file_quit, SIGNAL(triggered()), MidiPatternW, SLOT(close()));

        timeSigBox->setCurrentIndex(0);
        measureBox->setCurrentIndex(3);
        defaultLengthBox->setCurrentIndex(4);
        quantizeBox->setCurrentIndex(4);


        QMetaObject::connectSlotsByName(MidiPatternW);
    } // setupUi

    void retranslateUi(QMainWindow *MidiPatternW)
    {
        MidiPatternW->setWindowTitle(QCoreApplication::translate("MidiPatternW", "MIDI Pattern", nullptr));
        act_file_quit->setText(QCoreApplication::translate("MidiPatternW", "&Quit", nullptr));
        act_edit_insert->setText(QCoreApplication::translate("MidiPatternW", "&Insert Mode", nullptr));
#if QT_CONFIG(shortcut)
        act_edit_insert->setShortcut(QCoreApplication::translate("MidiPatternW", "F", nullptr));
#endif // QT_CONFIG(shortcut)
        act_edit_velocity->setText(QCoreApplication::translate("MidiPatternW", "&Velocity Mode", nullptr));
#if QT_CONFIG(shortcut)
        act_edit_velocity->setShortcut(QCoreApplication::translate("MidiPatternW", "D", nullptr));
#endif // QT_CONFIG(shortcut)
        act_edit_select_all->setText(QCoreApplication::translate("MidiPatternW", "Select All", nullptr));
#if QT_CONFIG(shortcut)
        act_edit_select_all->setShortcut(QCoreApplication::translate("MidiPatternW", "A", nullptr));
#endif // QT_CONFIG(shortcut)
        timeSigLabel->setText(QCoreApplication::translate("MidiPatternW", "Time Signature:", nullptr));
        timeSigBox->setItemText(0, QCoreApplication::translate("MidiPatternW", "1/4", nullptr));
        timeSigBox->setItemText(1, QCoreApplication::translate("MidiPatternW", "2/4", nullptr));
        timeSigBox->setItemText(2, QCoreApplication::translate("MidiPatternW", "3/4", nullptr));
        timeSigBox->setItemText(3, QCoreApplication::translate("MidiPatternW", "4/4", nullptr));
        timeSigBox->setItemText(4, QCoreApplication::translate("MidiPatternW", "5/4", nullptr));
        timeSigBox->setItemText(5, QCoreApplication::translate("MidiPatternW", "6/4", nullptr));

        measureLabel->setText(QCoreApplication::translate("MidiPatternW", "Measures:", nullptr));
        measureBox->setItemText(0, QCoreApplication::translate("MidiPatternW", "1", nullptr));
        measureBox->setItemText(1, QCoreApplication::translate("MidiPatternW", "2", nullptr));
        measureBox->setItemText(2, QCoreApplication::translate("MidiPatternW", "3", nullptr));
        measureBox->setItemText(3, QCoreApplication::translate("MidiPatternW", "4", nullptr));
        measureBox->setItemText(4, QCoreApplication::translate("MidiPatternW", "5", nullptr));
        measureBox->setItemText(5, QCoreApplication::translate("MidiPatternW", "6", nullptr));
        measureBox->setItemText(6, QCoreApplication::translate("MidiPatternW", "7", nullptr));
        measureBox->setItemText(7, QCoreApplication::translate("MidiPatternW", "8", nullptr));
        measureBox->setItemText(8, QCoreApplication::translate("MidiPatternW", "9", nullptr));
        measureBox->setItemText(9, QCoreApplication::translate("MidiPatternW", "10", nullptr));
        measureBox->setItemText(10, QCoreApplication::translate("MidiPatternW", "11", nullptr));
        measureBox->setItemText(11, QCoreApplication::translate("MidiPatternW", "12", nullptr));
        measureBox->setItemText(12, QCoreApplication::translate("MidiPatternW", "13", nullptr));
        measureBox->setItemText(13, QCoreApplication::translate("MidiPatternW", "14", nullptr));
        measureBox->setItemText(14, QCoreApplication::translate("MidiPatternW", "15", nullptr));
        measureBox->setItemText(15, QCoreApplication::translate("MidiPatternW", "16", nullptr));

        defaultLengthLabel->setText(QCoreApplication::translate("MidiPatternW", "Default Length:", nullptr));
        defaultLengthBox->setItemText(0, QCoreApplication::translate("MidiPatternW", "1/16", nullptr));
        defaultLengthBox->setItemText(1, QCoreApplication::translate("MidiPatternW", "1/15", nullptr));
        defaultLengthBox->setItemText(2, QCoreApplication::translate("MidiPatternW", "1/12", nullptr));
        defaultLengthBox->setItemText(3, QCoreApplication::translate("MidiPatternW", "1/9", nullptr));
        defaultLengthBox->setItemText(4, QCoreApplication::translate("MidiPatternW", "1/8", nullptr));
        defaultLengthBox->setItemText(5, QCoreApplication::translate("MidiPatternW", "1/6", nullptr));
        defaultLengthBox->setItemText(6, QCoreApplication::translate("MidiPatternW", "1/4", nullptr));
        defaultLengthBox->setItemText(7, QCoreApplication::translate("MidiPatternW", "1/3", nullptr));
        defaultLengthBox->setItemText(8, QCoreApplication::translate("MidiPatternW", "1/2", nullptr));
        defaultLengthBox->setItemText(9, QCoreApplication::translate("MidiPatternW", "1", nullptr));

        quantizeLabel->setText(QCoreApplication::translate("MidiPatternW", "Quantize:", nullptr));
        quantizeBox->setItemText(0, QCoreApplication::translate("MidiPatternW", "1/16", nullptr));
        quantizeBox->setItemText(1, QCoreApplication::translate("MidiPatternW", "1/15", nullptr));
        quantizeBox->setItemText(2, QCoreApplication::translate("MidiPatternW", "1/12", nullptr));
        quantizeBox->setItemText(3, QCoreApplication::translate("MidiPatternW", "1/9", nullptr));
        quantizeBox->setItemText(4, QCoreApplication::translate("MidiPatternW", "1/8", nullptr));
        quantizeBox->setItemText(5, QCoreApplication::translate("MidiPatternW", "1/6", nullptr));
        quantizeBox->setItemText(6, QCoreApplication::translate("MidiPatternW", "1/4", nullptr));
        quantizeBox->setItemText(7, QCoreApplication::translate("MidiPatternW", "1/3", nullptr));
        quantizeBox->setItemText(8, QCoreApplication::translate("MidiPatternW", "1/2", nullptr));
        quantizeBox->setItemText(9, QCoreApplication::translate("MidiPatternW", "1", nullptr));

        menu_File->setTitle(QCoreApplication::translate("MidiPatternW", "&File", nullptr));
        menu_Edit->setTitle(QCoreApplication::translate("MidiPatternW", "&Edit", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MidiPatternW: public Ui_MidiPatternW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MIDIPATTERN_H
