/********************************************************************************
** Form generated from reading UI file 'carla_parameter.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_PARAMETER_H
#define UI_CARLA_PARAMETER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>
#include "widgets/paramspinbox.h"

QT_BEGIN_NAMESPACE

class Ui_PluginParameter
{
public:
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    ParamSpinBox *widget;
    QSpinBox *sb_control;
    QSpinBox *sb_channel;

    void setupUi(QWidget *PluginParameter)
    {
        if (PluginParameter->objectName().isEmpty())
            PluginParameter->setObjectName(QString::fromUtf8("PluginParameter"));
        PluginParameter->resize(369, 22);
        horizontalLayout = new QHBoxLayout(PluginParameter);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(PluginParameter);
        label->setObjectName(QString::fromUtf8("label"));
        label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(label);

        widget = new ParamSpinBox(PluginParameter);
        widget->setObjectName(QString::fromUtf8("widget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
        widget->setSizePolicy(sizePolicy);
        widget->setContextMenuPolicy(Qt::CustomContextMenu);

        horizontalLayout->addWidget(widget);

        sb_control = new QSpinBox(PluginParameter);
        sb_control->setObjectName(QString::fromUtf8("sb_control"));
        sb_control->setContextMenuPolicy(Qt::CustomContextMenu);
        sb_control->setMinimum(-1);
        sb_control->setMaximum(119);
        sb_control->setValue(-1);

        horizontalLayout->addWidget(sb_control);

        sb_channel = new QSpinBox(PluginParameter);
        sb_channel->setObjectName(QString::fromUtf8("sb_channel"));
        sb_channel->setContextMenuPolicy(Qt::CustomContextMenu);
        sb_channel->setMinimum(1);
        sb_channel->setMaximum(16);
        sb_channel->setValue(1);

        horizontalLayout->addWidget(sb_channel);


        retranslateUi(PluginParameter);

        QMetaObject::connectSlotsByName(PluginParameter);
    } // setupUi

    void retranslateUi(QWidget *PluginParameter)
    {
        PluginParameter->setWindowTitle(QCoreApplication::translate("PluginParameter", "Form", nullptr));
        label->setText(QCoreApplication::translate("PluginParameter", "Parameter Name", nullptr));
        sb_control->setSpecialValueText(QCoreApplication::translate("PluginParameter", "(none)", nullptr));
        sb_control->setPrefix(QCoreApplication::translate("PluginParameter", "cc #", nullptr));
        sb_channel->setPrefix(QCoreApplication::translate("PluginParameter", "ch ", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PluginParameter: public Ui_PluginParameter {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_PARAMETER_H
