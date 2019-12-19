/********************************************************************************
** Form generated from reading UI file 'carla_edit.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_EDIT_H
#define UI_CARLA_EDIT_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "widgets/pixmapdial.h"

QT_BEGIN_NAMESPACE

class Ui_PluginEdit
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *tabEdit;
    QGridLayout *gridLayout_2;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout_5;
    QHBoxLayout *horizontalLayout_5;
    QSpacerItem *horizontalSpacer_5;
    QLabel *label_ctrl_channel;
    QSpinBox *sb_ctrl_channel;
    QSpacerItem *horizontalSpacer_3;
    QHBoxLayout *horizontalLayout_8;
    QSpacerItem *horizontalSpacer_6;
    PixmapDial *dial_drywet;
    PixmapDial *dial_vol;
    QStackedWidget *stackedWidget;
    QWidget *page_bal;
    QHBoxLayout *horizontalLayout_6;
    PixmapDial *dial_b_left;
    PixmapDial *dial_b_right;
    QWidget *page_pan;
    QHBoxLayout *horizontalLayout_7;
    PixmapDial *dial_pan;
    QVBoxLayout *verticalLayout_2;
    QRadioButton *rb_balance;
    QRadioButton *rb_pan;
    QSpacerItem *horizontalSpacer_4;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_4;
    QCheckBox *ch_use_chunks;
    QFrame *line_2;
    QLabel *label;
    QCheckBox *ch_fixed_buffer;
    QCheckBox *ch_force_stereo;
    QFrame *line;
    QLabel *label_2;
    QCheckBox *ch_map_program_changes;
    QCheckBox *ch_send_program_changes;
    QCheckBox *ch_send_control_changes;
    QCheckBox *ch_send_channel_pressure;
    QCheckBox *ch_send_note_aftertouch;
    QCheckBox *ch_send_pitchbend;
    QCheckBox *ch_send_all_sound_off;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QLabel *label_plugin;
    QSpacerItem *horizontalSpacer_2;
    QGridLayout *gridLayout;
    QStackedWidget *sw_programs;
    QWidget *sww_programs;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_8;
    QLabel *label_programs;
    QComboBox *cb_programs;
    QWidget *sww_midiPrograms;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer_7;
    QLabel *label_midi_programs;
    QComboBox *cb_midi_programs;
    QPushButton *b_save_state;
    QPushButton *b_load_state;
    QSpacerItem *horizontalSpacer_9;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_3;
    QGridLayout *gridLayout_3;
    QLineEdit *le_maker;
    QLabel *label_label;
    QLabel *label_name;
    QLineEdit *le_name;
    QLineEdit *le_type;
    QLineEdit *le_copyright;
    QLineEdit *le_unique_id;
    QLabel *label_type;
    QLabel *label_maker;
    QLabel *label_copyright;
    QLabel *label_unique_id;
    QLineEdit *le_label;
    QSpacerItem *verticalSpacer;

    void setupUi(QDialog *PluginEdit)
    {
        if (PluginEdit->objectName().isEmpty())
            PluginEdit->setObjectName(QString::fromUtf8("PluginEdit"));
        PluginEdit->resize(537, 535);
        verticalLayout = new QVBoxLayout(PluginEdit);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        tabWidget = new QTabWidget(PluginEdit);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setTabPosition(QTabWidget::South);
        tabEdit = new QWidget();
        tabEdit->setObjectName(QString::fromUtf8("tabEdit"));
        gridLayout_2 = new QGridLayout(tabEdit);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        groupBox_3 = new QGroupBox(tabEdit);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        verticalLayout_5 = new QVBoxLayout(groupBox_3);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_5);

        label_ctrl_channel = new QLabel(groupBox_3);
        label_ctrl_channel->setObjectName(QString::fromUtf8("label_ctrl_channel"));

        horizontalLayout_5->addWidget(label_ctrl_channel);

        sb_ctrl_channel = new QSpinBox(groupBox_3);
        sb_ctrl_channel->setObjectName(QString::fromUtf8("sb_ctrl_channel"));
        sb_ctrl_channel->setContextMenuPolicy(Qt::CustomContextMenu);
        sb_ctrl_channel->setAlignment(Qt::AlignCenter);
        sb_ctrl_channel->setMinimum(0);
        sb_ctrl_channel->setMaximum(16);
        sb_ctrl_channel->setValue(0);

        horizontalLayout_5->addWidget(sb_ctrl_channel);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_3);


        verticalLayout_5->addLayout(horizontalLayout_5);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        horizontalSpacer_6 = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_6);

        dial_drywet = new PixmapDial(groupBox_3);
        dial_drywet->setObjectName(QString::fromUtf8("dial_drywet"));
        dial_drywet->setMinimumSize(QSize(34, 34));
        dial_drywet->setMaximumSize(QSize(34, 34));
        dial_drywet->setContextMenuPolicy(Qt::CustomContextMenu);

        horizontalLayout_8->addWidget(dial_drywet);

        dial_vol = new PixmapDial(groupBox_3);
        dial_vol->setObjectName(QString::fromUtf8("dial_vol"));
        dial_vol->setMinimumSize(QSize(34, 34));
        dial_vol->setMaximumSize(QSize(34, 34));
        dial_vol->setContextMenuPolicy(Qt::CustomContextMenu);

        horizontalLayout_8->addWidget(dial_vol);

        stackedWidget = new QStackedWidget(groupBox_3);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setMaximumSize(QSize(16777215, 42));
        stackedWidget->setLineWidth(0);
        page_bal = new QWidget();
        page_bal->setObjectName(QString::fromUtf8("page_bal"));
        horizontalLayout_6 = new QHBoxLayout(page_bal);
        horizontalLayout_6->setSpacing(0);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        horizontalLayout_6->setContentsMargins(0, 0, 0, 0);
        dial_b_left = new PixmapDial(page_bal);
        dial_b_left->setObjectName(QString::fromUtf8("dial_b_left"));
        dial_b_left->setMinimumSize(QSize(26, 26));
        dial_b_left->setMaximumSize(QSize(26, 26));
        dial_b_left->setContextMenuPolicy(Qt::CustomContextMenu);

        horizontalLayout_6->addWidget(dial_b_left);

        dial_b_right = new PixmapDial(page_bal);
        dial_b_right->setObjectName(QString::fromUtf8("dial_b_right"));
        dial_b_right->setMinimumSize(QSize(26, 26));
        dial_b_right->setMaximumSize(QSize(26, 26));
        dial_b_right->setContextMenuPolicy(Qt::CustomContextMenu);

        horizontalLayout_6->addWidget(dial_b_right);

        stackedWidget->addWidget(page_bal);
        page_pan = new QWidget();
        page_pan->setObjectName(QString::fromUtf8("page_pan"));
        horizontalLayout_7 = new QHBoxLayout(page_pan);
        horizontalLayout_7->setSpacing(0);
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        horizontalLayout_7->setContentsMargins(0, 0, 0, 0);
        dial_pan = new PixmapDial(page_pan);
        dial_pan->setObjectName(QString::fromUtf8("dial_pan"));
        dial_pan->setMinimumSize(QSize(26, 26));
        dial_pan->setMaximumSize(QSize(26, 26));
        dial_pan->setContextMenuPolicy(Qt::CustomContextMenu);

        horizontalLayout_7->addWidget(dial_pan);

        stackedWidget->addWidget(page_pan);

        horizontalLayout_8->addWidget(stackedWidget);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        rb_balance = new QRadioButton(groupBox_3);
        rb_balance->setObjectName(QString::fromUtf8("rb_balance"));
        rb_balance->setChecked(true);

        verticalLayout_2->addWidget(rb_balance);

        rb_pan = new QRadioButton(groupBox_3);
        rb_pan->setObjectName(QString::fromUtf8("rb_pan"));

        verticalLayout_2->addWidget(rb_pan);


        horizontalLayout_8->addLayout(verticalLayout_2);

        horizontalSpacer_4 = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_4);


        verticalLayout_5->addLayout(horizontalLayout_8);


        gridLayout_2->addWidget(groupBox_3, 2, 0, 1, 1);

        groupBox = new QGroupBox(tabEdit);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy);
        verticalLayout_4 = new QVBoxLayout(groupBox);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        ch_use_chunks = new QCheckBox(groupBox);
        ch_use_chunks->setObjectName(QString::fromUtf8("ch_use_chunks"));

        verticalLayout_4->addWidget(ch_use_chunks);

        line_2 = new QFrame(groupBox);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setLineWidth(0);
        line_2->setMidLineWidth(1);
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);

        verticalLayout_4->addWidget(line_2);

        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        label->setFont(font);

        verticalLayout_4->addWidget(label);

        ch_fixed_buffer = new QCheckBox(groupBox);
        ch_fixed_buffer->setObjectName(QString::fromUtf8("ch_fixed_buffer"));

        verticalLayout_4->addWidget(ch_fixed_buffer);

        ch_force_stereo = new QCheckBox(groupBox);
        ch_force_stereo->setObjectName(QString::fromUtf8("ch_force_stereo"));

        verticalLayout_4->addWidget(ch_force_stereo);

        line = new QFrame(groupBox);
        line->setObjectName(QString::fromUtf8("line"));
        line->setLineWidth(0);
        line->setMidLineWidth(1);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout_4->addWidget(line);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setFont(font);

        verticalLayout_4->addWidget(label_2);

        ch_map_program_changes = new QCheckBox(groupBox);
        ch_map_program_changes->setObjectName(QString::fromUtf8("ch_map_program_changes"));

        verticalLayout_4->addWidget(ch_map_program_changes);

        ch_send_program_changes = new QCheckBox(groupBox);
        ch_send_program_changes->setObjectName(QString::fromUtf8("ch_send_program_changes"));

        verticalLayout_4->addWidget(ch_send_program_changes);

        ch_send_control_changes = new QCheckBox(groupBox);
        ch_send_control_changes->setObjectName(QString::fromUtf8("ch_send_control_changes"));

        verticalLayout_4->addWidget(ch_send_control_changes);

        ch_send_channel_pressure = new QCheckBox(groupBox);
        ch_send_channel_pressure->setObjectName(QString::fromUtf8("ch_send_channel_pressure"));

        verticalLayout_4->addWidget(ch_send_channel_pressure);

        ch_send_note_aftertouch = new QCheckBox(groupBox);
        ch_send_note_aftertouch->setObjectName(QString::fromUtf8("ch_send_note_aftertouch"));

        verticalLayout_4->addWidget(ch_send_note_aftertouch);

        ch_send_pitchbend = new QCheckBox(groupBox);
        ch_send_pitchbend->setObjectName(QString::fromUtf8("ch_send_pitchbend"));

        verticalLayout_4->addWidget(ch_send_pitchbend);

        ch_send_all_sound_off = new QCheckBox(groupBox);
        ch_send_all_sound_off->setObjectName(QString::fromUtf8("ch_send_all_sound_off"));

        verticalLayout_4->addWidget(ch_send_all_sound_off);


        gridLayout_2->addWidget(groupBox, 1, 1, 2, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        label_plugin = new QLabel(tabEdit);
        label_plugin->setObjectName(QString::fromUtf8("label_plugin"));
        label_plugin->setAlignment(Qt::AlignCenter);
        label_plugin->setWordWrap(true);

        horizontalLayout->addWidget(label_plugin);

        horizontalSpacer_2 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        sw_programs = new QStackedWidget(tabEdit);
        sw_programs->setObjectName(QString::fromUtf8("sw_programs"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sw_programs->sizePolicy().hasHeightForWidth());
        sw_programs->setSizePolicy(sizePolicy1);
        sw_programs->setLineWidth(0);
        sww_programs = new QWidget();
        sww_programs->setObjectName(QString::fromUtf8("sww_programs"));
        horizontalLayout_3 = new QHBoxLayout(sww_programs);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(2, 2, 2, 2);
        horizontalSpacer_8 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_8);

        label_programs = new QLabel(sww_programs);
        label_programs->setObjectName(QString::fromUtf8("label_programs"));
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(label_programs->sizePolicy().hasHeightForWidth());
        label_programs->setSizePolicy(sizePolicy2);

        horizontalLayout_3->addWidget(label_programs);

        cb_programs = new QComboBox(sww_programs);
        cb_programs->setObjectName(QString::fromUtf8("cb_programs"));
        cb_programs->setMinimumSize(QSize(150, 0));

        horizontalLayout_3->addWidget(cb_programs);

        sw_programs->addWidget(sww_programs);
        sww_midiPrograms = new QWidget();
        sww_midiPrograms->setObjectName(QString::fromUtf8("sww_midiPrograms"));
        horizontalLayout_4 = new QHBoxLayout(sww_midiPrograms);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(2, 2, 2, 2);
        horizontalSpacer_7 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_7);

        label_midi_programs = new QLabel(sww_midiPrograms);
        label_midi_programs->setObjectName(QString::fromUtf8("label_midi_programs"));
        sizePolicy2.setHeightForWidth(label_midi_programs->sizePolicy().hasHeightForWidth());
        label_midi_programs->setSizePolicy(sizePolicy2);

        horizontalLayout_4->addWidget(label_midi_programs);

        cb_midi_programs = new QComboBox(sww_midiPrograms);
        cb_midi_programs->setObjectName(QString::fromUtf8("cb_midi_programs"));
        cb_midi_programs->setMinimumSize(QSize(150, 0));

        horizontalLayout_4->addWidget(cb_midi_programs);

        sw_programs->addWidget(sww_midiPrograms);

        gridLayout->addWidget(sw_programs, 0, 1, 1, 2);

        b_save_state = new QPushButton(tabEdit);
        b_save_state->setObjectName(QString::fromUtf8("b_save_state"));
        b_save_state->setFocusPolicy(Qt::NoFocus);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/16x16/document-save.png"), QSize(), QIcon::Normal, QIcon::Off);
        b_save_state->setIcon(icon);

        gridLayout->addWidget(b_save_state, 1, 1, 1, 1);

        b_load_state = new QPushButton(tabEdit);
        b_load_state->setObjectName(QString::fromUtf8("b_load_state"));
        b_load_state->setFocusPolicy(Qt::NoFocus);
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/16x16/document-open.png"), QSize(), QIcon::Normal, QIcon::Off);
        b_load_state->setIcon(icon1);

        gridLayout->addWidget(b_load_state, 1, 2, 1, 1);

        horizontalSpacer_9 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_9, 1, 0, 1, 1);


        horizontalLayout->addLayout(gridLayout);


        gridLayout_2->addLayout(horizontalLayout, 0, 0, 1, 2);

        groupBox_2 = new QGroupBox(tabEdit);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        sizePolicy.setHeightForWidth(groupBox_2->sizePolicy().hasHeightForWidth());
        groupBox_2->setSizePolicy(sizePolicy);
        verticalLayout_3 = new QVBoxLayout(groupBox_2);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        le_maker = new QLineEdit(groupBox_2);
        le_maker->setObjectName(QString::fromUtf8("le_maker"));
        le_maker->setInputMask(QString::fromUtf8(""));
        le_maker->setText(QString::fromUtf8(""));
        le_maker->setFrame(false);
        le_maker->setReadOnly(true);

        gridLayout_3->addWidget(le_maker, 3, 1, 1, 1);

        label_label = new QLabel(groupBox_2);
        label_label->setObjectName(QString::fromUtf8("label_label"));
        label_label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_3->addWidget(label_label, 2, 0, 1, 1);

        label_name = new QLabel(groupBox_2);
        label_name->setObjectName(QString::fromUtf8("label_name"));
        label_name->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_3->addWidget(label_name, 1, 0, 1, 1);

        le_name = new QLineEdit(groupBox_2);
        le_name->setObjectName(QString::fromUtf8("le_name"));
        le_name->setInputMask(QString::fromUtf8(""));
        le_name->setText(QString::fromUtf8(""));
        le_name->setFrame(false);
        le_name->setReadOnly(true);

        gridLayout_3->addWidget(le_name, 1, 1, 1, 1);

        le_type = new QLineEdit(groupBox_2);
        le_type->setObjectName(QString::fromUtf8("le_type"));
        le_type->setInputMask(QString::fromUtf8(""));
        le_type->setText(QString::fromUtf8(""));
        le_type->setFrame(false);
        le_type->setReadOnly(true);

        gridLayout_3->addWidget(le_type, 0, 1, 1, 1);

        le_copyright = new QLineEdit(groupBox_2);
        le_copyright->setObjectName(QString::fromUtf8("le_copyright"));
        le_copyright->setInputMask(QString::fromUtf8(""));
        le_copyright->setText(QString::fromUtf8(""));
        le_copyright->setFrame(false);
        le_copyright->setReadOnly(true);

        gridLayout_3->addWidget(le_copyright, 4, 1, 1, 1);

        le_unique_id = new QLineEdit(groupBox_2);
        le_unique_id->setObjectName(QString::fromUtf8("le_unique_id"));
        le_unique_id->setInputMask(QString::fromUtf8(""));
        le_unique_id->setText(QString::fromUtf8(""));
        le_unique_id->setFrame(false);
        le_unique_id->setReadOnly(true);

        gridLayout_3->addWidget(le_unique_id, 5, 1, 1, 1);

        label_type = new QLabel(groupBox_2);
        label_type->setObjectName(QString::fromUtf8("label_type"));
        label_type->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_3->addWidget(label_type, 0, 0, 1, 1);

        label_maker = new QLabel(groupBox_2);
        label_maker->setObjectName(QString::fromUtf8("label_maker"));
        label_maker->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_3->addWidget(label_maker, 3, 0, 1, 1);

        label_copyright = new QLabel(groupBox_2);
        label_copyright->setObjectName(QString::fromUtf8("label_copyright"));
        label_copyright->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_3->addWidget(label_copyright, 4, 0, 1, 1);

        label_unique_id = new QLabel(groupBox_2);
        label_unique_id->setObjectName(QString::fromUtf8("label_unique_id"));
        label_unique_id->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_3->addWidget(label_unique_id, 5, 0, 1, 1);

        le_label = new QLineEdit(groupBox_2);
        le_label->setObjectName(QString::fromUtf8("le_label"));
        le_label->setInputMask(QString::fromUtf8(""));
        le_label->setText(QString::fromUtf8(""));
        le_label->setFrame(false);
        le_label->setReadOnly(true);

        gridLayout_3->addWidget(le_label, 2, 1, 1, 1);


        verticalLayout_3->addLayout(gridLayout_3);


        gridLayout_2->addWidget(groupBox_2, 1, 0, 1, 1);

        verticalSpacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        gridLayout_2->addItem(verticalSpacer, 3, 0, 1, 2);

        tabWidget->addTab(tabEdit, QString());

        verticalLayout->addWidget(tabWidget);


        retranslateUi(PluginEdit);

        tabWidget->setCurrentIndex(0);
        stackedWidget->setCurrentIndex(0);
        sw_programs->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(PluginEdit);
    } // setupUi

    void retranslateUi(QDialog *PluginEdit)
    {
        PluginEdit->setWindowTitle(QCoreApplication::translate("PluginEdit", "Plugin Editor", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("PluginEdit", "Control", nullptr));
        label_ctrl_channel->setText(QCoreApplication::translate("PluginEdit", "MIDI Control Channel:", nullptr));
        sb_ctrl_channel->setSpecialValueText(QCoreApplication::translate("PluginEdit", "N", nullptr));
#if QT_CONFIG(statustip)
        dial_drywet->setStatusTip(QCoreApplication::translate("PluginEdit", "Output dry/wet (100%)", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(statustip)
        dial_vol->setStatusTip(QCoreApplication::translate("PluginEdit", "Output volume (100%)", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(statustip)
        dial_b_left->setStatusTip(QCoreApplication::translate("PluginEdit", "Balance Left (0%)", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(statustip)
        dial_b_right->setStatusTip(QCoreApplication::translate("PluginEdit", "Balance Right (0%)", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(statustip)
        dial_pan->setStatusTip(QCoreApplication::translate("PluginEdit", "Balance Right (0%)", nullptr));
#endif // QT_CONFIG(statustip)
        rb_balance->setText(QCoreApplication::translate("PluginEdit", "Use Balance", nullptr));
        rb_pan->setText(QCoreApplication::translate("PluginEdit", "Use Panning", nullptr));
        groupBox->setTitle(QCoreApplication::translate("PluginEdit", "Settings", nullptr));
        ch_use_chunks->setText(QCoreApplication::translate("PluginEdit", "Use Chunks", nullptr));
        label->setText(QCoreApplication::translate("PluginEdit", "    Audio:", nullptr));
        ch_fixed_buffer->setText(QCoreApplication::translate("PluginEdit", "Fixed-Size Buffer", nullptr));
        ch_force_stereo->setText(QCoreApplication::translate("PluginEdit", "Force Stereo (needs reload)", nullptr));
        label_2->setText(QCoreApplication::translate("PluginEdit", "    MIDI:", nullptr));
        ch_map_program_changes->setText(QCoreApplication::translate("PluginEdit", "Map Program Changes", nullptr));
        ch_send_program_changes->setText(QCoreApplication::translate("PluginEdit", "Send Bank/Program Changes", nullptr));
        ch_send_control_changes->setText(QCoreApplication::translate("PluginEdit", "Send Control Changes", nullptr));
        ch_send_channel_pressure->setText(QCoreApplication::translate("PluginEdit", "Send Channel Pressure", nullptr));
        ch_send_note_aftertouch->setText(QCoreApplication::translate("PluginEdit", "Send Note Aftertouch", nullptr));
        ch_send_pitchbend->setText(QCoreApplication::translate("PluginEdit", "Send Pitchbend", nullptr));
        ch_send_all_sound_off->setText(QCoreApplication::translate("PluginEdit", "Send All Sound/Notes Off", nullptr));
        label_plugin->setText(QCoreApplication::translate("PluginEdit", "\n"
"Plugin Name\n"
"", nullptr));
        label_programs->setText(QCoreApplication::translate("PluginEdit", "Program:", nullptr));
        label_midi_programs->setText(QCoreApplication::translate("PluginEdit", "MIDI Program:", nullptr));
        b_save_state->setText(QCoreApplication::translate("PluginEdit", "Save State", nullptr));
        b_load_state->setText(QCoreApplication::translate("PluginEdit", "Load State", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("PluginEdit", "Information", nullptr));
        label_label->setText(QCoreApplication::translate("PluginEdit", "Label/URI:", nullptr));
        label_name->setText(QCoreApplication::translate("PluginEdit", "Name:", nullptr));
        label_type->setText(QCoreApplication::translate("PluginEdit", "Type:", nullptr));
        label_maker->setText(QCoreApplication::translate("PluginEdit", "Maker:", nullptr));
        label_copyright->setText(QCoreApplication::translate("PluginEdit", "Copyright:", nullptr));
        label_unique_id->setText(QCoreApplication::translate("PluginEdit", "Unique ID:", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabEdit), QCoreApplication::translate("PluginEdit", "Edit", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PluginEdit: public Ui_PluginEdit {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_EDIT_H
