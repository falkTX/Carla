/********************************************************************************
** Form generated from reading UI file 'carla_settings.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_SETTINGS_H
#define UI_CARLA_SETTINGS_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CarlaSettingsW
{
public:
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_3;
    QTableWidget *lw_page;
    QStackedWidget *stackedWidget;
    QWidget *page_main;
    QVBoxLayout *verticalLayout_5;
    QHBoxLayout *horizontalLayout;
    QLabel *label_main;
    QSpacerItem *horizontalSpacer;
    QLabel *label_icon_main;
    QGroupBox *group_main_paths;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_main_proj_folder_open;
    QLineEdit *le_main_proj_folder;
    QPushButton *b_main_proj_folder_open;
    QGroupBox *group_main_misc;
    QGridLayout *gridLayout_4;
    QLabel *label_main_refresh_interval;
    QSpinBox *sb_main_refresh_interval;
    QSpacerItem *horizontalSpacer_12;
    QCheckBox *ch_main_show_logs;
    QCheckBox *ch_main_confirm_exit;
    QGroupBox *group_main_theme;
    QGridLayout *gridLayout_3;
    QCheckBox *ch_main_theme_pro;
    QLabel *label_main_theme_color;
    QComboBox *cb_main_theme_color;
    QSpacerItem *horizontalSpacer_2;
    QGroupBox *group_main_experimental;
    QVBoxLayout *verticalLayout_10;
    QCheckBox *ch_main_experimental;
    QSpacerItem *verticalSpacer;
    QWidget *page_canvas;
    QVBoxLayout *verticalLayout_8;
    QHBoxLayout *horizontalLayout_10;
    QLabel *label_9;
    QSpacerItem *horizontalSpacer_5;
    QLabel *label_10;
    QGroupBox *group_canvas_theme;
    QGridLayout *gridLayout_2;
    QSpacerItem *horizontalSpacer_6;
    QSpacerItem *horizontalSpacer_8;
    QCheckBox *cb_canvas_bezier_lines;
    QComboBox *cb_canvas_theme;
    QLabel *label_canvas_theme;
    QSpacerItem *horizontalSpacer_7;
    QSpacerItem *horizontalSpacer_15;
    QLabel *label_canvas_size;
    QComboBox *cb_canvas_size;
    QSpacerItem *horizontalSpacer_16;
    QGroupBox *group_canvas_options;
    QVBoxLayout *verticalLayout_7;
    QCheckBox *cb_canvas_hide_groups;
    QCheckBox *cb_canvas_auto_select;
    QCheckBox *cb_canvas_eyecandy;
    QGroupBox *group_canvas_render;
    QVBoxLayout *verticalLayout_9;
    QCheckBox *cb_canvas_render_aa;
    QCheckBox *cb_canvas_full_repaints;
    QSpacerItem *verticalSpacer_3;
    QWidget *page_engine;
    QVBoxLayout *verticalLayout_6;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_6;
    QSpacerItem *horizontalSpacer_3;
    QLabel *label_icon_engine;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_7;
    QStackedWidget *sw_engine_process_mode;
    QWidget *page_engine_process_mode_jack;
    QHBoxLayout *horizontalLayout_4;
    QComboBox *cb_engine_process_mode_jack;
    QWidget *page_engine_process_mode_other;
    QHBoxLayout *horizontalLayout_7;
    QComboBox *cb_engine_process_mode_other;
    QLabel *label_21;
    QLabel *label_20;
    QLabel *label_7;
    QSpinBox *sb_engine_max_params;
    QToolButton *tb_engine_driver_config;
    QComboBox *cb_engine_audio_driver;
    QSpacerItem *horizontalSpacer_11;
    QSpacerItem *horizontalSpacer_14;
    QGroupBox *groupBox_8;
    QGridLayout *gridLayout_8;
    QLabel *label_engine_ui_bridges_timeout;
    QSpinBox *sb_engine_ui_bridges_timeout;
    QSpacerItem *horizontalSpacer_9;
    QCheckBox *ch_engine_prefer_ui_bridges;
    QCheckBox *ch_engine_uis_always_on_top;
    QCheckBox *ch_engine_manage_uis;
    QLabel *label_engine_ui_bridges_mac_note;
    QLabel *label_15;
    QHBoxLayout *horizontalLayout_13;
    QSpacerItem *horizontalSpacer_20;
    QLabel *label_22;
    QLabel *label_23;
    QSpacerItem *horizontalSpacer_21;
    QSpacerItem *verticalSpacer_9;
    QWidget *page_osc;
    QVBoxLayout *verticalLayout_22;
    QHBoxLayout *horizontalLayout_22;
    QLabel *label_14;
    QSpacerItem *horizontalSpacer_37;
    QLabel *label_icon_engine_5;
    QGroupBox *group_osc_core;
    QGridLayout *gridLayout_13;
    QCheckBox *ch_osc_enable;
    QSpacerItem *horizontalSpacer_43;
    QSpacerItem *horizontalSpacer_44;
    QGroupBox *group_osc_tcp_port;
    QGridLayout *gridLayout_12;
    QRadioButton *rb_osc_tcp_port_specific;
    QHBoxLayout *horizontalLayout_24;
    QSpacerItem *horizontalSpacer_45;
    QLabel *label_39;
    QLabel *label_40;
    QSpacerItem *horizontalSpacer_46;
    QSpacerItem *horizontalSpacer_41;
    QSpacerItem *horizontalSpacer_42;
    QSpinBox *sb_osc_tcp_port_number;
    QRadioButton *rb_osc_tcp_port_random;
    QGroupBox *group_osc_udp_port;
    QGridLayout *gridLayout_11;
    QRadioButton *rb_osc_udp_port_specific;
    QSpacerItem *horizontalSpacer_39;
    QSpacerItem *horizontalSpacer_40;
    QSpinBox *sb_osc_udp_port_number;
    QRadioButton *rb_osc_udp_port_random;
    QHBoxLayout *horizontalLayout_25;
    QSpacerItem *horizontalSpacer_47;
    QLabel *label_41;
    QLabel *label_42;
    QSpacerItem *horizontalSpacer_48;
    QLabel *label_36;
    QHBoxLayout *horizontalLayout_21;
    QSpacerItem *horizontalSpacer_34;
    QLabel *label_32;
    QLabel *label_35;
    QSpacerItem *horizontalSpacer_35;
    QHBoxLayout *horizontalLayout_23;
    QSpacerItem *horizontalSpacer_36;
    QLabel *label_37;
    QLabel *label_38;
    QSpacerItem *horizontalSpacer_38;
    QSpacerItem *verticalSpacer_10;
    QWidget *page_filepaths;
    QVBoxLayout *verticalLayout_30;
    QHBoxLayout *horizontalLayout_8;
    QLabel *label_24;
    QSpacerItem *horizontalSpacer_10;
    QLabel *label_31;
    QHBoxLayout *horizontalLayout_19;
    QComboBox *cb_filepaths;
    QStackedWidget *tw_filepaths_info;
    QWidget *tw_filepaths_info_audio;
    QHBoxLayout *horizontalLayout_20;
    QLabel *label_12;
    QWidget *tw_filepaths_info_midi;
    QHBoxLayout *horizontalLayout_26;
    QLabel *label_13;
    QSpacerItem *horizontalSpacer_23;
    QHBoxLayout *horizontalLayout_16;
    QStackedWidget *tw_filepaths;
    QWidget *tw_filepaths_audio;
    QVBoxLayout *verticalLayout_18;
    QListWidget *lw_files_audio;
    QWidget *tw_filepaths_midi;
    QVBoxLayout *verticalLayout_23;
    QListWidget *lw_files_midi;
    QVBoxLayout *verticalLayout_29;
    QPushButton *b_filepaths_add;
    QPushButton *b_filepaths_remove;
    QSpacerItem *verticalSpacer_6;
    QPushButton *b_filepaths_change;
    QSpacerItem *verticalSpacer_8;
    QWidget *page_pluginpaths;
    QVBoxLayout *verticalLayout_19;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_18;
    QSpacerItem *horizontalSpacer_4;
    QLabel *label_19;
    QHBoxLayout *horizontalLayout_9;
    QComboBox *cb_paths;
    QSpacerItem *horizontalSpacer_17;
    QHBoxLayout *horizontalLayout_11;
    QStackedWidget *tw_paths;
    QWidget *tw_paths_ladspa;
    QVBoxLayout *verticalLayout_11;
    QListWidget *lw_ladspa;
    QWidget *tw_paths_dssi;
    QVBoxLayout *verticalLayout_12;
    QListWidget *lw_dssi;
    QWidget *tw_paths_lv2;
    QGridLayout *gridLayout;
    QListWidget *lw_lv2;
    QLabel *label_4;
    QLabel *label_5;
    QWidget *tw_paths_vst;
    QVBoxLayout *verticalLayout_13;
    QListWidget *lw_vst;
    QWidget *tw_paths_vst3;
    QVBoxLayout *verticalLayout_15;
    QListWidget *lw_vst3;
    QWidget *tw_paths_sf2;
    QVBoxLayout *verticalLayout_4;
    QListWidget *lw_sf2;
    QWidget *tw_paths_sfz;
    QVBoxLayout *verticalLayout_14;
    QListWidget *lw_sfz;
    QVBoxLayout *verticalLayout;
    QPushButton *b_paths_add;
    QPushButton *b_paths_remove;
    QSpacerItem *verticalSpacer_2;
    QPushButton *b_paths_change;
    QSpacerItem *verticalSpacer_7;
    QWidget *page_wine;
    QVBoxLayout *verticalLayout_25;
    QHBoxLayout *horizontalLayout_17;
    QLabel *label_29;
    QSpacerItem *horizontalSpacer_26;
    QLabel *label_icon_wine;
    QGroupBox *group_wine_exec;
    QHBoxLayout *horizontalLayout_18;
    QLabel *label_8;
    QLineEdit *le_wine_exec;
    QGroupBox *group_wine_prefix;
    QGridLayout *gridLayout_6;
    QCheckBox *cb_wine_prefix_detect;
    QLabel *label;
    QLineEdit *le_wine_prefix_fallback;
    QLabel *label_30;
    QGroupBox *group_wine_realtime;
    QGridLayout *gridLayout_5;
    QSpacerItem *horizontalSpacer_28;
    QSpacerItem *horizontalSpacer_27;
    QSpinBox *sb_wine_base_prio;
    QLabel *label_2;
    QLabel *label_3;
    QSpinBox *sb_wine_server_prio;
    QSpacerItem *horizontalSpacer_29;
    QSpacerItem *horizontalSpacer_30;
    QHBoxLayout *horizontalLayout_14;
    QSpacerItem *horizontalSpacer_24;
    QLabel *label_26;
    QLabel *label_27;
    QSpacerItem *horizontalSpacer_25;
    QSpacerItem *verticalSpacer_5;
    QLabel *label_28;
    QWidget *page_experimental;
    QVBoxLayout *verticalLayout_21;
    QHBoxLayout *horizontalLayout_15;
    QLabel *label_11;
    QSpacerItem *horizontalSpacer_22;
    QLabel *label_icon_experimental;
    QHBoxLayout *horizontalLayout_12;
    QSpacerItem *horizontalSpacer_18;
    QLabel *label_16;
    QLabel *label_17;
    QSpacerItem *horizontalSpacer_19;
    QGroupBox *group_experimental_main;
    QVBoxLayout *verticalLayout_17;
    QCheckBox *cb_exp_plugin_bridges;
    QCheckBox *ch_exp_wine_bridges;
    QCheckBox *ch_exp_jack_apps;
    QCheckBox *ch_exp_export_lv2;
    QCheckBox *ch_exp_load_lib_global;
    QGroupBox *group_experimental_canvas;
    QVBoxLayout *verticalLayout_16;
    QCheckBox *cb_canvas_fancy_eyecandy;
    QCheckBox *cb_canvas_use_opengl;
    QCheckBox *cb_canvas_render_hq_aa;
    QCheckBox *cb_canvas_inline_displays;
    QGroupBox *group_experimental_engine;
    QVBoxLayout *verticalLayout_20;
    QCheckBox *ch_engine_force_stereo;
    QCheckBox *ch_exp_prevent_bad_behaviour;
    QCheckBox *ch_engine_prefer_plugin_bridges;
    QSpacerItem *verticalSpacer_4;
    QLabel *label_25;
    QWidget *page;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CarlaSettingsW)
    {
        if (CarlaSettingsW->objectName().isEmpty())
            CarlaSettingsW->setObjectName(QString::fromUtf8("CarlaSettingsW"));
        CarlaSettingsW->resize(612, 572);
        CarlaSettingsW->setMinimumSize(QSize(600, 0));
        verticalLayout_2 = new QVBoxLayout(CarlaSettingsW);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        lw_page = new QTableWidget(CarlaSettingsW);
        if (lw_page->columnCount() < 1)
            lw_page->setColumnCount(1);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        lw_page->setHorizontalHeaderItem(0, __qtablewidgetitem);
        if (lw_page->rowCount() < 8)
            lw_page->setRowCount(8);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        lw_page->setVerticalHeaderItem(0, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        lw_page->setVerticalHeaderItem(1, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        lw_page->setVerticalHeaderItem(2, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        lw_page->setVerticalHeaderItem(3, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        lw_page->setVerticalHeaderItem(4, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        lw_page->setVerticalHeaderItem(5, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        lw_page->setVerticalHeaderItem(6, __qtablewidgetitem7);
        QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
        lw_page->setVerticalHeaderItem(7, __qtablewidgetitem8);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/scalable/carla.svg"), QSize(), QIcon::Normal, QIcon::Off);
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        QTableWidgetItem *__qtablewidgetitem9 = new QTableWidgetItem();
        __qtablewidgetitem9->setFont(font);
        __qtablewidgetitem9->setIcon(icon);
        __qtablewidgetitem9->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        lw_page->setItem(0, 0, __qtablewidgetitem9);
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/48x48/canvas.png"), QSize(), QIcon::Normal, QIcon::Off);
        QTableWidgetItem *__qtablewidgetitem10 = new QTableWidgetItem();
        __qtablewidgetitem10->setFont(font);
        __qtablewidgetitem10->setIcon(icon1);
        __qtablewidgetitem10->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        lw_page->setItem(1, 0, __qtablewidgetitem10);
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/48x48/jack.png"), QSize(), QIcon::Normal, QIcon::Off);
        QTableWidgetItem *__qtablewidgetitem11 = new QTableWidgetItem();
        __qtablewidgetitem11->setFont(font);
        __qtablewidgetitem11->setIcon(icon2);
        __qtablewidgetitem11->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        lw_page->setItem(2, 0, __qtablewidgetitem11);
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/scalable/osc.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        QTableWidgetItem *__qtablewidgetitem12 = new QTableWidgetItem();
        __qtablewidgetitem12->setText(QString::fromUtf8("OSC"));
        __qtablewidgetitem12->setFont(font);
        __qtablewidgetitem12->setIcon(icon3);
        __qtablewidgetitem12->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        lw_page->setItem(3, 0, __qtablewidgetitem12);
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/scalable/folder.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
        __qtablewidgetitem13->setFont(font);
        __qtablewidgetitem13->setIcon(icon4);
        __qtablewidgetitem13->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        lw_page->setItem(4, 0, __qtablewidgetitem13);
        QTableWidgetItem *__qtablewidgetitem14 = new QTableWidgetItem();
        __qtablewidgetitem14->setFont(font);
        __qtablewidgetitem14->setIcon(icon4);
        __qtablewidgetitem14->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        lw_page->setItem(5, 0, __qtablewidgetitem14);
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/scalable/wine.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        QTableWidgetItem *__qtablewidgetitem15 = new QTableWidgetItem();
        __qtablewidgetitem15->setFont(font);
        __qtablewidgetitem15->setIcon(icon5);
        __qtablewidgetitem15->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        lw_page->setItem(6, 0, __qtablewidgetitem15);
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/scalable/warning.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        QTableWidgetItem *__qtablewidgetitem16 = new QTableWidgetItem();
        __qtablewidgetitem16->setFont(font);
        __qtablewidgetitem16->setIcon(icon6);
        __qtablewidgetitem16->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        lw_page->setItem(7, 0, __qtablewidgetitem16);
        lw_page->setObjectName(QString::fromUtf8("lw_page"));
        lw_page->setEditTriggers(QAbstractItemView::NoEditTriggers);
        lw_page->setProperty("showDropIndicator", QVariant(false));
        lw_page->setDragDropOverwriteMode(false);
        lw_page->setSelectionMode(QAbstractItemView::SingleSelection);
        lw_page->setSelectionBehavior(QAbstractItemView::SelectRows);
        lw_page->setIconSize(QSize(48, 48));
        lw_page->setTextElideMode(Qt::ElideMiddle);
        lw_page->setWordWrap(false);
        lw_page->setCornerButtonEnabled(false);
        lw_page->horizontalHeader()->setVisible(false);
        lw_page->horizontalHeader()->setStretchLastSection(true);
        lw_page->verticalHeader()->setVisible(false);
        lw_page->verticalHeader()->setDefaultSectionSize(52);

        horizontalLayout_3->addWidget(lw_page);

        stackedWidget = new QStackedWidget(CarlaSettingsW);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setLineWidth(0);
        page_main = new QWidget();
        page_main->setObjectName(QString::fromUtf8("page_main"));
        verticalLayout_5 = new QVBoxLayout(page_main);
        verticalLayout_5->setContentsMargins(2, 2, 2, 2);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label_main = new QLabel(page_main);
        label_main->setObjectName(QString::fromUtf8("label_main"));
        label_main->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        horizontalLayout->addWidget(label_main);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        label_icon_main = new QLabel(page_main);
        label_icon_main->setObjectName(QString::fromUtf8("label_icon_main"));
        label_icon_main->setMaximumSize(QSize(48, 48));
        label_icon_main->setText(QString::fromUtf8(""));
        label_icon_main->setPixmap(QPixmap(QString::fromUtf8(":/scalable/carla.svg")));
        label_icon_main->setScaledContents(true);
        label_icon_main->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        horizontalLayout->addWidget(label_icon_main);


        verticalLayout_5->addLayout(horizontalLayout);

        group_main_paths = new QGroupBox(page_main);
        group_main_paths->setObjectName(QString::fromUtf8("group_main_paths"));
        verticalLayout_3 = new QVBoxLayout(group_main_paths);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label_main_proj_folder_open = new QLabel(group_main_paths);
        label_main_proj_folder_open->setObjectName(QString::fromUtf8("label_main_proj_folder_open"));

        horizontalLayout_2->addWidget(label_main_proj_folder_open);

        le_main_proj_folder = new QLineEdit(group_main_paths);
        le_main_proj_folder->setObjectName(QString::fromUtf8("le_main_proj_folder"));

        horizontalLayout_2->addWidget(le_main_proj_folder);

        b_main_proj_folder_open = new QPushButton(group_main_paths);
        b_main_proj_folder_open->setObjectName(QString::fromUtf8("b_main_proj_folder_open"));
        b_main_proj_folder_open->setMaximumSize(QSize(22, 22));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/16x16/document-open.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_main_proj_folder_open->setIcon(icon7);
        b_main_proj_folder_open->setIconSize(QSize(16, 16));

        horizontalLayout_2->addWidget(b_main_proj_folder_open);


        verticalLayout_3->addLayout(horizontalLayout_2);


        verticalLayout_5->addWidget(group_main_paths);

        group_main_misc = new QGroupBox(page_main);
        group_main_misc->setObjectName(QString::fromUtf8("group_main_misc"));
        gridLayout_4 = new QGridLayout(group_main_misc);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        label_main_refresh_interval = new QLabel(group_main_misc);
        label_main_refresh_interval->setObjectName(QString::fromUtf8("label_main_refresh_interval"));
        label_main_refresh_interval->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_4->addWidget(label_main_refresh_interval, 3, 0, 1, 1);

        sb_main_refresh_interval = new QSpinBox(group_main_misc);
        sb_main_refresh_interval->setObjectName(QString::fromUtf8("sb_main_refresh_interval"));
        sb_main_refresh_interval->setMinimum(10);
        sb_main_refresh_interval->setMaximum(1000);
        sb_main_refresh_interval->setValue(250);

        gridLayout_4->addWidget(sb_main_refresh_interval, 3, 1, 1, 1);

        horizontalSpacer_12 = new QSpacerItem(130, 10, QSizePolicy::Preferred, QSizePolicy::Minimum);

        gridLayout_4->addItem(horizontalSpacer_12, 3, 2, 1, 1);

        ch_main_show_logs = new QCheckBox(group_main_misc);
        ch_main_show_logs->setObjectName(QString::fromUtf8("ch_main_show_logs"));

        gridLayout_4->addWidget(ch_main_show_logs, 1, 0, 1, 3);

        ch_main_confirm_exit = new QCheckBox(group_main_misc);
        ch_main_confirm_exit->setObjectName(QString::fromUtf8("ch_main_confirm_exit"));

        gridLayout_4->addWidget(ch_main_confirm_exit, 2, 0, 1, 3);


        verticalLayout_5->addWidget(group_main_misc);

        group_main_theme = new QGroupBox(page_main);
        group_main_theme->setObjectName(QString::fromUtf8("group_main_theme"));
        gridLayout_3 = new QGridLayout(group_main_theme);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        ch_main_theme_pro = new QCheckBox(group_main_theme);
        ch_main_theme_pro->setObjectName(QString::fromUtf8("ch_main_theme_pro"));

        gridLayout_3->addWidget(ch_main_theme_pro, 0, 0, 1, 3);

        label_main_theme_color = new QLabel(group_main_theme);
        label_main_theme_color->setObjectName(QString::fromUtf8("label_main_theme_color"));
        label_main_theme_color->setEnabled(false);
        label_main_theme_color->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_3->addWidget(label_main_theme_color, 1, 0, 1, 1);

        cb_main_theme_color = new QComboBox(group_main_theme);
        cb_main_theme_color->addItem(QString());
        cb_main_theme_color->addItem(QString());
        cb_main_theme_color->setObjectName(QString::fromUtf8("cb_main_theme_color"));
        cb_main_theme_color->setEnabled(false);

        gridLayout_3->addWidget(cb_main_theme_color, 1, 1, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(130, 10, QSizePolicy::Preferred, QSizePolicy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_2, 1, 2, 1, 1);


        verticalLayout_5->addWidget(group_main_theme);

        group_main_experimental = new QGroupBox(page_main);
        group_main_experimental->setObjectName(QString::fromUtf8("group_main_experimental"));
        verticalLayout_10 = new QVBoxLayout(group_main_experimental);
        verticalLayout_10->setObjectName(QString::fromUtf8("verticalLayout_10"));
        ch_main_experimental = new QCheckBox(group_main_experimental);
        ch_main_experimental->setObjectName(QString::fromUtf8("ch_main_experimental"));

        verticalLayout_10->addWidget(ch_main_experimental);


        verticalLayout_5->addWidget(group_main_experimental);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer);

        stackedWidget->addWidget(page_main);
        page_canvas = new QWidget();
        page_canvas->setObjectName(QString::fromUtf8("page_canvas"));
        verticalLayout_8 = new QVBoxLayout(page_canvas);
        verticalLayout_8->setContentsMargins(2, 2, 2, 2);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        label_9 = new QLabel(page_canvas);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        horizontalLayout_10->addWidget(label_9);

        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_10->addItem(horizontalSpacer_5);

        label_10 = new QLabel(page_canvas);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setText(QString::fromUtf8(""));
        label_10->setPixmap(QPixmap(QString::fromUtf8(":/48x48/canvas.png")));
        label_10->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        horizontalLayout_10->addWidget(label_10);


        verticalLayout_8->addLayout(horizontalLayout_10);

        group_canvas_theme = new QGroupBox(page_canvas);
        group_canvas_theme->setObjectName(QString::fromUtf8("group_canvas_theme"));
        gridLayout_2 = new QGridLayout(group_canvas_theme);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_6, 0, 4, 1, 1);

        horizontalSpacer_8 = new QSpacerItem(80, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_8, 2, 0, 1, 2);

        cb_canvas_bezier_lines = new QCheckBox(group_canvas_theme);
        cb_canvas_bezier_lines->setObjectName(QString::fromUtf8("cb_canvas_bezier_lines"));

        gridLayout_2->addWidget(cb_canvas_bezier_lines, 2, 2, 1, 3);

        cb_canvas_theme = new QComboBox(group_canvas_theme);
        cb_canvas_theme->setObjectName(QString::fromUtf8("cb_canvas_theme"));

        gridLayout_2->addWidget(cb_canvas_theme, 0, 3, 1, 1);

        label_canvas_theme = new QLabel(group_canvas_theme);
        label_canvas_theme->setObjectName(QString::fromUtf8("label_canvas_theme"));
        label_canvas_theme->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_canvas_theme, 0, 1, 1, 2);

        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_7, 0, 0, 1, 1);

        horizontalSpacer_15 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_15, 1, 0, 1, 1);

        label_canvas_size = new QLabel(group_canvas_theme);
        label_canvas_size->setObjectName(QString::fromUtf8("label_canvas_size"));
        label_canvas_size->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_canvas_size, 1, 1, 1, 2);

        cb_canvas_size = new QComboBox(group_canvas_theme);
        cb_canvas_size->addItem(QString());
        cb_canvas_size->addItem(QString());
        cb_canvas_size->addItem(QString());
        cb_canvas_size->addItem(QString());
        cb_canvas_size->addItem(QString());
        cb_canvas_size->setObjectName(QString::fromUtf8("cb_canvas_size"));

        gridLayout_2->addWidget(cb_canvas_size, 1, 3, 1, 1);

        horizontalSpacer_16 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_16, 1, 4, 1, 1);


        verticalLayout_8->addWidget(group_canvas_theme);

        group_canvas_options = new QGroupBox(page_canvas);
        group_canvas_options->setObjectName(QString::fromUtf8("group_canvas_options"));
        verticalLayout_7 = new QVBoxLayout(group_canvas_options);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        cb_canvas_hide_groups = new QCheckBox(group_canvas_options);
        cb_canvas_hide_groups->setObjectName(QString::fromUtf8("cb_canvas_hide_groups"));

        verticalLayout_7->addWidget(cb_canvas_hide_groups);

        cb_canvas_auto_select = new QCheckBox(group_canvas_options);
        cb_canvas_auto_select->setObjectName(QString::fromUtf8("cb_canvas_auto_select"));

        verticalLayout_7->addWidget(cb_canvas_auto_select);

        cb_canvas_eyecandy = new QCheckBox(group_canvas_options);
        cb_canvas_eyecandy->setObjectName(QString::fromUtf8("cb_canvas_eyecandy"));

        verticalLayout_7->addWidget(cb_canvas_eyecandy);


        verticalLayout_8->addWidget(group_canvas_options);

        group_canvas_render = new QGroupBox(page_canvas);
        group_canvas_render->setObjectName(QString::fromUtf8("group_canvas_render"));
        verticalLayout_9 = new QVBoxLayout(group_canvas_render);
        verticalLayout_9->setObjectName(QString::fromUtf8("verticalLayout_9"));
        cb_canvas_render_aa = new QCheckBox(group_canvas_render);
        cb_canvas_render_aa->setObjectName(QString::fromUtf8("cb_canvas_render_aa"));
        cb_canvas_render_aa->setTristate(true);

        verticalLayout_9->addWidget(cb_canvas_render_aa);

        cb_canvas_full_repaints = new QCheckBox(group_canvas_render);
        cb_canvas_full_repaints->setObjectName(QString::fromUtf8("cb_canvas_full_repaints"));

        verticalLayout_9->addWidget(cb_canvas_full_repaints);


        verticalLayout_8->addWidget(group_canvas_render);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_8->addItem(verticalSpacer_3);

        stackedWidget->addWidget(page_canvas);
        page_engine = new QWidget();
        page_engine->setObjectName(QString::fromUtf8("page_engine"));
        verticalLayout_6 = new QVBoxLayout(page_engine);
        verticalLayout_6->setContentsMargins(2, 2, 2, 2);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        label_6 = new QLabel(page_engine);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        horizontalLayout_5->addWidget(label_6);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_3);

        label_icon_engine = new QLabel(page_engine);
        label_icon_engine->setObjectName(QString::fromUtf8("label_icon_engine"));
        label_icon_engine->setText(QString::fromUtf8(""));
        label_icon_engine->setPixmap(QPixmap(QString::fromUtf8(":/48x48/jack.png")));
        label_icon_engine->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        horizontalLayout_5->addWidget(label_icon_engine);


        verticalLayout_6->addLayout(horizontalLayout_5);

        groupBox = new QGroupBox(page_engine);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_7 = new QGridLayout(groupBox);
        gridLayout_7->setObjectName(QString::fromUtf8("gridLayout_7"));
        sw_engine_process_mode = new QStackedWidget(groupBox);
        sw_engine_process_mode->setObjectName(QString::fromUtf8("sw_engine_process_mode"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(sw_engine_process_mode->sizePolicy().hasHeightForWidth());
        sw_engine_process_mode->setSizePolicy(sizePolicy);
        page_engine_process_mode_jack = new QWidget();
        page_engine_process_mode_jack->setObjectName(QString::fromUtf8("page_engine_process_mode_jack"));
        horizontalLayout_4 = new QHBoxLayout(page_engine_process_mode_jack);
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalLayout_4->setSizeConstraint(QLayout::SetMinimumSize);
        cb_engine_process_mode_jack = new QComboBox(page_engine_process_mode_jack);
        cb_engine_process_mode_jack->addItem(QString());
        cb_engine_process_mode_jack->addItem(QString());
        cb_engine_process_mode_jack->addItem(QString());
        cb_engine_process_mode_jack->addItem(QString());
        cb_engine_process_mode_jack->setObjectName(QString::fromUtf8("cb_engine_process_mode_jack"));

        horizontalLayout_4->addWidget(cb_engine_process_mode_jack);

        sw_engine_process_mode->addWidget(page_engine_process_mode_jack);
        page_engine_process_mode_other = new QWidget();
        page_engine_process_mode_other->setObjectName(QString::fromUtf8("page_engine_process_mode_other"));
        horizontalLayout_7 = new QHBoxLayout(page_engine_process_mode_other);
        horizontalLayout_7->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        horizontalLayout_7->setSizeConstraint(QLayout::SetMinimumSize);
        cb_engine_process_mode_other = new QComboBox(page_engine_process_mode_other);
        cb_engine_process_mode_other->addItem(QString());
        cb_engine_process_mode_other->addItem(QString());
        cb_engine_process_mode_other->setObjectName(QString::fromUtf8("cb_engine_process_mode_other"));

        horizontalLayout_7->addWidget(cb_engine_process_mode_other);

        sw_engine_process_mode->addWidget(page_engine_process_mode_other);

        gridLayout_7->addWidget(sw_engine_process_mode, 1, 2, 1, 1);

        label_21 = new QLabel(groupBox);
        label_21->setObjectName(QString::fromUtf8("label_21"));
        label_21->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_7->addWidget(label_21, 0, 1, 1, 1);

        label_20 = new QLabel(groupBox);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        label_20->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_7->addWidget(label_20, 1, 1, 1, 1);

        label_7 = new QLabel(groupBox);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_7->addWidget(label_7, 2, 1, 1, 1);

        sb_engine_max_params = new QSpinBox(groupBox);
        sb_engine_max_params->setObjectName(QString::fromUtf8("sb_engine_max_params"));
        sb_engine_max_params->setMaximum(1000);

        gridLayout_7->addWidget(sb_engine_max_params, 2, 2, 1, 1);

        tb_engine_driver_config = new QToolButton(groupBox);
        tb_engine_driver_config->setObjectName(QString::fromUtf8("tb_engine_driver_config"));

        gridLayout_7->addWidget(tb_engine_driver_config, 0, 3, 1, 1);

        cb_engine_audio_driver = new QComboBox(groupBox);
        cb_engine_audio_driver->setObjectName(QString::fromUtf8("cb_engine_audio_driver"));

        gridLayout_7->addWidget(cb_engine_audio_driver, 0, 2, 1, 1);

        horizontalSpacer_11 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_7->addItem(horizontalSpacer_11, 0, 0, 1, 1);

        horizontalSpacer_14 = new QSpacerItem(100, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_7->addItem(horizontalSpacer_14, 1, 3, 1, 1);


        verticalLayout_6->addWidget(groupBox);

        groupBox_8 = new QGroupBox(page_engine);
        groupBox_8->setObjectName(QString::fromUtf8("groupBox_8"));
        gridLayout_8 = new QGridLayout(groupBox_8);
        gridLayout_8->setObjectName(QString::fromUtf8("gridLayout_8"));
        label_engine_ui_bridges_timeout = new QLabel(groupBox_8);
        label_engine_ui_bridges_timeout->setObjectName(QString::fromUtf8("label_engine_ui_bridges_timeout"));
        label_engine_ui_bridges_timeout->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_8->addWidget(label_engine_ui_bridges_timeout, 5, 0, 1, 1);

        sb_engine_ui_bridges_timeout = new QSpinBox(groupBox_8);
        sb_engine_ui_bridges_timeout->setObjectName(QString::fromUtf8("sb_engine_ui_bridges_timeout"));
        sb_engine_ui_bridges_timeout->setMaximum(10000);

        gridLayout_8->addWidget(sb_engine_ui_bridges_timeout, 5, 1, 1, 1);

        horizontalSpacer_9 = new QSpacerItem(100, 10, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_8->addItem(horizontalSpacer_9, 5, 2, 1, 1);

        ch_engine_prefer_ui_bridges = new QCheckBox(groupBox_8);
        ch_engine_prefer_ui_bridges->setObjectName(QString::fromUtf8("ch_engine_prefer_ui_bridges"));

        gridLayout_8->addWidget(ch_engine_prefer_ui_bridges, 2, 0, 1, 3);

        ch_engine_uis_always_on_top = new QCheckBox(groupBox_8);
        ch_engine_uis_always_on_top->setObjectName(QString::fromUtf8("ch_engine_uis_always_on_top"));

        gridLayout_8->addWidget(ch_engine_uis_always_on_top, 1, 0, 1, 3);

        ch_engine_manage_uis = new QCheckBox(groupBox_8);
        ch_engine_manage_uis->setObjectName(QString::fromUtf8("ch_engine_manage_uis"));

        gridLayout_8->addWidget(ch_engine_manage_uis, 0, 0, 1, 3);

        label_engine_ui_bridges_mac_note = new QLabel(groupBox_8);
        label_engine_ui_bridges_mac_note->setObjectName(QString::fromUtf8("label_engine_ui_bridges_mac_note"));

        gridLayout_8->addWidget(label_engine_ui_bridges_mac_note, 6, 0, 1, 3);


        verticalLayout_6->addWidget(groupBox_8);

        label_15 = new QLabel(page_engine);
        label_15->setObjectName(QString::fromUtf8("label_15"));
        label_15->setMaximumSize(QSize(16777215, 5));

        verticalLayout_6->addWidget(label_15);

        horizontalLayout_13 = new QHBoxLayout();
        horizontalLayout_13->setObjectName(QString::fromUtf8("horizontalLayout_13"));
        horizontalSpacer_20 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_13->addItem(horizontalSpacer_20);

        label_22 = new QLabel(page_engine);
        label_22->setObjectName(QString::fromUtf8("label_22"));
        label_22->setMaximumSize(QSize(22, 22));
        label_22->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-information.svgz")));
        label_22->setScaledContents(true);
        label_22->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_13->addWidget(label_22);

        label_23 = new QLabel(page_engine);
        label_23->setObjectName(QString::fromUtf8("label_23"));

        horizontalLayout_13->addWidget(label_23);

        horizontalSpacer_21 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_13->addItem(horizontalSpacer_21);


        verticalLayout_6->addLayout(horizontalLayout_13);

        verticalSpacer_9 = new QSpacerItem(20, 300, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer_9);

        stackedWidget->addWidget(page_engine);
        page_osc = new QWidget();
        page_osc->setObjectName(QString::fromUtf8("page_osc"));
        verticalLayout_22 = new QVBoxLayout(page_osc);
        verticalLayout_22->setContentsMargins(2, 2, 2, 2);
        verticalLayout_22->setObjectName(QString::fromUtf8("verticalLayout_22"));
        horizontalLayout_22 = new QHBoxLayout();
        horizontalLayout_22->setObjectName(QString::fromUtf8("horizontalLayout_22"));
        label_14 = new QLabel(page_osc);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        label_14->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        horizontalLayout_22->addWidget(label_14);

        horizontalSpacer_37 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_22->addItem(horizontalSpacer_37);

        label_icon_engine_5 = new QLabel(page_osc);
        label_icon_engine_5->setObjectName(QString::fromUtf8("label_icon_engine_5"));
        label_icon_engine_5->setMaximumSize(QSize(48, 48));
        label_icon_engine_5->setText(QString::fromUtf8(""));
        label_icon_engine_5->setPixmap(QPixmap(QString::fromUtf8(":/scalable/osc.svgz")));
        label_icon_engine_5->setScaledContents(true);
        label_icon_engine_5->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        horizontalLayout_22->addWidget(label_icon_engine_5);


        verticalLayout_22->addLayout(horizontalLayout_22);

        group_osc_core = new QGroupBox(page_osc);
        group_osc_core->setObjectName(QString::fromUtf8("group_osc_core"));
        gridLayout_13 = new QGridLayout(group_osc_core);
        gridLayout_13->setObjectName(QString::fromUtf8("gridLayout_13"));
        ch_osc_enable = new QCheckBox(group_osc_core);
        ch_osc_enable->setObjectName(QString::fromUtf8("ch_osc_enable"));

        gridLayout_13->addWidget(ch_osc_enable, 0, 1, 1, 1);

        horizontalSpacer_43 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_13->addItem(horizontalSpacer_43, 0, 0, 1, 1);

        horizontalSpacer_44 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_13->addItem(horizontalSpacer_44, 0, 2, 1, 1);


        verticalLayout_22->addWidget(group_osc_core);

        group_osc_tcp_port = new QGroupBox(page_osc);
        group_osc_tcp_port->setObjectName(QString::fromUtf8("group_osc_tcp_port"));
        group_osc_tcp_port->setCheckable(true);
        gridLayout_12 = new QGridLayout(group_osc_tcp_port);
        gridLayout_12->setObjectName(QString::fromUtf8("gridLayout_12"));
        rb_osc_tcp_port_specific = new QRadioButton(group_osc_tcp_port);
        rb_osc_tcp_port_specific->setObjectName(QString::fromUtf8("rb_osc_tcp_port_specific"));

        gridLayout_12->addWidget(rb_osc_tcp_port_specific, 1, 1, 1, 1);

        horizontalLayout_24 = new QHBoxLayout();
        horizontalLayout_24->setObjectName(QString::fromUtf8("horizontalLayout_24"));
        horizontalSpacer_45 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_24->addItem(horizontalSpacer_45);

        label_39 = new QLabel(group_osc_tcp_port);
        label_39->setObjectName(QString::fromUtf8("label_39"));
        label_39->setMaximumSize(QSize(22, 22));
        label_39->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-information.svgz")));
        label_39->setScaledContents(true);
        label_39->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_24->addWidget(label_39);

        label_40 = new QLabel(group_osc_tcp_port);
        label_40->setObjectName(QString::fromUtf8("label_40"));
        label_40->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

        horizontalLayout_24->addWidget(label_40);

        horizontalSpacer_46 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_24->addItem(horizontalSpacer_46);


        gridLayout_12->addLayout(horizontalLayout_24, 2, 0, 1, 4);

        horizontalSpacer_41 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_12->addItem(horizontalSpacer_41, 0, 0, 2, 1);

        horizontalSpacer_42 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_12->addItem(horizontalSpacer_42, 0, 3, 2, 1);

        sb_osc_tcp_port_number = new QSpinBox(group_osc_tcp_port);
        sb_osc_tcp_port_number->setObjectName(QString::fromUtf8("sb_osc_tcp_port_number"));
        sb_osc_tcp_port_number->setEnabled(false);
        sb_osc_tcp_port_number->setMinimum(1024);
        sb_osc_tcp_port_number->setMaximum(32767);

        gridLayout_12->addWidget(sb_osc_tcp_port_number, 1, 2, 1, 1);

        rb_osc_tcp_port_random = new QRadioButton(group_osc_tcp_port);
        rb_osc_tcp_port_random->setObjectName(QString::fromUtf8("rb_osc_tcp_port_random"));
        rb_osc_tcp_port_random->setChecked(true);

        gridLayout_12->addWidget(rb_osc_tcp_port_random, 0, 1, 1, 2);


        verticalLayout_22->addWidget(group_osc_tcp_port);

        group_osc_udp_port = new QGroupBox(page_osc);
        group_osc_udp_port->setObjectName(QString::fromUtf8("group_osc_udp_port"));
        group_osc_udp_port->setCheckable(true);
        gridLayout_11 = new QGridLayout(group_osc_udp_port);
        gridLayout_11->setObjectName(QString::fromUtf8("gridLayout_11"));
        rb_osc_udp_port_specific = new QRadioButton(group_osc_udp_port);
        rb_osc_udp_port_specific->setObjectName(QString::fromUtf8("rb_osc_udp_port_specific"));

        gridLayout_11->addWidget(rb_osc_udp_port_specific, 1, 1, 1, 1);

        horizontalSpacer_39 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_11->addItem(horizontalSpacer_39, 0, 0, 2, 1);

        horizontalSpacer_40 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_11->addItem(horizontalSpacer_40, 0, 3, 2, 1);

        sb_osc_udp_port_number = new QSpinBox(group_osc_udp_port);
        sb_osc_udp_port_number->setObjectName(QString::fromUtf8("sb_osc_udp_port_number"));
        sb_osc_udp_port_number->setEnabled(false);
        sb_osc_udp_port_number->setMinimum(1024);
        sb_osc_udp_port_number->setMaximum(32767);

        gridLayout_11->addWidget(sb_osc_udp_port_number, 1, 2, 1, 1);

        rb_osc_udp_port_random = new QRadioButton(group_osc_udp_port);
        rb_osc_udp_port_random->setObjectName(QString::fromUtf8("rb_osc_udp_port_random"));
        rb_osc_udp_port_random->setChecked(true);

        gridLayout_11->addWidget(rb_osc_udp_port_random, 0, 1, 1, 2);

        horizontalLayout_25 = new QHBoxLayout();
        horizontalLayout_25->setObjectName(QString::fromUtf8("horizontalLayout_25"));
        horizontalSpacer_47 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_25->addItem(horizontalSpacer_47);

        label_41 = new QLabel(group_osc_udp_port);
        label_41->setObjectName(QString::fromUtf8("label_41"));
        label_41->setMaximumSize(QSize(22, 22));
        label_41->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-information.svgz")));
        label_41->setScaledContents(true);
        label_41->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_25->addWidget(label_41);

        label_42 = new QLabel(group_osc_udp_port);
        label_42->setObjectName(QString::fromUtf8("label_42"));
        label_42->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

        horizontalLayout_25->addWidget(label_42);

        horizontalSpacer_48 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_25->addItem(horizontalSpacer_48);


        gridLayout_11->addLayout(horizontalLayout_25, 2, 0, 1, 4);


        verticalLayout_22->addWidget(group_osc_udp_port);

        label_36 = new QLabel(page_osc);
        label_36->setObjectName(QString::fromUtf8("label_36"));
        label_36->setMaximumSize(QSize(16777215, 5));

        verticalLayout_22->addWidget(label_36);

        horizontalLayout_21 = new QHBoxLayout();
        horizontalLayout_21->setObjectName(QString::fromUtf8("horizontalLayout_21"));
        horizontalSpacer_34 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_21->addItem(horizontalSpacer_34);

        label_32 = new QLabel(page_osc);
        label_32->setObjectName(QString::fromUtf8("label_32"));
        label_32->setMaximumSize(QSize(22, 22));
        label_32->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-information.svgz")));
        label_32->setScaledContents(true);
        label_32->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_21->addWidget(label_32);

        label_35 = new QLabel(page_osc);
        label_35->setObjectName(QString::fromUtf8("label_35"));

        horizontalLayout_21->addWidget(label_35);

        horizontalSpacer_35 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_21->addItem(horizontalSpacer_35);


        verticalLayout_22->addLayout(horizontalLayout_21);

        horizontalLayout_23 = new QHBoxLayout();
        horizontalLayout_23->setObjectName(QString::fromUtf8("horizontalLayout_23"));
        horizontalSpacer_36 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_23->addItem(horizontalSpacer_36);

        label_37 = new QLabel(page_osc);
        label_37->setObjectName(QString::fromUtf8("label_37"));
        label_37->setMaximumSize(QSize(22, 22));
        label_37->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-warning.svgz")));
        label_37->setScaledContents(true);
        label_37->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_23->addWidget(label_37);

        label_38 = new QLabel(page_osc);
        label_38->setObjectName(QString::fromUtf8("label_38"));

        horizontalLayout_23->addWidget(label_38);

        horizontalSpacer_38 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_23->addItem(horizontalSpacer_38);


        verticalLayout_22->addLayout(horizontalLayout_23);

        verticalSpacer_10 = new QSpacerItem(20, 126, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_22->addItem(verticalSpacer_10);

        stackedWidget->addWidget(page_osc);
        label_36->raise();
        group_osc_tcp_port->raise();
        group_osc_udp_port->raise();
        group_osc_core->raise();
        page_filepaths = new QWidget();
        page_filepaths->setObjectName(QString::fromUtf8("page_filepaths"));
        verticalLayout_30 = new QVBoxLayout(page_filepaths);
        verticalLayout_30->setContentsMargins(2, 2, 2, 2);
        verticalLayout_30->setObjectName(QString::fromUtf8("verticalLayout_30"));
        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        label_24 = new QLabel(page_filepaths);
        label_24->setObjectName(QString::fromUtf8("label_24"));
        label_24->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        horizontalLayout_8->addWidget(label_24);

        horizontalSpacer_10 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_10);

        label_31 = new QLabel(page_filepaths);
        label_31->setObjectName(QString::fromUtf8("label_31"));
        label_31->setMaximumSize(QSize(48, 48));
        label_31->setText(QString::fromUtf8(""));
        label_31->setPixmap(QPixmap(QString::fromUtf8(":/scalable/folder.svgz")));
        label_31->setScaledContents(true);
        label_31->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
        label_31->setWordWrap(true);

        horizontalLayout_8->addWidget(label_31);


        verticalLayout_30->addLayout(horizontalLayout_8);

        horizontalLayout_19 = new QHBoxLayout();
        horizontalLayout_19->setObjectName(QString::fromUtf8("horizontalLayout_19"));
        cb_filepaths = new QComboBox(page_filepaths);
        cb_filepaths->addItem(QString());
        cb_filepaths->addItem(QString());
        cb_filepaths->setObjectName(QString::fromUtf8("cb_filepaths"));
        cb_filepaths->setMinimumSize(QSize(120, 0));

        horizontalLayout_19->addWidget(cb_filepaths);

        tw_filepaths_info = new QStackedWidget(page_filepaths);
        tw_filepaths_info->setObjectName(QString::fromUtf8("tw_filepaths_info"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(tw_filepaths_info->sizePolicy().hasHeightForWidth());
        tw_filepaths_info->setSizePolicy(sizePolicy1);
        tw_filepaths_info_audio = new QWidget();
        tw_filepaths_info_audio->setObjectName(QString::fromUtf8("tw_filepaths_info_audio"));
        horizontalLayout_20 = new QHBoxLayout(tw_filepaths_info_audio);
        horizontalLayout_20->setSpacing(0);
        horizontalLayout_20->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_20->setObjectName(QString::fromUtf8("horizontalLayout_20"));
        label_12 = new QLabel(tw_filepaths_info_audio);
        label_12->setObjectName(QString::fromUtf8("label_12"));

        horizontalLayout_20->addWidget(label_12);

        tw_filepaths_info->addWidget(tw_filepaths_info_audio);
        tw_filepaths_info_midi = new QWidget();
        tw_filepaths_info_midi->setObjectName(QString::fromUtf8("tw_filepaths_info_midi"));
        horizontalLayout_26 = new QHBoxLayout(tw_filepaths_info_midi);
        horizontalLayout_26->setSpacing(0);
        horizontalLayout_26->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_26->setObjectName(QString::fromUtf8("horizontalLayout_26"));
        label_13 = new QLabel(tw_filepaths_info_midi);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        horizontalLayout_26->addWidget(label_13);

        tw_filepaths_info->addWidget(tw_filepaths_info_midi);

        horizontalLayout_19->addWidget(tw_filepaths_info);

        horizontalSpacer_23 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_19->addItem(horizontalSpacer_23);


        verticalLayout_30->addLayout(horizontalLayout_19);

        horizontalLayout_16 = new QHBoxLayout();
        horizontalLayout_16->setObjectName(QString::fromUtf8("horizontalLayout_16"));
        tw_filepaths = new QStackedWidget(page_filepaths);
        tw_filepaths->setObjectName(QString::fromUtf8("tw_filepaths"));
        tw_filepaths_audio = new QWidget();
        tw_filepaths_audio->setObjectName(QString::fromUtf8("tw_filepaths_audio"));
        verticalLayout_18 = new QVBoxLayout(tw_filepaths_audio);
        verticalLayout_18->setSpacing(0);
        verticalLayout_18->setContentsMargins(0, 0, 0, 0);
        verticalLayout_18->setObjectName(QString::fromUtf8("verticalLayout_18"));
        lw_files_audio = new QListWidget(tw_filepaths_audio);
        lw_files_audio->setObjectName(QString::fromUtf8("lw_files_audio"));

        verticalLayout_18->addWidget(lw_files_audio);

        tw_filepaths->addWidget(tw_filepaths_audio);
        tw_filepaths_midi = new QWidget();
        tw_filepaths_midi->setObjectName(QString::fromUtf8("tw_filepaths_midi"));
        verticalLayout_23 = new QVBoxLayout(tw_filepaths_midi);
        verticalLayout_23->setSpacing(0);
        verticalLayout_23->setContentsMargins(0, 0, 0, 0);
        verticalLayout_23->setObjectName(QString::fromUtf8("verticalLayout_23"));
        lw_files_midi = new QListWidget(tw_filepaths_midi);
        lw_files_midi->setObjectName(QString::fromUtf8("lw_files_midi"));

        verticalLayout_23->addWidget(lw_files_midi);

        tw_filepaths->addWidget(tw_filepaths_midi);

        horizontalLayout_16->addWidget(tw_filepaths);

        verticalLayout_29 = new QVBoxLayout();
        verticalLayout_29->setObjectName(QString::fromUtf8("verticalLayout_29"));
        b_filepaths_add = new QPushButton(page_filepaths);
        b_filepaths_add->setObjectName(QString::fromUtf8("b_filepaths_add"));
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/16x16/list-add.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_filepaths_add->setIcon(icon8);

        verticalLayout_29->addWidget(b_filepaths_add);

        b_filepaths_remove = new QPushButton(page_filepaths);
        b_filepaths_remove->setObjectName(QString::fromUtf8("b_filepaths_remove"));
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/16x16/list-remove.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_filepaths_remove->setIcon(icon9);

        verticalLayout_29->addWidget(b_filepaths_remove);

        verticalSpacer_6 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_29->addItem(verticalSpacer_6);

        b_filepaths_change = new QPushButton(page_filepaths);
        b_filepaths_change->setObjectName(QString::fromUtf8("b_filepaths_change"));
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/16x16/edit-rename.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_filepaths_change->setIcon(icon10);

        verticalLayout_29->addWidget(b_filepaths_change);

        verticalSpacer_8 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_29->addItem(verticalSpacer_8);


        horizontalLayout_16->addLayout(verticalLayout_29);


        verticalLayout_30->addLayout(horizontalLayout_16);

        stackedWidget->addWidget(page_filepaths);
        page_pluginpaths = new QWidget();
        page_pluginpaths->setObjectName(QString::fromUtf8("page_pluginpaths"));
        verticalLayout_19 = new QVBoxLayout(page_pluginpaths);
        verticalLayout_19->setContentsMargins(2, 2, 2, 2);
        verticalLayout_19->setObjectName(QString::fromUtf8("verticalLayout_19"));
        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        label_18 = new QLabel(page_pluginpaths);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        label_18->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        horizontalLayout_6->addWidget(label_18);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_4);

        label_19 = new QLabel(page_pluginpaths);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        label_19->setMaximumSize(QSize(48, 48));
        label_19->setText(QString::fromUtf8(""));
        label_19->setPixmap(QPixmap(QString::fromUtf8(":/scalable/folder.svgz")));
        label_19->setScaledContents(true);
        label_19->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
        label_19->setWordWrap(true);

        horizontalLayout_6->addWidget(label_19);


        verticalLayout_19->addLayout(horizontalLayout_6);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
        cb_paths = new QComboBox(page_pluginpaths);
        cb_paths->addItem(QString());
        cb_paths->addItem(QString());
        cb_paths->addItem(QString());
        cb_paths->addItem(QString());
        cb_paths->addItem(QString());
        cb_paths->addItem(QString());
        cb_paths->addItem(QString());
        cb_paths->setObjectName(QString::fromUtf8("cb_paths"));
        cb_paths->setMinimumSize(QSize(120, 0));

        horizontalLayout_9->addWidget(cb_paths);

        horizontalSpacer_17 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_17);


        verticalLayout_19->addLayout(horizontalLayout_9);

        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setObjectName(QString::fromUtf8("horizontalLayout_11"));
        tw_paths = new QStackedWidget(page_pluginpaths);
        tw_paths->setObjectName(QString::fromUtf8("tw_paths"));
        tw_paths_ladspa = new QWidget();
        tw_paths_ladspa->setObjectName(QString::fromUtf8("tw_paths_ladspa"));
        verticalLayout_11 = new QVBoxLayout(tw_paths_ladspa);
        verticalLayout_11->setSpacing(0);
        verticalLayout_11->setContentsMargins(0, 0, 0, 0);
        verticalLayout_11->setObjectName(QString::fromUtf8("verticalLayout_11"));
        lw_ladspa = new QListWidget(tw_paths_ladspa);
        lw_ladspa->setObjectName(QString::fromUtf8("lw_ladspa"));

        verticalLayout_11->addWidget(lw_ladspa);

        tw_paths->addWidget(tw_paths_ladspa);
        tw_paths_dssi = new QWidget();
        tw_paths_dssi->setObjectName(QString::fromUtf8("tw_paths_dssi"));
        verticalLayout_12 = new QVBoxLayout(tw_paths_dssi);
        verticalLayout_12->setSpacing(0);
        verticalLayout_12->setContentsMargins(0, 0, 0, 0);
        verticalLayout_12->setObjectName(QString::fromUtf8("verticalLayout_12"));
        lw_dssi = new QListWidget(tw_paths_dssi);
        lw_dssi->setObjectName(QString::fromUtf8("lw_dssi"));

        verticalLayout_12->addWidget(lw_dssi);

        tw_paths->addWidget(tw_paths_dssi);
        tw_paths_lv2 = new QWidget();
        tw_paths_lv2->setObjectName(QString::fromUtf8("tw_paths_lv2"));
        gridLayout = new QGridLayout(tw_paths_lv2);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 6);
        lw_lv2 = new QListWidget(tw_paths_lv2);
        lw_lv2->setObjectName(QString::fromUtf8("lw_lv2"));

        gridLayout->addWidget(lw_lv2, 0, 0, 1, 2);

        label_4 = new QLabel(tw_paths_lv2);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setMaximumSize(QSize(22, 22));
        label_4->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-information.svgz")));
        label_4->setScaledContents(true);
        label_4->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_4, 1, 0, 1, 1);

        label_5 = new QLabel(tw_paths_lv2);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        gridLayout->addWidget(label_5, 1, 1, 1, 1);

        tw_paths->addWidget(tw_paths_lv2);
        tw_paths_vst = new QWidget();
        tw_paths_vst->setObjectName(QString::fromUtf8("tw_paths_vst"));
        verticalLayout_13 = new QVBoxLayout(tw_paths_vst);
        verticalLayout_13->setSpacing(0);
        verticalLayout_13->setContentsMargins(0, 0, 0, 0);
        verticalLayout_13->setObjectName(QString::fromUtf8("verticalLayout_13"));
        lw_vst = new QListWidget(tw_paths_vst);
        lw_vst->setObjectName(QString::fromUtf8("lw_vst"));

        verticalLayout_13->addWidget(lw_vst);

        tw_paths->addWidget(tw_paths_vst);
        tw_paths_vst3 = new QWidget();
        tw_paths_vst3->setObjectName(QString::fromUtf8("tw_paths_vst3"));
        verticalLayout_15 = new QVBoxLayout(tw_paths_vst3);
        verticalLayout_15->setSpacing(0);
        verticalLayout_15->setContentsMargins(0, 0, 0, 0);
        verticalLayout_15->setObjectName(QString::fromUtf8("verticalLayout_15"));
        lw_vst3 = new QListWidget(tw_paths_vst3);
        lw_vst3->setObjectName(QString::fromUtf8("lw_vst3"));

        verticalLayout_15->addWidget(lw_vst3);

        tw_paths->addWidget(tw_paths_vst3);
        tw_paths_sf2 = new QWidget();
        tw_paths_sf2->setObjectName(QString::fromUtf8("tw_paths_sf2"));
        verticalLayout_4 = new QVBoxLayout(tw_paths_sf2);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        lw_sf2 = new QListWidget(tw_paths_sf2);
        lw_sf2->setObjectName(QString::fromUtf8("lw_sf2"));

        verticalLayout_4->addWidget(lw_sf2);

        tw_paths->addWidget(tw_paths_sf2);
        tw_paths_sfz = new QWidget();
        tw_paths_sfz->setObjectName(QString::fromUtf8("tw_paths_sfz"));
        verticalLayout_14 = new QVBoxLayout(tw_paths_sfz);
        verticalLayout_14->setSpacing(0);
        verticalLayout_14->setContentsMargins(0, 0, 0, 0);
        verticalLayout_14->setObjectName(QString::fromUtf8("verticalLayout_14"));
        lw_sfz = new QListWidget(tw_paths_sfz);
        lw_sfz->setObjectName(QString::fromUtf8("lw_sfz"));

        verticalLayout_14->addWidget(lw_sfz);

        tw_paths->addWidget(tw_paths_sfz);

        horizontalLayout_11->addWidget(tw_paths);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        b_paths_add = new QPushButton(page_pluginpaths);
        b_paths_add->setObjectName(QString::fromUtf8("b_paths_add"));
        b_paths_add->setIcon(icon8);

        verticalLayout->addWidget(b_paths_add);

        b_paths_remove = new QPushButton(page_pluginpaths);
        b_paths_remove->setObjectName(QString::fromUtf8("b_paths_remove"));
        b_paths_remove->setIcon(icon9);

        verticalLayout->addWidget(b_paths_remove);

        verticalSpacer_2 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout->addItem(verticalSpacer_2);

        b_paths_change = new QPushButton(page_pluginpaths);
        b_paths_change->setObjectName(QString::fromUtf8("b_paths_change"));
        b_paths_change->setIcon(icon10);

        verticalLayout->addWidget(b_paths_change);

        verticalSpacer_7 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_7);


        horizontalLayout_11->addLayout(verticalLayout);


        verticalLayout_19->addLayout(horizontalLayout_11);

        stackedWidget->addWidget(page_pluginpaths);
        page_wine = new QWidget();
        page_wine->setObjectName(QString::fromUtf8("page_wine"));
        verticalLayout_25 = new QVBoxLayout(page_wine);
        verticalLayout_25->setContentsMargins(2, 2, 2, 2);
        verticalLayout_25->setObjectName(QString::fromUtf8("verticalLayout_25"));
        horizontalLayout_17 = new QHBoxLayout();
        horizontalLayout_17->setObjectName(QString::fromUtf8("horizontalLayout_17"));
        label_29 = new QLabel(page_wine);
        label_29->setObjectName(QString::fromUtf8("label_29"));
        label_29->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        horizontalLayout_17->addWidget(label_29);

        horizontalSpacer_26 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_17->addItem(horizontalSpacer_26);

        label_icon_wine = new QLabel(page_wine);
        label_icon_wine->setObjectName(QString::fromUtf8("label_icon_wine"));
        label_icon_wine->setMaximumSize(QSize(48, 48));
        label_icon_wine->setText(QString::fromUtf8(""));
        label_icon_wine->setPixmap(QPixmap(QString::fromUtf8(":/scalable/wine.svgz")));
        label_icon_wine->setScaledContents(true);
        label_icon_wine->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        horizontalLayout_17->addWidget(label_icon_wine);


        verticalLayout_25->addLayout(horizontalLayout_17);

        group_wine_exec = new QGroupBox(page_wine);
        group_wine_exec->setObjectName(QString::fromUtf8("group_wine_exec"));
        horizontalLayout_18 = new QHBoxLayout(group_wine_exec);
        horizontalLayout_18->setObjectName(QString::fromUtf8("horizontalLayout_18"));
        label_8 = new QLabel(group_wine_exec);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_18->addWidget(label_8);

        le_wine_exec = new QLineEdit(group_wine_exec);
        le_wine_exec->setObjectName(QString::fromUtf8("le_wine_exec"));

        horizontalLayout_18->addWidget(le_wine_exec);


        verticalLayout_25->addWidget(group_wine_exec);

        group_wine_prefix = new QGroupBox(page_wine);
        group_wine_prefix->setObjectName(QString::fromUtf8("group_wine_prefix"));
        gridLayout_6 = new QGridLayout(group_wine_prefix);
        gridLayout_6->setObjectName(QString::fromUtf8("gridLayout_6"));
        cb_wine_prefix_detect = new QCheckBox(group_wine_prefix);
        cb_wine_prefix_detect->setObjectName(QString::fromUtf8("cb_wine_prefix_detect"));

        gridLayout_6->addWidget(cb_wine_prefix_detect, 0, 0, 1, 2);

        label = new QLabel(group_wine_prefix);
        label->setObjectName(QString::fromUtf8("label"));
        label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_6->addWidget(label, 1, 0, 1, 1);

        le_wine_prefix_fallback = new QLineEdit(group_wine_prefix);
        le_wine_prefix_fallback->setObjectName(QString::fromUtf8("le_wine_prefix_fallback"));

        gridLayout_6->addWidget(le_wine_prefix_fallback, 1, 1, 1, 1);

        label_30 = new QLabel(group_wine_prefix);
        label_30->setObjectName(QString::fromUtf8("label_30"));
        label_30->setAlignment(Qt::AlignCenter);

        gridLayout_6->addWidget(label_30, 2, 0, 1, 2);


        verticalLayout_25->addWidget(group_wine_prefix);

        group_wine_realtime = new QGroupBox(page_wine);
        group_wine_realtime->setObjectName(QString::fromUtf8("group_wine_realtime"));
        group_wine_realtime->setCheckable(true);
        gridLayout_5 = new QGridLayout(group_wine_realtime);
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        horizontalSpacer_28 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_5->addItem(horizontalSpacer_28, 1, 3, 1, 1);

        horizontalSpacer_27 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_5->addItem(horizontalSpacer_27, 0, 3, 1, 1);

        sb_wine_base_prio = new QSpinBox(group_wine_realtime);
        sb_wine_base_prio->setObjectName(QString::fromUtf8("sb_wine_base_prio"));
        sb_wine_base_prio->setMinimum(1);
        sb_wine_base_prio->setMaximum(89);

        gridLayout_5->addWidget(sb_wine_base_prio, 0, 2, 1, 1);

        label_2 = new QLabel(group_wine_realtime);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_5->addWidget(label_2, 0, 1, 1, 1);

        label_3 = new QLabel(group_wine_realtime);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_5->addWidget(label_3, 1, 1, 1, 1);

        sb_wine_server_prio = new QSpinBox(group_wine_realtime);
        sb_wine_server_prio->setObjectName(QString::fromUtf8("sb_wine_server_prio"));
        sb_wine_server_prio->setMinimum(1);
        sb_wine_server_prio->setMaximum(99);

        gridLayout_5->addWidget(sb_wine_server_prio, 1, 2, 1, 1);

        horizontalSpacer_29 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_5->addItem(horizontalSpacer_29, 0, 0, 1, 1);

        horizontalSpacer_30 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_5->addItem(horizontalSpacer_30, 1, 0, 1, 1);


        verticalLayout_25->addWidget(group_wine_realtime);

        horizontalLayout_14 = new QHBoxLayout();
        horizontalLayout_14->setObjectName(QString::fromUtf8("horizontalLayout_14"));
        horizontalSpacer_24 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_14->addItem(horizontalSpacer_24);

        label_26 = new QLabel(page_wine);
        label_26->setObjectName(QString::fromUtf8("label_26"));
        label_26->setMaximumSize(QSize(22, 22));
        label_26->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-information.svgz")));
        label_26->setScaledContents(true);
        label_26->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_14->addWidget(label_26);

        label_27 = new QLabel(page_wine);
        label_27->setObjectName(QString::fromUtf8("label_27"));

        horizontalLayout_14->addWidget(label_27);

        horizontalSpacer_25 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_14->addItem(horizontalSpacer_25);


        verticalLayout_25->addLayout(horizontalLayout_14);

        verticalSpacer_5 = new QSpacerItem(20, 96, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_25->addItem(verticalSpacer_5);

        label_28 = new QLabel(page_wine);
        label_28->setObjectName(QString::fromUtf8("label_28"));
        label_28->setMaximumSize(QSize(16777215, 5));

        verticalLayout_25->addWidget(label_28);

        stackedWidget->addWidget(page_wine);
        page_experimental = new QWidget();
        page_experimental->setObjectName(QString::fromUtf8("page_experimental"));
        verticalLayout_21 = new QVBoxLayout(page_experimental);
        verticalLayout_21->setContentsMargins(2, 2, 2, 2);
        verticalLayout_21->setObjectName(QString::fromUtf8("verticalLayout_21"));
        horizontalLayout_15 = new QHBoxLayout();
        horizontalLayout_15->setObjectName(QString::fromUtf8("horizontalLayout_15"));
        label_11 = new QLabel(page_experimental);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        horizontalLayout_15->addWidget(label_11);

        horizontalSpacer_22 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_15->addItem(horizontalSpacer_22);

        label_icon_experimental = new QLabel(page_experimental);
        label_icon_experimental->setObjectName(QString::fromUtf8("label_icon_experimental"));
        label_icon_experimental->setMaximumSize(QSize(48, 48));
        label_icon_experimental->setText(QString::fromUtf8(""));
        label_icon_experimental->setPixmap(QPixmap(QString::fromUtf8(":/scalable/warning.svgz")));
        label_icon_experimental->setScaledContents(true);
        label_icon_experimental->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        horizontalLayout_15->addWidget(label_icon_experimental);


        verticalLayout_21->addLayout(horizontalLayout_15);

        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setObjectName(QString::fromUtf8("horizontalLayout_12"));
        horizontalSpacer_18 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_12->addItem(horizontalSpacer_18);

        label_16 = new QLabel(page_experimental);
        label_16->setObjectName(QString::fromUtf8("label_16"));
        label_16->setMaximumSize(QSize(22, 22));
        label_16->setPixmap(QPixmap(QString::fromUtf8(":/16x16/dialog-warning.svgz")));
        label_16->setScaledContents(true);
        label_16->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        label_16->setWordWrap(true);

        horizontalLayout_12->addWidget(label_16);

        label_17 = new QLabel(page_experimental);
        label_17->setObjectName(QString::fromUtf8("label_17"));

        horizontalLayout_12->addWidget(label_17);

        horizontalSpacer_19 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_12->addItem(horizontalSpacer_19);


        verticalLayout_21->addLayout(horizontalLayout_12);

        group_experimental_main = new QGroupBox(page_experimental);
        group_experimental_main->setObjectName(QString::fromUtf8("group_experimental_main"));
        verticalLayout_17 = new QVBoxLayout(group_experimental_main);
        verticalLayout_17->setObjectName(QString::fromUtf8("verticalLayout_17"));
        cb_exp_plugin_bridges = new QCheckBox(group_experimental_main);
        cb_exp_plugin_bridges->setObjectName(QString::fromUtf8("cb_exp_plugin_bridges"));

        verticalLayout_17->addWidget(cb_exp_plugin_bridges);

        ch_exp_wine_bridges = new QCheckBox(group_experimental_main);
        ch_exp_wine_bridges->setObjectName(QString::fromUtf8("ch_exp_wine_bridges"));
        ch_exp_wine_bridges->setEnabled(false);

        verticalLayout_17->addWidget(ch_exp_wine_bridges);

        ch_exp_jack_apps = new QCheckBox(group_experimental_main);
        ch_exp_jack_apps->setObjectName(QString::fromUtf8("ch_exp_jack_apps"));

        verticalLayout_17->addWidget(ch_exp_jack_apps);

        ch_exp_export_lv2 = new QCheckBox(group_experimental_main);
        ch_exp_export_lv2->setObjectName(QString::fromUtf8("ch_exp_export_lv2"));

        verticalLayout_17->addWidget(ch_exp_export_lv2);

        ch_exp_load_lib_global = new QCheckBox(group_experimental_main);
        ch_exp_load_lib_global->setObjectName(QString::fromUtf8("ch_exp_load_lib_global"));

        verticalLayout_17->addWidget(ch_exp_load_lib_global);


        verticalLayout_21->addWidget(group_experimental_main);

        group_experimental_canvas = new QGroupBox(page_experimental);
        group_experimental_canvas->setObjectName(QString::fromUtf8("group_experimental_canvas"));
        verticalLayout_16 = new QVBoxLayout(group_experimental_canvas);
        verticalLayout_16->setObjectName(QString::fromUtf8("verticalLayout_16"));
        cb_canvas_fancy_eyecandy = new QCheckBox(group_experimental_canvas);
        cb_canvas_fancy_eyecandy->setObjectName(QString::fromUtf8("cb_canvas_fancy_eyecandy"));

        verticalLayout_16->addWidget(cb_canvas_fancy_eyecandy);

        cb_canvas_use_opengl = new QCheckBox(group_experimental_canvas);
        cb_canvas_use_opengl->setObjectName(QString::fromUtf8("cb_canvas_use_opengl"));

        verticalLayout_16->addWidget(cb_canvas_use_opengl);

        cb_canvas_render_hq_aa = new QCheckBox(group_experimental_canvas);
        cb_canvas_render_hq_aa->setObjectName(QString::fromUtf8("cb_canvas_render_hq_aa"));
        cb_canvas_render_hq_aa->setEnabled(false);

        verticalLayout_16->addWidget(cb_canvas_render_hq_aa);

        cb_canvas_inline_displays = new QCheckBox(group_experimental_canvas);
        cb_canvas_inline_displays->setObjectName(QString::fromUtf8("cb_canvas_inline_displays"));

        verticalLayout_16->addWidget(cb_canvas_inline_displays);


        verticalLayout_21->addWidget(group_experimental_canvas);

        group_experimental_engine = new QGroupBox(page_experimental);
        group_experimental_engine->setObjectName(QString::fromUtf8("group_experimental_engine"));
        verticalLayout_20 = new QVBoxLayout(group_experimental_engine);
        verticalLayout_20->setObjectName(QString::fromUtf8("verticalLayout_20"));
        ch_engine_force_stereo = new QCheckBox(group_experimental_engine);
        ch_engine_force_stereo->setObjectName(QString::fromUtf8("ch_engine_force_stereo"));

        verticalLayout_20->addWidget(ch_engine_force_stereo);

        ch_exp_prevent_bad_behaviour = new QCheckBox(group_experimental_engine);
        ch_exp_prevent_bad_behaviour->setObjectName(QString::fromUtf8("ch_exp_prevent_bad_behaviour"));

        verticalLayout_20->addWidget(ch_exp_prevent_bad_behaviour);

        ch_engine_prefer_plugin_bridges = new QCheckBox(group_experimental_engine);
        ch_engine_prefer_plugin_bridges->setObjectName(QString::fromUtf8("ch_engine_prefer_plugin_bridges"));
        ch_engine_prefer_plugin_bridges->setEnabled(false);

        verticalLayout_20->addWidget(ch_engine_prefer_plugin_bridges);


        verticalLayout_21->addWidget(group_experimental_engine);

        verticalSpacer_4 = new QSpacerItem(20, 70, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_21->addItem(verticalSpacer_4);

        label_25 = new QLabel(page_experimental);
        label_25->setObjectName(QString::fromUtf8("label_25"));
        label_25->setMaximumSize(QSize(16777215, 5));

        verticalLayout_21->addWidget(label_25);

        stackedWidget->addWidget(page_experimental);
        page = new QWidget();
        page->setObjectName(QString::fromUtf8("page"));
        stackedWidget->addWidget(page);

        horizontalLayout_3->addWidget(stackedWidget);


        verticalLayout_2->addLayout(horizontalLayout_3);

        buttonBox = new QDialogButtonBox(CarlaSettingsW);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok|QDialogButtonBox::Reset);

        verticalLayout_2->addWidget(buttonBox);


        retranslateUi(CarlaSettingsW);
        QObject::connect(buttonBox, SIGNAL(accepted()), CarlaSettingsW, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), CarlaSettingsW, SLOT(reject()));
        QObject::connect(lw_page, SIGNAL(currentCellChanged(int,int,int,int)), stackedWidget, SLOT(setCurrentIndex(int)));
        QObject::connect(ch_main_theme_pro, SIGNAL(toggled(bool)), label_main_theme_color, SLOT(setEnabled(bool)));
        QObject::connect(ch_main_theme_pro, SIGNAL(toggled(bool)), cb_main_theme_color, SLOT(setEnabled(bool)));
        QObject::connect(cb_paths, SIGNAL(currentIndexChanged(int)), tw_paths, SLOT(setCurrentIndex(int)));
        QObject::connect(cb_canvas_use_opengl, SIGNAL(toggled(bool)), cb_canvas_render_hq_aa, SLOT(setEnabled(bool)));
        QObject::connect(cb_exp_plugin_bridges, SIGNAL(toggled(bool)), ch_exp_wine_bridges, SLOT(setEnabled(bool)));
        QObject::connect(cb_exp_plugin_bridges, SIGNAL(toggled(bool)), ch_engine_prefer_plugin_bridges, SLOT(setEnabled(bool)));
        QObject::connect(rb_osc_tcp_port_specific, SIGNAL(toggled(bool)), sb_osc_tcp_port_number, SLOT(setEnabled(bool)));
        QObject::connect(rb_osc_udp_port_specific, SIGNAL(toggled(bool)), sb_osc_udp_port_number, SLOT(setEnabled(bool)));
        QObject::connect(cb_filepaths, SIGNAL(currentIndexChanged(int)), tw_filepaths, SLOT(setCurrentIndex(int)));
        QObject::connect(cb_filepaths, SIGNAL(currentIndexChanged(int)), tw_filepaths_info, SLOT(setCurrentIndex(int)));

        stackedWidget->setCurrentIndex(0);
        sw_engine_process_mode->setCurrentIndex(1);
        cb_engine_process_mode_jack->setCurrentIndex(0);
        tw_filepaths_info->setCurrentIndex(0);
        tw_filepaths->setCurrentIndex(0);
        tw_paths->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(CarlaSettingsW);
    } // setupUi

    void retranslateUi(QDialog *CarlaSettingsW)
    {
        CarlaSettingsW->setWindowTitle(QCoreApplication::translate("CarlaSettingsW", "Settings", nullptr));
        QTableWidgetItem *___qtablewidgetitem = lw_page->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("CarlaSettingsW", "Widget", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = lw_page->verticalHeaderItem(0);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("CarlaSettingsW", "main", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = lw_page->verticalHeaderItem(1);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("CarlaSettingsW", "canvas", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = lw_page->verticalHeaderItem(2);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("CarlaSettingsW", "engine", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = lw_page->verticalHeaderItem(3);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("CarlaSettingsW", "osc", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = lw_page->verticalHeaderItem(4);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("CarlaSettingsW", "file-paths", nullptr));
        QTableWidgetItem *___qtablewidgetitem6 = lw_page->verticalHeaderItem(5);
        ___qtablewidgetitem6->setText(QCoreApplication::translate("CarlaSettingsW", "plugin-paths", nullptr));
        QTableWidgetItem *___qtablewidgetitem7 = lw_page->verticalHeaderItem(6);
        ___qtablewidgetitem7->setText(QCoreApplication::translate("CarlaSettingsW", "wine", nullptr));
        QTableWidgetItem *___qtablewidgetitem8 = lw_page->verticalHeaderItem(7);
        ___qtablewidgetitem8->setText(QCoreApplication::translate("CarlaSettingsW", "experimental", nullptr));

        const bool __sortingEnabled = lw_page->isSortingEnabled();
        lw_page->setSortingEnabled(false);
        QTableWidgetItem *___qtablewidgetitem9 = lw_page->item(0, 0);
        ___qtablewidgetitem9->setText(QCoreApplication::translate("CarlaSettingsW", "Main", nullptr));
        QTableWidgetItem *___qtablewidgetitem10 = lw_page->item(1, 0);
        ___qtablewidgetitem10->setText(QCoreApplication::translate("CarlaSettingsW", "Canvas", nullptr));
        QTableWidgetItem *___qtablewidgetitem11 = lw_page->item(2, 0);
        ___qtablewidgetitem11->setText(QCoreApplication::translate("CarlaSettingsW", "Engine", nullptr));
        QTableWidgetItem *___qtablewidgetitem12 = lw_page->item(4, 0);
        ___qtablewidgetitem12->setText(QCoreApplication::translate("CarlaSettingsW", "File Paths", nullptr));
        QTableWidgetItem *___qtablewidgetitem13 = lw_page->item(5, 0);
        ___qtablewidgetitem13->setText(QCoreApplication::translate("CarlaSettingsW", "Plugin Paths", nullptr));
        QTableWidgetItem *___qtablewidgetitem14 = lw_page->item(6, 0);
        ___qtablewidgetitem14->setText(QCoreApplication::translate("CarlaSettingsW", "Wine", nullptr));
        QTableWidgetItem *___qtablewidgetitem15 = lw_page->item(7, 0);
        ___qtablewidgetitem15->setText(QCoreApplication::translate("CarlaSettingsW", "Experimental", nullptr));
        lw_page->setSortingEnabled(__sortingEnabled);

        label_main->setText(QCoreApplication::translate("CarlaSettingsW", "<b>Main</b>", nullptr));
        group_main_paths->setTitle(QCoreApplication::translate("CarlaSettingsW", "Paths", nullptr));
        label_main_proj_folder_open->setText(QCoreApplication::translate("CarlaSettingsW", "Default project folder:", nullptr));
        b_main_proj_folder_open->setText(QString());
        group_main_misc->setTitle(QCoreApplication::translate("CarlaSettingsW", "Interface", nullptr));
        label_main_refresh_interval->setText(QCoreApplication::translate("CarlaSettingsW", "Interface refresh interval:", nullptr));
        sb_main_refresh_interval->setSuffix(QCoreApplication::translate("CarlaSettingsW", " ms", nullptr));
        ch_main_show_logs->setText(QCoreApplication::translate("CarlaSettingsW", "Show console output in Logs tab (needs engine restart)", nullptr));
        ch_main_confirm_exit->setText(QCoreApplication::translate("CarlaSettingsW", "Show a confirmation dialog before quitting", nullptr));
        group_main_theme->setTitle(QCoreApplication::translate("CarlaSettingsW", "Theme", nullptr));
        ch_main_theme_pro->setText(QCoreApplication::translate("CarlaSettingsW", "Use Carla \"PRO\" theme (needs restart)", nullptr));
        label_main_theme_color->setText(QCoreApplication::translate("CarlaSettingsW", "Color scheme:", nullptr));
        cb_main_theme_color->setItemText(0, QCoreApplication::translate("CarlaSettingsW", "Black", nullptr));
        cb_main_theme_color->setItemText(1, QCoreApplication::translate("CarlaSettingsW", "System", nullptr));

        group_main_experimental->setTitle(QCoreApplication::translate("CarlaSettingsW", "Experimental", nullptr));
        ch_main_experimental->setText(QCoreApplication::translate("CarlaSettingsW", "Enable experimental features", nullptr));
        label_9->setText(QCoreApplication::translate("CarlaSettingsW", "<b>Canvas</b>", nullptr));
        group_canvas_theme->setTitle(QCoreApplication::translate("CarlaSettingsW", "Theme", nullptr));
        cb_canvas_bezier_lines->setText(QCoreApplication::translate("CarlaSettingsW", "Bezier Lines", nullptr));
        label_canvas_theme->setText(QCoreApplication::translate("CarlaSettingsW", "Theme:", nullptr));
        label_canvas_size->setText(QCoreApplication::translate("CarlaSettingsW", "Size:", nullptr));
        cb_canvas_size->setItemText(0, QCoreApplication::translate("CarlaSettingsW", "775x600", nullptr));
        cb_canvas_size->setItemText(1, QCoreApplication::translate("CarlaSettingsW", "1550x1200", nullptr));
        cb_canvas_size->setItemText(2, QCoreApplication::translate("CarlaSettingsW", "3100x2400", nullptr));
        cb_canvas_size->setItemText(3, QCoreApplication::translate("CarlaSettingsW", "4650x3600", nullptr));
        cb_canvas_size->setItemText(4, QCoreApplication::translate("CarlaSettingsW", "6200x4800", nullptr));

        group_canvas_options->setTitle(QCoreApplication::translate("CarlaSettingsW", "Options", nullptr));
        cb_canvas_hide_groups->setText(QCoreApplication::translate("CarlaSettingsW", "Auto-hide groups with no ports", nullptr));
        cb_canvas_auto_select->setText(QCoreApplication::translate("CarlaSettingsW", "Auto-select items on hover", nullptr));
        cb_canvas_eyecandy->setText(QCoreApplication::translate("CarlaSettingsW", "Basic eye-candy (group shadows)", nullptr));
        group_canvas_render->setTitle(QCoreApplication::translate("CarlaSettingsW", "Render Hints", nullptr));
        cb_canvas_render_aa->setText(QCoreApplication::translate("CarlaSettingsW", "Anti-Aliasing", nullptr));
        cb_canvas_full_repaints->setText(QCoreApplication::translate("CarlaSettingsW", "Full canvas repaints (slower, but prevents drawing issues)", nullptr));
        label_6->setText(QCoreApplication::translate("CarlaSettingsW", "<b>Engine</b>", nullptr));
        groupBox->setTitle(QCoreApplication::translate("CarlaSettingsW", "Core", nullptr));
        cb_engine_process_mode_jack->setItemText(0, QCoreApplication::translate("CarlaSettingsW", "Single Client", nullptr));
        cb_engine_process_mode_jack->setItemText(1, QCoreApplication::translate("CarlaSettingsW", "Multiple Clients", nullptr));
        cb_engine_process_mode_jack->setItemText(2, QCoreApplication::translate("CarlaSettingsW", "Continuous Rack", nullptr));
        cb_engine_process_mode_jack->setItemText(3, QCoreApplication::translate("CarlaSettingsW", "Patchbay", nullptr));

        cb_engine_process_mode_other->setItemText(0, QCoreApplication::translate("CarlaSettingsW", "Continuous Rack", nullptr));
        cb_engine_process_mode_other->setItemText(1, QCoreApplication::translate("CarlaSettingsW", "Patchbay", nullptr));

        label_21->setText(QCoreApplication::translate("CarlaSettingsW", "Audio driver:", nullptr));
        label_20->setText(QCoreApplication::translate("CarlaSettingsW", "Process mode:", nullptr));
#if QT_CONFIG(tooltip)
        label_7->setToolTip(QCoreApplication::translate("CarlaSettingsW", "Maximum number of parameters to allow in the built-in 'Edit' dialog", nullptr));
#endif // QT_CONFIG(tooltip)
        label_7->setText(QCoreApplication::translate("CarlaSettingsW", "Max Parameters:", nullptr));
#if QT_CONFIG(tooltip)
        sb_engine_max_params->setToolTip(QCoreApplication::translate("CarlaSettingsW", "Maximum number of parameters to allow in the built-in 'Edit' dialog", nullptr));
#endif // QT_CONFIG(tooltip)
        tb_engine_driver_config->setText(QCoreApplication::translate("CarlaSettingsW", "...", nullptr));
        groupBox_8->setTitle(QCoreApplication::translate("CarlaSettingsW", "Plugin UIs", nullptr));
#if QT_CONFIG(tooltip)
        label_engine_ui_bridges_timeout->setToolTip(QCoreApplication::translate("CarlaSettingsW", "How much time to wait for OSC GUIs to ping back the host", nullptr));
#endif // QT_CONFIG(tooltip)
        label_engine_ui_bridges_timeout->setText(QCoreApplication::translate("CarlaSettingsW", "UI Bridge Timeout:", nullptr));
#if QT_CONFIG(tooltip)
        sb_engine_ui_bridges_timeout->setToolTip(QCoreApplication::translate("CarlaSettingsW", "How much time to wait for OSC GUIs to ping back the host", nullptr));
#endif // QT_CONFIG(tooltip)
        sb_engine_ui_bridges_timeout->setSuffix(QCoreApplication::translate("CarlaSettingsW", " ms", nullptr));
#if QT_CONFIG(tooltip)
        ch_engine_prefer_ui_bridges->setToolTip(QCoreApplication::translate("CarlaSettingsW", "Use OSC-GUI bridges when possible, this way separating the UI from DSP code", nullptr));
#endif // QT_CONFIG(tooltip)
        ch_engine_prefer_ui_bridges->setText(QCoreApplication::translate("CarlaSettingsW", "Use UI bridges instead of direct handling when possible", nullptr));
        ch_engine_uis_always_on_top->setText(QCoreApplication::translate("CarlaSettingsW", "Make plugin UIs always-on-top", nullptr));
        ch_engine_manage_uis->setText(QCoreApplication::translate("CarlaSettingsW", "Make plugin UIs appear on top of Carla (needs restart)", nullptr));
        label_engine_ui_bridges_mac_note->setText(QCoreApplication::translate("CarlaSettingsW", "NOTE: Plugin-bridge UIs cannot be managed by Carla on macOS", nullptr));
        label_15->setText(QString());
        label_22->setText(QString());
        label_23->setText(QCoreApplication::translate("CarlaSettingsW", "Restart the engine to load the new settings", nullptr));
        label_14->setText(QCoreApplication::translate("CarlaSettingsW", "<b>OSC</b>", nullptr));
        group_osc_core->setTitle(QCoreApplication::translate("CarlaSettingsW", "Core", nullptr));
        ch_osc_enable->setText(QCoreApplication::translate("CarlaSettingsW", "Enable OSC", nullptr));
        group_osc_tcp_port->setTitle(QCoreApplication::translate("CarlaSettingsW", "Enable TCP port", nullptr));
        rb_osc_tcp_port_specific->setText(QCoreApplication::translate("CarlaSettingsW", "Use specific port:", nullptr));
        label_39->setText(QString());
        label_40->setText(QCoreApplication::translate("CarlaSettingsW", "Overridden by CARLA_OSC_TCP_PORT env var", nullptr));
#if QT_CONFIG(tooltip)
        sb_osc_tcp_port_number->setToolTip(QCoreApplication::translate("CarlaSettingsW", "Maximum number of parameters to allow in the built-in 'Edit' dialog", nullptr));
#endif // QT_CONFIG(tooltip)
        rb_osc_tcp_port_random->setText(QCoreApplication::translate("CarlaSettingsW", "Use randomly assigned port", nullptr));
        group_osc_udp_port->setTitle(QCoreApplication::translate("CarlaSettingsW", "Enable UDP port", nullptr));
        rb_osc_udp_port_specific->setText(QCoreApplication::translate("CarlaSettingsW", "Use specific port:", nullptr));
#if QT_CONFIG(tooltip)
        sb_osc_udp_port_number->setToolTip(QCoreApplication::translate("CarlaSettingsW", "Maximum number of parameters to allow in the built-in 'Edit' dialog", nullptr));
#endif // QT_CONFIG(tooltip)
        rb_osc_udp_port_random->setText(QCoreApplication::translate("CarlaSettingsW", "Use randomly assigned port", nullptr));
        label_41->setText(QString());
        label_42->setText(QCoreApplication::translate("CarlaSettingsW", "Overridden by CARLA_OSC_UDP_PORT env var", nullptr));
        label_36->setText(QString());
        label_32->setText(QString());
        label_35->setText(QCoreApplication::translate("CarlaSettingsW", "Restart the engine to load the new settings", nullptr));
        label_37->setText(QString());
        label_38->setText(QCoreApplication::translate("CarlaSettingsW", "DSSI UIs require OSC UDP port enabled", nullptr));
        label_24->setText(QCoreApplication::translate("CarlaSettingsW", "<b>File Paths</b>", nullptr));
        cb_filepaths->setItemText(0, QCoreApplication::translate("CarlaSettingsW", "Audio", nullptr));
        cb_filepaths->setItemText(1, QCoreApplication::translate("CarlaSettingsW", "MIDI", nullptr));

        label_12->setText(QCoreApplication::translate("CarlaSettingsW", "Used for the \"audiofile\" plugin", nullptr));
        label_13->setText(QCoreApplication::translate("CarlaSettingsW", "Used for the \"midifile\" plugin", nullptr));
        b_filepaths_add->setText(QCoreApplication::translate("CarlaSettingsW", "Add...", nullptr));
        b_filepaths_remove->setText(QCoreApplication::translate("CarlaSettingsW", "Remove", nullptr));
        b_filepaths_change->setText(QCoreApplication::translate("CarlaSettingsW", "Change...", nullptr));
        label_18->setText(QCoreApplication::translate("CarlaSettingsW", "<b>Plugin Paths</b>", nullptr));
        cb_paths->setItemText(0, QCoreApplication::translate("CarlaSettingsW", "LADSPA", nullptr));
        cb_paths->setItemText(1, QCoreApplication::translate("CarlaSettingsW", "DSSI", nullptr));
        cb_paths->setItemText(2, QCoreApplication::translate("CarlaSettingsW", "LV2", nullptr));
        cb_paths->setItemText(3, QCoreApplication::translate("CarlaSettingsW", "VST2", nullptr));
        cb_paths->setItemText(4, QCoreApplication::translate("CarlaSettingsW", "VST3", nullptr));
        cb_paths->setItemText(5, QCoreApplication::translate("CarlaSettingsW", "SF2/3", nullptr));
        cb_paths->setItemText(6, QCoreApplication::translate("CarlaSettingsW", "SFZ", nullptr));

        label_4->setText(QString());
        label_5->setText(QCoreApplication::translate("CarlaSettingsW", "Restart Carla to find new plugins", nullptr));
        b_paths_add->setText(QCoreApplication::translate("CarlaSettingsW", "Add...", nullptr));
        b_paths_remove->setText(QCoreApplication::translate("CarlaSettingsW", "Remove", nullptr));
        b_paths_change->setText(QCoreApplication::translate("CarlaSettingsW", "Change...", nullptr));
        label_29->setText(QCoreApplication::translate("CarlaSettingsW", "<b>Wine</b>", nullptr));
        group_wine_exec->setTitle(QCoreApplication::translate("CarlaSettingsW", "Executable", nullptr));
        label_8->setText(QCoreApplication::translate("CarlaSettingsW", "Path to 'wine' binary:", nullptr));
        group_wine_prefix->setTitle(QCoreApplication::translate("CarlaSettingsW", "Prefix", nullptr));
        cb_wine_prefix_detect->setText(QCoreApplication::translate("CarlaSettingsW", "Auto-detect Wine prefix based on plugin filename", nullptr));
        label->setText(QCoreApplication::translate("CarlaSettingsW", "Fallback:", nullptr));
        label_30->setText(QCoreApplication::translate("CarlaSettingsW", "Note: WINEPREFIX env var is preferred over this fallback", nullptr));
        group_wine_realtime->setTitle(QCoreApplication::translate("CarlaSettingsW", "Realtime Priority", nullptr));
        label_2->setText(QCoreApplication::translate("CarlaSettingsW", "Base priority:", nullptr));
        label_3->setText(QCoreApplication::translate("CarlaSettingsW", "WineServer priority:", nullptr));
        label_26->setText(QString());
        label_27->setText(QCoreApplication::translate("CarlaSettingsW", "These options are not available for Carla as plugin", nullptr));
        label_28->setText(QString());
        label_11->setText(QCoreApplication::translate("CarlaSettingsW", "<b>Experimental</b>", nullptr));
        label_16->setText(QString());
        label_17->setText(QCoreApplication::translate("CarlaSettingsW", "Experimental options! Likely to be unstable!", nullptr));
        group_experimental_main->setTitle(QCoreApplication::translate("CarlaSettingsW", "Main", nullptr));
        cb_exp_plugin_bridges->setText(QCoreApplication::translate("CarlaSettingsW", "Enable plugin bridges", nullptr));
        ch_exp_wine_bridges->setText(QCoreApplication::translate("CarlaSettingsW", "Enable Wine bridges", nullptr));
        ch_exp_jack_apps->setText(QCoreApplication::translate("CarlaSettingsW", "Enable jack applications", nullptr));
        ch_exp_export_lv2->setText(QCoreApplication::translate("CarlaSettingsW", "Export single plugins to LV2", nullptr));
        ch_exp_load_lib_global->setText(QCoreApplication::translate("CarlaSettingsW", "Load Carla backend in global namespace (NOT RECOMMENDED)", nullptr));
        group_experimental_canvas->setTitle(QCoreApplication::translate("CarlaSettingsW", "Canvas", nullptr));
        cb_canvas_fancy_eyecandy->setText(QCoreApplication::translate("CarlaSettingsW", "Fancy eye-candy (fade-in/out groups, glow connections)", nullptr));
        cb_canvas_use_opengl->setText(QCoreApplication::translate("CarlaSettingsW", "Use OpenGL for rendering (needs restart)", nullptr));
        cb_canvas_render_hq_aa->setText(QCoreApplication::translate("CarlaSettingsW", "High Quality Anti-Aliasing (OpenGL only)", nullptr));
        cb_canvas_inline_displays->setText(QCoreApplication::translate("CarlaSettingsW", "Render Ardour-style \"Inline Displays\"", nullptr));
        group_experimental_engine->setTitle(QCoreApplication::translate("CarlaSettingsW", "Engine", nullptr));
#if QT_CONFIG(tooltip)
        ch_engine_force_stereo->setToolTip(QCoreApplication::translate("CarlaSettingsW", "Force mono plugins as stereo by running 2 instances at the same time.\n"
"This mode is not available for VST plugins.", nullptr));
#endif // QT_CONFIG(tooltip)
        ch_engine_force_stereo->setText(QCoreApplication::translate("CarlaSettingsW", "Force mono plugins as stereo", nullptr));
        ch_exp_prevent_bad_behaviour->setText(QCoreApplication::translate("CarlaSettingsW", "Prevent plugins from doing bad stuff (needs restart)", nullptr));
#if QT_CONFIG(tooltip)
        ch_engine_prefer_plugin_bridges->setToolTip(QCoreApplication::translate("CarlaSettingsW", "Whenever possible, run the plugins in bridge mode.", nullptr));
#endif // QT_CONFIG(tooltip)
        ch_engine_prefer_plugin_bridges->setText(QCoreApplication::translate("CarlaSettingsW", "Run plugins in bridge mode when possible", nullptr));
        label_25->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class CarlaSettingsW: public Ui_CarlaSettingsW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_SETTINGS_H
