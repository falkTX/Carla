/********************************************************************************
** Form generated from reading UI file 'carla_database.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_DATABASE_H
#define UI_CARLA_DATABASE_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PluginDatabaseW
{
public:
    QAction *act_focus_search;
    QGridLayout *gridLayout_3;
    QToolBox *toolBox;
    QWidget *p_format;
    QVBoxLayout *verticalLayout;
    QCheckBox *ch_internal;
    QCheckBox *ch_ladspa;
    QCheckBox *ch_dssi;
    QCheckBox *ch_lv2;
    QCheckBox *ch_vst;
    QCheckBox *ch_vst3;
    QCheckBox *ch_au;
    QCheckBox *ch_kits;
    QSpacerItem *verticalSpacer_5;
    QWidget *p_type;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *ch_effects;
    QCheckBox *ch_instruments;
    QCheckBox *ch_midi;
    QCheckBox *ch_other;
    QSpacerItem *verticalSpacer_3;
    QWidget *p_arch;
    QVBoxLayout *verticalLayout_3;
    QCheckBox *ch_native;
    QCheckBox *ch_bridged;
    QCheckBox *ch_bridged_wine;
    QSpacerItem *verticalSpacer_4;
    QSpacerItem *verticalSpacer_6;
    QTabWidget *tab_reqs;
    QWidget *tw_reqs;
    QGridLayout *gridLayout;
    QLabel *l_reqs;
    QCheckBox *ch_gui;
    QCheckBox *ch_cv;
    QCheckBox *ch_rtsafe;
    QCheckBox *ch_stereo;
    QCheckBox *ch_inline_display;
    QCheckBox *ch_favorites;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    QSpacerItem *horizontalSpacer;
    QPushButton *b_add;
    QPushButton *b_cancel;
    QHBoxLayout *horizontalLayout;
    QLineEdit *lineEdit;
    QPushButton *b_refresh;
    QPushButton *b_clear_filters;
    QTabWidget *tab_info;
    QWidget *tw_info;
    QGridLayout *gridLayout_2;
    QSpacerItem *verticalSpacer_2;
    QLabel *l_format;
    QLabel *label_2;
    QLabel *label_4;
    QLabel *l_arch;
    QLabel *label_6;
    QLabel *l_type;
    QSpacerItem *verticalSpacer;
    QLabel *label_12;
    QLabel *label_8;
    QLabel *label_11;
    QLabel *label_13;
    QLabel *label_14;
    QLabel *label_15;
    QLabel *label_9;
    QLabel *label_10;
    QLabel *la_id;
    QLabel *label_18;
    QLabel *label_17;
    QLabel *label_19;
    QLabel *label_20;
    QLabel *l_aouts;
    QLabel *l_id;
    QLabel *l_ains;
    QLabel *l_synth;
    QLabel *l_pins;
    QLabel *l_cvins;
    QLabel *l_pouts;
    QLabel *l_mouts;
    QLabel *l_mins;
    QLabel *l_bridged;
    QLabel *l_idisp;
    QLabel *l_gui;
    QLabel *l_cvouts;
    QLabel *label_3;
    QFrame *line_2;
    QFrame *line;
    QTableWidget *tableWidget;

    void setupUi(QDialog *PluginDatabaseW)
    {
        if (PluginDatabaseW->objectName().isEmpty())
            PluginDatabaseW->setObjectName(QString::fromUtf8("PluginDatabaseW"));
        PluginDatabaseW->resize(1100, 683);
        act_focus_search = new QAction(PluginDatabaseW);
        act_focus_search->setObjectName(QString::fromUtf8("act_focus_search"));
        gridLayout_3 = new QGridLayout(PluginDatabaseW);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        toolBox = new QToolBox(PluginDatabaseW);
        toolBox->setObjectName(QString::fromUtf8("toolBox"));
        p_format = new QWidget();
        p_format->setObjectName(QString::fromUtf8("p_format"));
        p_format->setGeometry(QRect(0, 0, 164, 254));
        verticalLayout = new QVBoxLayout(p_format);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        ch_internal = new QCheckBox(p_format);
        ch_internal->setObjectName(QString::fromUtf8("ch_internal"));

        verticalLayout->addWidget(ch_internal);

        ch_ladspa = new QCheckBox(p_format);
        ch_ladspa->setObjectName(QString::fromUtf8("ch_ladspa"));

        verticalLayout->addWidget(ch_ladspa);

        ch_dssi = new QCheckBox(p_format);
        ch_dssi->setObjectName(QString::fromUtf8("ch_dssi"));

        verticalLayout->addWidget(ch_dssi);

        ch_lv2 = new QCheckBox(p_format);
        ch_lv2->setObjectName(QString::fromUtf8("ch_lv2"));

        verticalLayout->addWidget(ch_lv2);

        ch_vst = new QCheckBox(p_format);
        ch_vst->setObjectName(QString::fromUtf8("ch_vst"));

        verticalLayout->addWidget(ch_vst);

        ch_vst3 = new QCheckBox(p_format);
        ch_vst3->setObjectName(QString::fromUtf8("ch_vst3"));

        verticalLayout->addWidget(ch_vst3);

        ch_au = new QCheckBox(p_format);
        ch_au->setObjectName(QString::fromUtf8("ch_au"));

        verticalLayout->addWidget(ch_au);

        ch_kits = new QCheckBox(p_format);
        ch_kits->setObjectName(QString::fromUtf8("ch_kits"));

        verticalLayout->addWidget(ch_kits);

        verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_5);

        toolBox->addItem(p_format, QString::fromUtf8("Format"));
        p_type = new QWidget();
        p_type->setObjectName(QString::fromUtf8("p_type"));
        p_type->setGeometry(QRect(0, 0, 164, 164));
        verticalLayout_2 = new QVBoxLayout(p_type);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        ch_effects = new QCheckBox(p_type);
        ch_effects->setObjectName(QString::fromUtf8("ch_effects"));

        verticalLayout_2->addWidget(ch_effects);

        ch_instruments = new QCheckBox(p_type);
        ch_instruments->setObjectName(QString::fromUtf8("ch_instruments"));

        verticalLayout_2->addWidget(ch_instruments);

        ch_midi = new QCheckBox(p_type);
        ch_midi->setObjectName(QString::fromUtf8("ch_midi"));

        verticalLayout_2->addWidget(ch_midi);

        ch_other = new QCheckBox(p_type);
        ch_other->setObjectName(QString::fromUtf8("ch_other"));

        verticalLayout_2->addWidget(ch_other);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_3);

        toolBox->addItem(p_type, QString::fromUtf8("Type"));
        p_arch = new QWidget();
        p_arch->setObjectName(QString::fromUtf8("p_arch"));
        p_arch->setGeometry(QRect(0, 0, 164, 136));
        verticalLayout_3 = new QVBoxLayout(p_arch);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        ch_native = new QCheckBox(p_arch);
        ch_native->setObjectName(QString::fromUtf8("ch_native"));

        verticalLayout_3->addWidget(ch_native);

        ch_bridged = new QCheckBox(p_arch);
        ch_bridged->setObjectName(QString::fromUtf8("ch_bridged"));

        verticalLayout_3->addWidget(ch_bridged);

        ch_bridged_wine = new QCheckBox(p_arch);
        ch_bridged_wine->setObjectName(QString::fromUtf8("ch_bridged_wine"));

        verticalLayout_3->addWidget(ch_bridged_wine);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_4);

        toolBox->addItem(p_arch, QString::fromUtf8("Architecture"));

        gridLayout_3->addWidget(toolBox, 1, 0, 1, 1);

        verticalSpacer_6 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_3->addItem(verticalSpacer_6, 2, 0, 1, 1);

        tab_reqs = new QTabWidget(PluginDatabaseW);
        tab_reqs->setObjectName(QString::fromUtf8("tab_reqs"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(tab_reqs->sizePolicy().hasHeightForWidth());
        tab_reqs->setSizePolicy(sizePolicy);
        tab_reqs->setTabBarAutoHide(true);
        tw_reqs = new QWidget();
        tw_reqs->setObjectName(QString::fromUtf8("tw_reqs"));
        gridLayout = new QGridLayout(tw_reqs);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        l_reqs = new QLabel(tw_reqs);
        l_reqs->setObjectName(QString::fromUtf8("l_reqs"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        l_reqs->setFont(font);

        gridLayout->addWidget(l_reqs, 0, 1, 1, 1);

        ch_gui = new QCheckBox(tw_reqs);
        ch_gui->setObjectName(QString::fromUtf8("ch_gui"));

        gridLayout->addWidget(ch_gui, 5, 1, 1, 1);

        ch_cv = new QCheckBox(tw_reqs);
        ch_cv->setObjectName(QString::fromUtf8("ch_cv"));

        gridLayout->addWidget(ch_cv, 4, 1, 1, 1);

        ch_rtsafe = new QCheckBox(tw_reqs);
        ch_rtsafe->setObjectName(QString::fromUtf8("ch_rtsafe"));

        gridLayout->addWidget(ch_rtsafe, 2, 1, 1, 1);

        ch_stereo = new QCheckBox(tw_reqs);
        ch_stereo->setObjectName(QString::fromUtf8("ch_stereo"));

        gridLayout->addWidget(ch_stereo, 3, 1, 1, 1);

        ch_inline_display = new QCheckBox(tw_reqs);
        ch_inline_display->setObjectName(QString::fromUtf8("ch_inline_display"));

        gridLayout->addWidget(ch_inline_display, 6, 1, 1, 1);

        ch_favorites = new QCheckBox(tw_reqs);
        ch_favorites->setObjectName(QString::fromUtf8("ch_favorites"));

        gridLayout->addWidget(ch_favorites, 1, 1, 1, 1);

        tab_reqs->addTab(tw_reqs, QString());

        gridLayout_3->addWidget(tab_reqs, 3, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label = new QLabel(PluginDatabaseW);
        label->setObjectName(QString::fromUtf8("label"));
        label->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

        horizontalLayout_2->addWidget(label);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        b_add = new QPushButton(PluginDatabaseW);
        b_add->setObjectName(QString::fromUtf8("b_add"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/16x16/list-add.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_add->setIcon(icon);

        horizontalLayout_2->addWidget(b_add);

        b_cancel = new QPushButton(PluginDatabaseW);
        b_cancel->setObjectName(QString::fromUtf8("b_cancel"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/16x16/dialog-cancel.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_cancel->setIcon(icon1);

        horizontalLayout_2->addWidget(b_cancel);


        gridLayout_3->addLayout(horizontalLayout_2, 4, 0, 1, 3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        lineEdit = new QLineEdit(PluginDatabaseW);
        lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
        lineEdit->setClearButtonEnabled(true);

        horizontalLayout->addWidget(lineEdit);

        b_refresh = new QPushButton(PluginDatabaseW);
        b_refresh->setObjectName(QString::fromUtf8("b_refresh"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/16x16/view-refresh.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_refresh->setIcon(icon2);

        horizontalLayout->addWidget(b_refresh);

        b_clear_filters = new QPushButton(PluginDatabaseW);
        b_clear_filters->setObjectName(QString::fromUtf8("b_clear_filters"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/16x16/edit-clear.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_clear_filters->setIcon(icon3);

        horizontalLayout->addWidget(b_clear_filters);


        gridLayout_3->addLayout(horizontalLayout, 0, 0, 1, 3);

        tab_info = new QTabWidget(PluginDatabaseW);
        tab_info->setObjectName(QString::fromUtf8("tab_info"));
        tab_info->setTabBarAutoHide(true);
        tw_info = new QWidget();
        tw_info->setObjectName(QString::fromUtf8("tw_info"));
        gridLayout_2 = new QGridLayout(tw_info);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer_2, 19, 0, 1, 1);

        l_format = new QLabel(tw_info);
        l_format->setObjectName(QString::fromUtf8("l_format"));

        gridLayout_2->addWidget(l_format, 1, 1, 1, 1);

        label_2 = new QLabel(tw_info);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_2, 1, 0, 1, 1);

        label_4 = new QLabel(tw_info);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_4, 3, 0, 1, 1);

        l_arch = new QLabel(tw_info);
        l_arch->setObjectName(QString::fromUtf8("l_arch"));

        gridLayout_2->addWidget(l_arch, 3, 1, 1, 1);

        label_6 = new QLabel(tw_info);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_6, 2, 0, 1, 1);

        l_type = new QLabel(tw_info);
        l_type->setObjectName(QString::fromUtf8("l_type"));

        gridLayout_2->addWidget(l_type, 2, 1, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer, 19, 1, 1, 1);

        label_12 = new QLabel(tw_info);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_12, 10, 0, 1, 1);

        label_8 = new QLabel(tw_info);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_8, 6, 0, 1, 1);

        label_11 = new QLabel(tw_info);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_11, 9, 0, 1, 1);

        label_13 = new QLabel(tw_info);
        label_13->setObjectName(QString::fromUtf8("label_13"));
        label_13->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_13, 11, 0, 1, 1);

        label_14 = new QLabel(tw_info);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        label_14->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_14, 12, 0, 1, 1);

        label_15 = new QLabel(tw_info);
        label_15->setObjectName(QString::fromUtf8("label_15"));
        label_15->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_15, 13, 0, 1, 1);

        label_9 = new QLabel(tw_info);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_9, 7, 0, 1, 1);

        label_10 = new QLabel(tw_info);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_10, 8, 0, 1, 1);

        la_id = new QLabel(tw_info);
        la_id->setObjectName(QString::fromUtf8("la_id"));
        la_id->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(la_id, 4, 0, 1, 1);

        label_18 = new QLabel(tw_info);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        label_18->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_18, 16, 0, 1, 1);

        label_17 = new QLabel(tw_info);
        label_17->setObjectName(QString::fromUtf8("label_17"));
        label_17->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_17, 15, 0, 1, 1);

        label_19 = new QLabel(tw_info);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        label_19->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_19, 18, 0, 1, 1);

        label_20 = new QLabel(tw_info);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        label_20->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_20, 17, 0, 1, 1);

        l_aouts = new QLabel(tw_info);
        l_aouts->setObjectName(QString::fromUtf8("l_aouts"));

        gridLayout_2->addWidget(l_aouts, 7, 1, 1, 1);

        l_id = new QLabel(tw_info);
        l_id->setObjectName(QString::fromUtf8("l_id"));

        gridLayout_2->addWidget(l_id, 4, 1, 1, 1);

        l_ains = new QLabel(tw_info);
        l_ains->setObjectName(QString::fromUtf8("l_ains"));

        gridLayout_2->addWidget(l_ains, 6, 1, 1, 1);

        l_synth = new QLabel(tw_info);
        l_synth->setObjectName(QString::fromUtf8("l_synth"));

        gridLayout_2->addWidget(l_synth, 18, 1, 1, 1);

        l_pins = new QLabel(tw_info);
        l_pins->setObjectName(QString::fromUtf8("l_pins"));

        gridLayout_2->addWidget(l_pins, 12, 1, 1, 1);

        l_cvins = new QLabel(tw_info);
        l_cvins->setObjectName(QString::fromUtf8("l_cvins"));

        gridLayout_2->addWidget(l_cvins, 8, 1, 1, 1);

        l_pouts = new QLabel(tw_info);
        l_pouts->setObjectName(QString::fromUtf8("l_pouts"));

        gridLayout_2->addWidget(l_pouts, 13, 1, 1, 1);

        l_mouts = new QLabel(tw_info);
        l_mouts->setObjectName(QString::fromUtf8("l_mouts"));

        gridLayout_2->addWidget(l_mouts, 11, 1, 1, 1);

        l_mins = new QLabel(tw_info);
        l_mins->setObjectName(QString::fromUtf8("l_mins"));

        gridLayout_2->addWidget(l_mins, 10, 1, 1, 1);

        l_bridged = new QLabel(tw_info);
        l_bridged->setObjectName(QString::fromUtf8("l_bridged"));

        gridLayout_2->addWidget(l_bridged, 17, 1, 1, 1);

        l_idisp = new QLabel(tw_info);
        l_idisp->setObjectName(QString::fromUtf8("l_idisp"));

        gridLayout_2->addWidget(l_idisp, 16, 1, 1, 1);

        l_gui = new QLabel(tw_info);
        l_gui->setObjectName(QString::fromUtf8("l_gui"));

        gridLayout_2->addWidget(l_gui, 15, 1, 1, 1);

        l_cvouts = new QLabel(tw_info);
        l_cvouts->setObjectName(QString::fromUtf8("l_cvouts"));

        gridLayout_2->addWidget(l_cvouts, 9, 1, 1, 1);

        label_3 = new QLabel(tw_info);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setFont(font);

        gridLayout_2->addWidget(label_3, 0, 0, 1, 2);

        line_2 = new QFrame(tw_info);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setLineWidth(0);
        line_2->setMidLineWidth(1);
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);

        gridLayout_2->addWidget(line_2, 14, 0, 1, 2);

        line = new QFrame(tw_info);
        line->setObjectName(QString::fromUtf8("line"));
        line->setLineWidth(0);
        line->setMidLineWidth(1);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        gridLayout_2->addWidget(line, 5, 0, 1, 2);

        tab_info->addTab(tw_info, QString());

        gridLayout_3->addWidget(tab_info, 1, 2, 3, 1);

        tableWidget = new QTableWidget(PluginDatabaseW);
        if (tableWidget->columnCount() < 5)
            tableWidget->setColumnCount(5);
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/16x16/bookmarks.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        __qtablewidgetitem->setIcon(icon4);
        tableWidget->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        tableWidget->setObjectName(QString::fromUtf8("tableWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(tableWidget->sizePolicy().hasHeightForWidth());
        tableWidget->setSizePolicy(sizePolicy1);
        tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tableWidget->setProperty("showDropIndicator", QVariant(false));
        tableWidget->setDragDropOverwriteMode(false);
        tableWidget->setAlternatingRowColors(true);
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableWidget->setShowGrid(false);
        tableWidget->setGridStyle(Qt::NoPen);
        tableWidget->setSortingEnabled(true);
        tableWidget->setWordWrap(false);
        tableWidget->horizontalHeader()->setMinimumSectionSize(24);
        tableWidget->horizontalHeader()->setStretchLastSection(true);
        tableWidget->verticalHeader()->setVisible(false);
        tableWidget->verticalHeader()->setMinimumSectionSize(12);
        tableWidget->verticalHeader()->setDefaultSectionSize(22);

        gridLayout_3->addWidget(tableWidget, 1, 1, 3, 1);

        QWidget::setTabOrder(lineEdit, tableWidget);
        QWidget::setTabOrder(tableWidget, b_add);
        QWidget::setTabOrder(b_add, b_cancel);
        QWidget::setTabOrder(b_cancel, b_refresh);
        QWidget::setTabOrder(b_refresh, b_clear_filters);
        QWidget::setTabOrder(b_clear_filters, ch_internal);
        QWidget::setTabOrder(ch_internal, ch_ladspa);
        QWidget::setTabOrder(ch_ladspa, ch_dssi);
        QWidget::setTabOrder(ch_dssi, ch_lv2);
        QWidget::setTabOrder(ch_lv2, ch_vst);
        QWidget::setTabOrder(ch_vst, ch_vst3);
        QWidget::setTabOrder(ch_vst3, ch_au);
        QWidget::setTabOrder(ch_au, ch_kits);
        QWidget::setTabOrder(ch_kits, ch_effects);
        QWidget::setTabOrder(ch_effects, ch_instruments);
        QWidget::setTabOrder(ch_instruments, ch_midi);
        QWidget::setTabOrder(ch_midi, ch_other);
        QWidget::setTabOrder(ch_other, ch_native);
        QWidget::setTabOrder(ch_native, ch_bridged);
        QWidget::setTabOrder(ch_bridged, ch_bridged_wine);
        QWidget::setTabOrder(ch_bridged_wine, ch_inline_display);
        QWidget::setTabOrder(ch_inline_display, ch_stereo);
        QWidget::setTabOrder(ch_stereo, ch_rtsafe);
        QWidget::setTabOrder(ch_rtsafe, ch_gui);
        QWidget::setTabOrder(ch_gui, ch_cv);
        QWidget::setTabOrder(ch_cv, tab_reqs);
        QWidget::setTabOrder(tab_reqs, tab_info);

        retranslateUi(PluginDatabaseW);
        QObject::connect(act_focus_search, SIGNAL(triggered()), lineEdit, SLOT(setFocus()));

        toolBox->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(PluginDatabaseW);
    } // setupUi

    void retranslateUi(QDialog *PluginDatabaseW)
    {
        PluginDatabaseW->setWindowTitle(QCoreApplication::translate("PluginDatabaseW", "Carla - Add New", nullptr));
        act_focus_search->setText(QCoreApplication::translate("PluginDatabaseW", "Focus Text Search", nullptr));
#if QT_CONFIG(shortcut)
        act_focus_search->setShortcut(QCoreApplication::translate("PluginDatabaseW", "Ctrl+F", nullptr));
#endif // QT_CONFIG(shortcut)
        ch_internal->setText(QCoreApplication::translate("PluginDatabaseW", "Internal", nullptr));
        ch_ladspa->setText(QCoreApplication::translate("PluginDatabaseW", "LADSPA", nullptr));
        ch_dssi->setText(QCoreApplication::translate("PluginDatabaseW", "DSSI", nullptr));
        ch_lv2->setText(QCoreApplication::translate("PluginDatabaseW", "LV2", nullptr));
        ch_vst->setText(QCoreApplication::translate("PluginDatabaseW", "VST2", nullptr));
        ch_vst3->setText(QCoreApplication::translate("PluginDatabaseW", "VST3", nullptr));
        ch_au->setText(QCoreApplication::translate("PluginDatabaseW", "AU", nullptr));
        ch_kits->setText(QCoreApplication::translate("PluginDatabaseW", "Sound Kits", nullptr));
        toolBox->setItemText(toolBox->indexOf(p_format), QCoreApplication::translate("PluginDatabaseW", "Format", nullptr));
        ch_effects->setText(QCoreApplication::translate("PluginDatabaseW", "Effects", nullptr));
        ch_instruments->setText(QCoreApplication::translate("PluginDatabaseW", "Instruments", nullptr));
        ch_midi->setText(QCoreApplication::translate("PluginDatabaseW", "MIDI Plugins", nullptr));
        ch_other->setText(QCoreApplication::translate("PluginDatabaseW", "Other/Misc", nullptr));
        toolBox->setItemText(toolBox->indexOf(p_type), QCoreApplication::translate("PluginDatabaseW", "Type", nullptr));
        ch_native->setText(QCoreApplication::translate("PluginDatabaseW", "Native", nullptr));
        ch_bridged->setText(QCoreApplication::translate("PluginDatabaseW", "Bridged", nullptr));
        ch_bridged_wine->setText(QCoreApplication::translate("PluginDatabaseW", "Bridged (Wine)", nullptr));
        toolBox->setItemText(toolBox->indexOf(p_arch), QCoreApplication::translate("PluginDatabaseW", "Architecture", nullptr));
        l_reqs->setText(QCoreApplication::translate("PluginDatabaseW", "Requirements", nullptr));
        ch_gui->setText(QCoreApplication::translate("PluginDatabaseW", "With Custom GUI ", nullptr));
        ch_cv->setText(QCoreApplication::translate("PluginDatabaseW", "With CV Ports", nullptr));
        ch_rtsafe->setText(QCoreApplication::translate("PluginDatabaseW", "Real-time safe only", nullptr));
        ch_stereo->setText(QCoreApplication::translate("PluginDatabaseW", "Stereo only", nullptr));
        ch_inline_display->setText(QCoreApplication::translate("PluginDatabaseW", "With Inline Display", nullptr));
        ch_favorites->setText(QCoreApplication::translate("PluginDatabaseW", "Favorites only", nullptr));
        tab_reqs->setTabText(tab_reqs->indexOf(tw_reqs), QString());
        label->setText(QCoreApplication::translate("PluginDatabaseW", "(Number of Plugins go here)", nullptr));
        b_add->setText(QCoreApplication::translate("PluginDatabaseW", "&Add Plugin", nullptr));
        b_cancel->setText(QCoreApplication::translate("PluginDatabaseW", "Cancel", nullptr));
        b_refresh->setText(QCoreApplication::translate("PluginDatabaseW", "Refresh", nullptr));
        b_clear_filters->setText(QCoreApplication::translate("PluginDatabaseW", "Reset filters", nullptr));
        l_format->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        label_2->setText(QCoreApplication::translate("PluginDatabaseW", "Format:", nullptr));
        label_4->setText(QCoreApplication::translate("PluginDatabaseW", "Architecture:", nullptr));
        l_arch->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        label_6->setText(QCoreApplication::translate("PluginDatabaseW", "Type:", nullptr));
        l_type->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        label_12->setText(QCoreApplication::translate("PluginDatabaseW", "MIDI Ins:", nullptr));
        label_8->setText(QCoreApplication::translate("PluginDatabaseW", "Audio Ins:", nullptr));
        label_11->setText(QCoreApplication::translate("PluginDatabaseW", "CV Outs:", nullptr));
        label_13->setText(QCoreApplication::translate("PluginDatabaseW", "MIDI Outs:", nullptr));
        label_14->setText(QCoreApplication::translate("PluginDatabaseW", "Parameter Ins:", nullptr));
        label_15->setText(QCoreApplication::translate("PluginDatabaseW", "Parameter Outs:", nullptr));
        label_9->setText(QCoreApplication::translate("PluginDatabaseW", "Audio Outs:", nullptr));
        label_10->setText(QCoreApplication::translate("PluginDatabaseW", "CV Ins:", nullptr));
        la_id->setText(QCoreApplication::translate("PluginDatabaseW", "UniqueID:", nullptr));
        label_18->setText(QCoreApplication::translate("PluginDatabaseW", "Has Inline Display:", nullptr));
        label_17->setText(QCoreApplication::translate("PluginDatabaseW", "Has Custom GUI:", nullptr));
        label_19->setText(QCoreApplication::translate("PluginDatabaseW", "Is Synth:", nullptr));
        label_20->setText(QCoreApplication::translate("PluginDatabaseW", "Is Bridged:", nullptr));
        l_aouts->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_id->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_ains->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_synth->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_pins->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_cvins->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_pouts->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_mouts->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_mins->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_bridged->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_idisp->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_gui->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        l_cvouts->setText(QCoreApplication::translate("PluginDatabaseW", "TextLabel", nullptr));
        label_3->setText(QCoreApplication::translate("PluginDatabaseW", "Information", nullptr));
        tab_info->setTabText(tab_info->indexOf(tw_info), QString());
        QTableWidgetItem *___qtablewidgetitem = tableWidget->horizontalHeaderItem(1);
        ___qtablewidgetitem->setText(QCoreApplication::translate("PluginDatabaseW", "Name", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = tableWidget->horizontalHeaderItem(2);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("PluginDatabaseW", "Label/URI", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = tableWidget->horizontalHeaderItem(3);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("PluginDatabaseW", "Maker", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = tableWidget->horizontalHeaderItem(4);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("PluginDatabaseW", "Binary/Filename", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PluginDatabaseW: public Ui_PluginDatabaseW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_DATABASE_H
