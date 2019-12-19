/********************************************************************************
** Form generated from reading UI file 'carla_osc_connect.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_OSC_CONNECT_H
#define UI_CARLA_OSC_CONNECT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *group_remote_setup;
    QGridLayout *gridLayout;
    QLabel *label_4;
    QLabel *label_10;
    QSpinBox *sb_udp_port;
    QSpinBox *sb_tcp_port;
    QLabel *label_3;
    QSpacerItem *horizontalSpacer_4;
    QLineEdit *le_host;
    QSpacerItem *horizontalSpacer_3;
    QGroupBox *group_reported_host;
    QGridLayout *gridLayout_2;
    QSpacerItem *horizontalSpacer_10;
    QRadioButton *rb_reported_auto;
    QSpacerItem *horizontalSpacer_9;
    QRadioButton *rb_reported_custom;
    QLineEdit *le_reported_host;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPlainTextEdit *te_reported_hint;
    QSpacerItem *horizontalSpacer_2;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QString::fromUtf8("Dialog"));
        Dialog->resize(443, 281);
        verticalLayout = new QVBoxLayout(Dialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        group_remote_setup = new QGroupBox(Dialog);
        group_remote_setup->setObjectName(QString::fromUtf8("group_remote_setup"));
        gridLayout = new QGridLayout(group_remote_setup);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_4 = new QLabel(group_remote_setup);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_4, 4, 1, 1, 1);

        label_10 = new QLabel(group_remote_setup);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_10, 0, 1, 1, 1);

        sb_udp_port = new QSpinBox(group_remote_setup);
        sb_udp_port->setObjectName(QString::fromUtf8("sb_udp_port"));
        sb_udp_port->setMinimum(1024);
        sb_udp_port->setMaximum(32767);

        gridLayout->addWidget(sb_udp_port, 4, 2, 1, 1);

        sb_tcp_port = new QSpinBox(group_remote_setup);
        sb_tcp_port->setObjectName(QString::fromUtf8("sb_tcp_port"));
        sb_tcp_port->setMinimum(1024);
        sb_tcp_port->setMaximum(32767);

        gridLayout->addWidget(sb_tcp_port, 2, 2, 1, 1);

        label_3 = new QLabel(group_remote_setup);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_3, 2, 1, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(20, 60, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_4, 0, 0, 6, 1);

        le_host = new QLineEdit(group_remote_setup);
        le_host->setObjectName(QString::fromUtf8("le_host"));

        gridLayout->addWidget(le_host, 0, 2, 1, 2);

        horizontalSpacer_3 = new QSpacerItem(20, 60, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_3, 0, 4, 6, 1);


        verticalLayout->addWidget(group_remote_setup);

        group_reported_host = new QGroupBox(Dialog);
        group_reported_host->setObjectName(QString::fromUtf8("group_reported_host"));
        gridLayout_2 = new QGridLayout(group_reported_host);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        horizontalSpacer_10 = new QSpacerItem(92, 38, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_10, 0, 0, 2, 1);

        rb_reported_auto = new QRadioButton(group_reported_host);
        rb_reported_auto->setObjectName(QString::fromUtf8("rb_reported_auto"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(rb_reported_auto->sizePolicy().hasHeightForWidth());
        rb_reported_auto->setSizePolicy(sizePolicy);

        gridLayout_2->addWidget(rb_reported_auto, 0, 1, 1, 2);

        horizontalSpacer_9 = new QSpacerItem(92, 38, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_9, 0, 3, 2, 1);

        rb_reported_custom = new QRadioButton(group_reported_host);
        rb_reported_custom->setObjectName(QString::fromUtf8("rb_reported_custom"));
        sizePolicy.setHeightForWidth(rb_reported_custom->sizePolicy().hasHeightForWidth());
        rb_reported_custom->setSizePolicy(sizePolicy);

        gridLayout_2->addWidget(rb_reported_custom, 1, 1, 1, 1);

        le_reported_host = new QLineEdit(group_reported_host);
        le_reported_host->setObjectName(QString::fromUtf8("le_reported_host"));

        gridLayout_2->addWidget(le_reported_host, 1, 2, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        te_reported_hint = new QPlainTextEdit(group_reported_host);
        te_reported_hint->setObjectName(QString::fromUtf8("te_reported_hint"));
        te_reported_hint->setFrameShape(QFrame::NoFrame);
        te_reported_hint->setFrameShadow(QFrame::Plain);
        te_reported_hint->setLineWidth(0);
        te_reported_hint->setMidLineWidth(1);
        te_reported_hint->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        te_reported_hint->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        te_reported_hint->setUndoRedoEnabled(false);
        te_reported_hint->setTextInteractionFlags(Qt::NoTextInteraction);

        horizontalLayout->addWidget(te_reported_hint);

        horizontalSpacer_2 = new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        gridLayout_2->addLayout(horizontalLayout, 2, 0, 1, 4);


        verticalLayout->addWidget(group_reported_host);

        buttonBox = new QDialogButtonBox(Dialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(Dialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), Dialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), Dialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QCoreApplication::translate("Dialog", "Carla Control - Connect", nullptr));
        group_remote_setup->setTitle(QCoreApplication::translate("Dialog", "Remote setup", nullptr));
        label_4->setText(QCoreApplication::translate("Dialog", "UDP Port:", nullptr));
        label_10->setText(QCoreApplication::translate("Dialog", "Remote host:", nullptr));
        label_3->setText(QCoreApplication::translate("Dialog", "TCP Port:", nullptr));
        group_reported_host->setTitle(QCoreApplication::translate("Dialog", "Reported host", nullptr));
        rb_reported_auto->setText(QCoreApplication::translate("Dialog", "Automatic", nullptr));
        rb_reported_custom->setText(QCoreApplication::translate("Dialog", "Custom:", nullptr));
        te_reported_hint->setPlainText(QCoreApplication::translate("Dialog", "In some networks (like USB connections), the remote system cannot reach the local network. You can specify here which hostname or IP to make the remote Carla connect to.\n"
"If you are unsure, leave it as 'Automatic'.", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_OSC_CONNECT_H
