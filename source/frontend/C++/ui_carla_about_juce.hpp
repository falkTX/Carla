/********************************************************************************
** Form generated from reading UI file 'carla_about_juce.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_ABOUT_JUCE_H
#define UI_CARLA_ABOUT_JUCE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_JuceAboutW
{
public:
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout;
    QLabel *icon;
    QSpacerItem *verticalSpacer;
    QVBoxLayout *verticalLayout_2;
    QLabel *l_text1;
    QSpacerItem *verticalSpacer_3;
    QLabel *l_text2;
    QSpacerItem *verticalSpacer_4;
    QLabel *l_text3;
    QSpacerItem *verticalSpacer_2;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *JuceAboutW)
    {
        if (JuceAboutW->objectName().isEmpty())
            JuceAboutW->setObjectName(QString::fromUtf8("JuceAboutW"));
        JuceAboutW->resize(463, 244);
        gridLayout = new QGridLayout(JuceAboutW);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        icon = new QLabel(JuceAboutW);
        icon->setObjectName(QString::fromUtf8("icon"));
        icon->setMinimumSize(QSize(48, 48));
        icon->setMaximumSize(QSize(48, 48));
        icon->setPixmap(QPixmap(QString::fromUtf8(":/48x48/juce.png")));

        verticalLayout->addWidget(icon);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        l_text1 = new QLabel(JuceAboutW);
        l_text1->setObjectName(QString::fromUtf8("l_text1"));

        verticalLayout_2->addWidget(l_text1);

        verticalSpacer_3 = new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_2->addItem(verticalSpacer_3);

        l_text2 = new QLabel(JuceAboutW);
        l_text2->setObjectName(QString::fromUtf8("l_text2"));

        verticalLayout_2->addWidget(l_text2);

        verticalSpacer_4 = new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_2->addItem(verticalSpacer_4);

        l_text3 = new QLabel(JuceAboutW);
        l_text3->setObjectName(QString::fromUtf8("l_text3"));
        l_text3->setWordWrap(true);

        verticalLayout_2->addWidget(l_text3);

        verticalSpacer_2 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_2);


        gridLayout->addLayout(verticalLayout_2, 0, 1, 1, 1);

        buttonBox = new QDialogButtonBox(JuceAboutW);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 1, 0, 1, 2);


        retranslateUi(JuceAboutW);
        QObject::connect(buttonBox, SIGNAL(accepted()), JuceAboutW, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), JuceAboutW, SLOT(reject()));

        QMetaObject::connectSlotsByName(JuceAboutW);
    } // setupUi

    void retranslateUi(QDialog *JuceAboutW)
    {
        JuceAboutW->setWindowTitle(QCoreApplication::translate("JuceAboutW", "About JUCE", nullptr));
        icon->setText(QString());
        l_text1->setText(QCoreApplication::translate("JuceAboutW", "<b>About JUCE</b>", nullptr));
        l_text2->setText(QCoreApplication::translate("JuceAboutW", "This program uses JUCE version 3.x.x.", nullptr));
        l_text3->setText(QCoreApplication::translate("JuceAboutW", "JUCE (Jules' Utility Class Extensions) is an all-encompassing C++ class library for developing cross-platform software.\n"
"\n"
"It contains pretty much everything you're likely to need to create most applications, and is particularly well-suited for building highly-customised GUIs, and for handling graphics and sound.\n"
"\n"
"JUCE is licensed under the GNU Public Licence version 2.0.\n"
"One module (juce_core) is permissively licensed under the ISC.\n"
"\n"
"Copyright (C) 2017 ROLI Ltd.", nullptr));
    } // retranslateUi

};

namespace Ui {
    class JuceAboutW: public Ui_JuceAboutW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_ABOUT_JUCE_H
