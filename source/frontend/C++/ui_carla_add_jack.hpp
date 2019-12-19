/********************************************************************************
** Form generated from reading UI file 'carla_add_jack.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_ADD_JACK_H
#define UI_CARLA_ADD_JACK_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label_9;
    QGroupBox *group_command;
    QGridLayout *gridLayout;
    QSpacerItem *horizontalSpacer_4;
    QLabel *label_10;
    QLineEdit *le_name;
    QSpacerItem *horizontalSpacer_3;
    QLabel *label_5;
    QRadioButton *rb_template;
    QRadioButton *rb_custom;
    QStackedWidget *stackedWidget;
    QWidget *page_template;
    QHBoxLayout *horizontalLayout_2;
    QLabel *l_template;
    QComboBox *cb_template;
    QWidget *page_command;
    QHBoxLayout *horizontalLayout_3;
    QLabel *l_command;
    QLineEdit *le_command;
    QGroupBox *group_setup;
    QGridLayout *gridLayout_2;
    QSpacerItem *horizontalSpacer_5;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_10;
    QLabel *label_6;
    QComboBox *cb_session_mgr;
    QSpacerItem *horizontalSpacer_9;
    QLabel *label_3;
    QSpinBox *sb_audio_ins;
    QSpacerItem *horizontalSpacer_7;
    QFrame *line;
    QSpacerItem *horizontalSpacer_13;
    QLabel *label_7;
    QSpinBox *sb_midi_ins;
    QSpacerItem *horizontalSpacer_15;
    QSpacerItem *horizontalSpacer_6;
    QLabel *label_4;
    QSpinBox *sb_audio_outs;
    QSpacerItem *horizontalSpacer_8;
    QSpacerItem *horizontalSpacer_14;
    QLabel *label_8;
    QSpinBox *sb_midi_outs;
    QSpacerItem *horizontalSpacer_16;
    QCheckBox *cb_manage_window;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_3;
    QCheckBox *cb_external_start;
    QCheckBox *cb_capture_first_window;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *horizontalSpacer_2;
    QCheckBox *cb_buffers_addition_mode;
    QCheckBox *cb_out_midi_mixdown;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QString::fromUtf8("Dialog"));
        Dialog->resize(487, 572);
        verticalLayout = new QVBoxLayout(Dialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label_9 = new QLabel(Dialog);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        verticalLayout->addWidget(label_9);

        group_command = new QGroupBox(Dialog);
        group_command->setObjectName(QString::fromUtf8("group_command"));
        gridLayout = new QGridLayout(group_command);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        horizontalSpacer_4 = new QSpacerItem(20, 60, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_4, 0, 0, 3, 1);

        label_10 = new QLabel(group_command);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_10, 0, 1, 1, 1);

        le_name = new QLineEdit(group_command);
        le_name->setObjectName(QString::fromUtf8("le_name"));

        gridLayout->addWidget(le_name, 0, 2, 1, 2);

        horizontalSpacer_3 = new QSpacerItem(20, 60, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_3, 0, 4, 3, 1);

        label_5 = new QLabel(group_command);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_5, 1, 1, 1, 1);

        rb_template = new QRadioButton(group_command);
        rb_template->setObjectName(QString::fromUtf8("rb_template"));
        rb_template->setEnabled(false);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(rb_template->sizePolicy().hasHeightForWidth());
        rb_template->setSizePolicy(sizePolicy);

        gridLayout->addWidget(rb_template, 1, 2, 1, 1);

        rb_custom = new QRadioButton(group_command);
        rb_custom->setObjectName(QString::fromUtf8("rb_custom"));
        sizePolicy.setHeightForWidth(rb_custom->sizePolicy().hasHeightForWidth());
        rb_custom->setSizePolicy(sizePolicy);
        rb_custom->setChecked(true);

        gridLayout->addWidget(rb_custom, 1, 3, 1, 1);

        stackedWidget = new QStackedWidget(group_command);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        page_template = new QWidget();
        page_template->setObjectName(QString::fromUtf8("page_template"));
        horizontalLayout_2 = new QHBoxLayout(page_template);
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        l_template = new QLabel(page_template);
        l_template->setObjectName(QString::fromUtf8("l_template"));
        l_template->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_2->addWidget(l_template);

        cb_template = new QComboBox(page_template);
        cb_template->setObjectName(QString::fromUtf8("cb_template"));
        sizePolicy.setHeightForWidth(cb_template->sizePolicy().hasHeightForWidth());
        cb_template->setSizePolicy(sizePolicy);
        cb_template->setEditable(false);

        horizontalLayout_2->addWidget(cb_template);

        stackedWidget->addWidget(page_template);
        page_command = new QWidget();
        page_command->setObjectName(QString::fromUtf8("page_command"));
        horizontalLayout_3 = new QHBoxLayout(page_command);
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        l_command = new QLabel(page_command);
        l_command->setObjectName(QString::fromUtf8("l_command"));
        l_command->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_3->addWidget(l_command);

        le_command = new QLineEdit(page_command);
        le_command->setObjectName(QString::fromUtf8("le_command"));

        horizontalLayout_3->addWidget(le_command);

        stackedWidget->addWidget(page_command);

        gridLayout->addWidget(stackedWidget, 2, 1, 1, 3);


        verticalLayout->addWidget(group_command);

        group_setup = new QGroupBox(Dialog);
        group_setup->setObjectName(QString::fromUtf8("group_setup"));
        gridLayout_2 = new QGridLayout(group_setup);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        horizontalSpacer_5 = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_5, 2, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_10 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_10);

        label_6 = new QLabel(group_setup);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(label_6);

        cb_session_mgr = new QComboBox(group_setup);
        cb_session_mgr->addItem(QString());
        cb_session_mgr->addItem(QString::fromUtf8("LADISH (SIGUSR1)"));
        cb_session_mgr->addItem(QString::fromUtf8("NSM"));
        cb_session_mgr->setObjectName(QString::fromUtf8("cb_session_mgr"));

        horizontalLayout->addWidget(cb_session_mgr);

        horizontalSpacer_9 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_9);


        gridLayout_2->addLayout(horizontalLayout, 0, 1, 1, 8);

        label_3 = new QLabel(group_setup);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
        label_3->setSizePolicy(sizePolicy1);
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_3, 2, 1, 1, 1);

        sb_audio_ins = new QSpinBox(group_setup);
        sb_audio_ins->setObjectName(QString::fromUtf8("sb_audio_ins"));
        sb_audio_ins->setMaximum(64);

        gridLayout_2->addWidget(sb_audio_ins, 2, 2, 1, 1);

        horizontalSpacer_7 = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_7, 2, 3, 1, 1);

        line = new QFrame(group_setup);
        line->setObjectName(QString::fromUtf8("line"));
        line->setLineWidth(0);
        line->setMidLineWidth(1);
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Sunken);

        gridLayout_2->addWidget(line, 2, 4, 2, 1);

        horizontalSpacer_13 = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_13, 2, 5, 1, 1);

        label_7 = new QLabel(group_setup);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_7, 2, 6, 1, 1);

        sb_midi_ins = new QSpinBox(group_setup);
        sb_midi_ins->setObjectName(QString::fromUtf8("sb_midi_ins"));
        sb_midi_ins->setMaximum(1);

        gridLayout_2->addWidget(sb_midi_ins, 2, 7, 1, 1);

        horizontalSpacer_15 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_15, 2, 8, 1, 1);

        horizontalSpacer_6 = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_6, 3, 0, 1, 1);

        label_4 = new QLabel(group_setup);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        sizePolicy1.setHeightForWidth(label_4->sizePolicy().hasHeightForWidth());
        label_4->setSizePolicy(sizePolicy1);
        label_4->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_4, 3, 1, 1, 1);

        sb_audio_outs = new QSpinBox(group_setup);
        sb_audio_outs->setObjectName(QString::fromUtf8("sb_audio_outs"));
        sb_audio_outs->setMaximum(64);

        gridLayout_2->addWidget(sb_audio_outs, 3, 2, 1, 1);

        horizontalSpacer_8 = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_8, 3, 3, 1, 1);

        horizontalSpacer_14 = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_14, 3, 5, 1, 1);

        label_8 = new QLabel(group_setup);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_8, 3, 6, 1, 1);

        sb_midi_outs = new QSpinBox(group_setup);
        sb_midi_outs->setObjectName(QString::fromUtf8("sb_midi_outs"));
        sb_midi_outs->setMaximum(1);

        gridLayout_2->addWidget(sb_midi_outs, 3, 7, 1, 1);

        horizontalSpacer_16 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_16, 3, 8, 1, 1);

        cb_manage_window = new QCheckBox(group_setup);
        cb_manage_window->setObjectName(QString::fromUtf8("cb_manage_window"));

        gridLayout_2->addWidget(cb_manage_window, 1, 2, 1, 7);


        verticalLayout->addWidget(group_setup);

        groupBox = new QGroupBox(Dialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_3 = new QGridLayout(groupBox);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        cb_external_start = new QCheckBox(groupBox);
        cb_external_start->setObjectName(QString::fromUtf8("cb_external_start"));

        gridLayout_3->addWidget(cb_external_start, 4, 1, 1, 1);

        cb_capture_first_window = new QCheckBox(groupBox);
        cb_capture_first_window->setObjectName(QString::fromUtf8("cb_capture_first_window"));
        cb_capture_first_window->setEnabled(false);

        gridLayout_3->addWidget(cb_capture_first_window, 0, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_3->addItem(horizontalSpacer, 0, 0, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(135, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_2, 0, 2, 1, 1);

        cb_buffers_addition_mode = new QCheckBox(groupBox);
        cb_buffers_addition_mode->setObjectName(QString::fromUtf8("cb_buffers_addition_mode"));

        gridLayout_3->addWidget(cb_buffers_addition_mode, 2, 1, 1, 1);

        cb_out_midi_mixdown = new QCheckBox(groupBox);
        cb_out_midi_mixdown->setObjectName(QString::fromUtf8("cb_out_midi_mixdown"));

        gridLayout_3->addWidget(cb_out_midi_mixdown, 1, 1, 1, 1);


        verticalLayout->addWidget(groupBox);

        buttonBox = new QDialogButtonBox(Dialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(Dialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), Dialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), Dialog, SLOT(reject()));

        stackedWidget->setCurrentIndex(1);
        cb_session_mgr->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QCoreApplication::translate("Dialog", "Add JACK Application", nullptr));
        label_9->setText(QCoreApplication::translate("Dialog", "Note: Features not implemented yet are greyed out", nullptr));
        group_command->setTitle(QCoreApplication::translate("Dialog", "Application", nullptr));
        label_10->setText(QCoreApplication::translate("Dialog", "Name:", nullptr));
        label_5->setText(QCoreApplication::translate("Dialog", "Application:", nullptr));
        rb_template->setText(QCoreApplication::translate("Dialog", "From template", nullptr));
        rb_custom->setText(QCoreApplication::translate("Dialog", "Custom", nullptr));
        l_template->setText(QCoreApplication::translate("Dialog", "Template:", nullptr));
        l_command->setText(QCoreApplication::translate("Dialog", "Command:", nullptr));
        group_setup->setTitle(QCoreApplication::translate("Dialog", "Setup", nullptr));
        label_6->setText(QCoreApplication::translate("Dialog", "Session Manager:", nullptr));
        cb_session_mgr->setItemText(0, QCoreApplication::translate("Dialog", "None", nullptr));

        label_3->setText(QCoreApplication::translate("Dialog", "Audio inputs:", nullptr));
        label_7->setText(QCoreApplication::translate("Dialog", "MIDI inputs:", nullptr));
        label_4->setText(QCoreApplication::translate("Dialog", "Audio outputs:", nullptr));
        label_8->setText(QCoreApplication::translate("Dialog", "MIDI outputs:", nullptr));
        cb_manage_window->setText(QCoreApplication::translate("Dialog", "Take control of main application window", nullptr));
        groupBox->setTitle(QCoreApplication::translate("Dialog", "Workarounds", nullptr));
        cb_external_start->setText(QCoreApplication::translate("Dialog", "Wait for external application start (Advanced, for Debug only)", nullptr));
        cb_capture_first_window->setText(QCoreApplication::translate("Dialog", "Capture only the first X11 Window", nullptr));
        cb_buffers_addition_mode->setText(QCoreApplication::translate("Dialog", "Use previous client output buffer as input for the next client", nullptr));
        cb_out_midi_mixdown->setText(QCoreApplication::translate("Dialog", "Simulate 16 JACK MIDI outputs, with MIDI channel as port index", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_ADD_JACK_H
