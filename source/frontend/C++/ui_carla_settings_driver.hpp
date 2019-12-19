/********************************************************************************
** Form generated from reading UI file 'carla_settings_driver.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_SETTINGS_DRIVER_H
#define UI_CARLA_SETTINGS_DRIVER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_DriverSettingsW
{
public:
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout;
    QComboBox *cb_samplerate;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *horizontalSpacer_2;
    QComboBox *cb_device;
    QSpacerItem *horizontalSpacer_4;
    QLabel *label_device;
    QLabel *label_buffersize;
    QComboBox *cb_buffersize;
    QLabel *label_samplerate;
    QHBoxLayout *layout_triple_buffer;
    QSpacerItem *horizontalSpacer_5;
    QCheckBox *cb_triple_buffer;
    QSpacerItem *horizontalSpacer_6;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_3;
    QPushButton *b_panel;
    QSpacerItem *horizontalSpacer_7;
    QHBoxLayout *layout_restart;
    QSpacerItem *spacer_1;
    QLabel *ico_restart;
    QLabel *label_restart;
    QSpacerItem *spacer_2;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *DriverSettingsW)
    {
        if (DriverSettingsW->objectName().isEmpty())
            DriverSettingsW->setObjectName(QString::fromUtf8("DriverSettingsW"));
        DriverSettingsW->resize(350, 264);
        DriverSettingsW->setMinimumSize(QSize(350, 0));
        verticalLayout = new QVBoxLayout(DriverSettingsW);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        cb_samplerate = new QComboBox(DriverSettingsW);
        cb_samplerate->setObjectName(QString::fromUtf8("cb_samplerate"));

        gridLayout->addWidget(cb_samplerate, 2, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 2, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 1, 2, 1, 1);

        cb_device = new QComboBox(DriverSettingsW);
        cb_device->setObjectName(QString::fromUtf8("cb_device"));

        gridLayout->addWidget(cb_device, 0, 1, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_4, 2, 2, 1, 1);

        label_device = new QLabel(DriverSettingsW);
        label_device->setObjectName(QString::fromUtf8("label_device"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_device->sizePolicy().hasHeightForWidth());
        label_device->setSizePolicy(sizePolicy);
        label_device->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_device, 0, 0, 1, 1);

        label_buffersize = new QLabel(DriverSettingsW);
        label_buffersize->setObjectName(QString::fromUtf8("label_buffersize"));
        sizePolicy.setHeightForWidth(label_buffersize->sizePolicy().hasHeightForWidth());
        label_buffersize->setSizePolicy(sizePolicy);
        label_buffersize->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_buffersize, 1, 0, 1, 1);

        cb_buffersize = new QComboBox(DriverSettingsW);
        cb_buffersize->setObjectName(QString::fromUtf8("cb_buffersize"));

        gridLayout->addWidget(cb_buffersize, 1, 1, 1, 1);

        label_samplerate = new QLabel(DriverSettingsW);
        label_samplerate->setObjectName(QString::fromUtf8("label_samplerate"));
        sizePolicy.setHeightForWidth(label_samplerate->sizePolicy().hasHeightForWidth());
        label_samplerate->setSizePolicy(sizePolicy);
        label_samplerate->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_samplerate, 2, 0, 1, 1);


        verticalLayout->addLayout(gridLayout);

        layout_triple_buffer = new QHBoxLayout();
        layout_triple_buffer->setObjectName(QString::fromUtf8("layout_triple_buffer"));
        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        layout_triple_buffer->addItem(horizontalSpacer_5);

        cb_triple_buffer = new QCheckBox(DriverSettingsW);
        cb_triple_buffer->setObjectName(QString::fromUtf8("cb_triple_buffer"));

        layout_triple_buffer->addWidget(cb_triple_buffer);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        layout_triple_buffer->addItem(horizontalSpacer_6);


        verticalLayout->addLayout(layout_triple_buffer);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);

        b_panel = new QPushButton(DriverSettingsW);
        b_panel->setObjectName(QString::fromUtf8("b_panel"));
        b_panel->setMinimumSize(QSize(32, 0));

        horizontalLayout_2->addWidget(b_panel);

        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_7);


        verticalLayout->addLayout(horizontalLayout_2);

        layout_restart = new QHBoxLayout();
        layout_restart->setObjectName(QString::fromUtf8("layout_restart"));
        spacer_1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        layout_restart->addItem(spacer_1);

        ico_restart = new QLabel(DriverSettingsW);
        ico_restart->setObjectName(QString::fromUtf8("ico_restart"));
        ico_restart->setMaximumSize(QSize(22, 22));
        ico_restart->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-information.svgz")));
        ico_restart->setScaledContents(true);
        ico_restart->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        layout_restart->addWidget(ico_restart);

        label_restart = new QLabel(DriverSettingsW);
        label_restart->setObjectName(QString::fromUtf8("label_restart"));

        layout_restart->addWidget(label_restart);

        spacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        layout_restart->addItem(spacer_2);


        verticalLayout->addLayout(layout_restart);

        buttonBox = new QDialogButtonBox(DriverSettingsW);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(DriverSettingsW);
        QObject::connect(buttonBox, SIGNAL(accepted()), DriverSettingsW, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), DriverSettingsW, SLOT(reject()));

        QMetaObject::connectSlotsByName(DriverSettingsW);
    } // setupUi

    void retranslateUi(QDialog *DriverSettingsW)
    {
        DriverSettingsW->setWindowTitle(QCoreApplication::translate("DriverSettingsW", "Driver Settings", nullptr));
        label_device->setText(QCoreApplication::translate("DriverSettingsW", "Device:", nullptr));
        label_buffersize->setText(QCoreApplication::translate("DriverSettingsW", "Buffer size:", nullptr));
        label_samplerate->setText(QCoreApplication::translate("DriverSettingsW", "Sample rate:", nullptr));
        cb_triple_buffer->setText(QCoreApplication::translate("DriverSettingsW", "Triple buffer", nullptr));
        b_panel->setText(QCoreApplication::translate("DriverSettingsW", "Show Driver Control Panel", nullptr));
        ico_restart->setText(QString());
        label_restart->setText(QCoreApplication::translate("DriverSettingsW", "Restart the engine to load the new settings", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DriverSettingsW: public Ui_DriverSettingsW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_SETTINGS_DRIVER_H
