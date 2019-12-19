/********************************************************************************
** Form generated from reading UI file 'carla_host.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CARLA_HOST_H
#define UI_CARLA_HOST_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "widgets/canvaspreviewframe.h"
#include "widgets/digitalpeakmeter.h"
#include "widgets/draggablegraphicsview.h"
#include "widgets/racklistwidget.h"

QT_BEGIN_NAMESPACE

class Ui_CarlaHostW
{
public:
    QAction *act_file_new;
    QAction *act_file_open;
    QAction *act_file_save;
    QAction *act_file_save_as;
    QAction *act_file_quit;
    QAction *act_engine_start;
    QAction *act_engine_stop;
    QAction *act_plugin_add;
    QAction *act_plugin_remove_all;
    QAction *act_plugins_enable;
    QAction *act_plugins_disable;
    QAction *act_plugins_bypass;
    QAction *act_plugins_wet100;
    QAction *act_plugins_mute;
    QAction *act_plugins_volume100;
    QAction *act_plugins_center;
    QAction *act_transport_play;
    QAction *act_transport_stop;
    QAction *act_transport_backwards;
    QAction *act_transport_forwards;
    QAction *act_canvas_arrange;
    QAction *act_canvas_refresh;
    QAction *act_canvas_save_image;
    QAction *act_canvas_zoom_fit;
    QAction *act_canvas_zoom_in;
    QAction *act_canvas_zoom_out;
    QAction *act_canvas_zoom_100;
    QAction *act_settings_show_toolbar;
    QAction *act_settings_configure;
    QAction *act_help_about;
    QAction *act_help_about_juce;
    QAction *act_help_about_qt;
    QAction *act_settings_show_meters;
    QAction *act_settings_show_keyboard;
    QAction *act_canvas_show_internal;
    QAction *act_canvas_show_external;
    QAction *act_settings_show_time_panel;
    QAction *act_settings_show_side_panel;
    QAction *act_file_connect;
    QAction *act_file_refresh;
    QAction *act_plugins_compact;
    QAction *act_plugins_expand;
    QAction *act_secret_1;
    QAction *act_secret_2;
    QAction *act_secret_3;
    QAction *act_secret_4;
    QAction *act_secret_5;
    QAction *act_plugin_add_jack;
    QAction *act_engine_config;
    QAction *act_engine_panic;
    QAction *act_engine_panel;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout_4;
    QTabWidget *tabWidget;
    QWidget *rack;
    QHBoxLayout *horizontalLayout_2;
    QFrame *frame;
    QHBoxLayout *horizontalLayout_10;
    QLabel *pad_left;
    RackListWidget *listWidget;
    QLabel *pad_right;
    QScrollBar *rackScrollBar;
    QWidget *patchbay;
    QGridLayout *gridLayout;
    DraggableGraphicsView *graphicsView;
    DigitalPeakMeter *peak_out;
    DigitalPeakMeter *peak_in;
    QWidget *logs;
    QHBoxLayout *horizontalLayout_4;
    QPlainTextEdit *text_logs;
    QFrame *tw_statusbar;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_buffer_size;
    QComboBox *cb_buffer_size;
    QFrame *line_1;
    QLabel *label_sample_rate;
    QComboBox *cb_sample_rate;
    QFrame *line_2;
    QPushButton *b_xruns;
    QFrame *line_4;
    QProgressBar *pb_dsp_load;
    QMenuBar *menubar;
    QMenu *menu_File;
    QMenu *menu_Engine;
    QMenu *menu_Plugin;
    QMenu *menu_PluginMacros;
    QMenu *menu_Canvas;
    QMenu *menu_Canvas_Zoom;
    QMenu *menu_Settings;
    QMenu *menu_Help;
    QToolBar *toolBar;
    QDockWidget *dockWidget;
    QWidget *dockWidgetContents;
    QVBoxLayout *verticalLayout;
    QSplitter *splitter;
    QTabWidget *tabUtils;
    QWidget *disk;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout;
    QComboBox *cb_disk;
    QToolButton *b_disk_add;
    QToolButton *b_disk_remove;
    QTreeView *fileTreeView;
    QWidget *w_transport;
    QVBoxLayout *verticalLayout_6;
    QGroupBox *group_transport_controls;
    QHBoxLayout *horizontalLayout_5;
    QPushButton *b_transport_play;
    QPushButton *b_transport_stop;
    QPushButton *b_transport_backwards;
    QPushButton *b_transport_forwards;
    QGroupBox *group_transport_info;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_transport_frame;
    QLabel *l_transport_frame;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label_transport_time;
    QLabel *l_transport_time;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout_8;
    QLabel *label_transport_bbt;
    QLabel *l_transport_bbt;
    QSpacerItem *horizontalSpacer_3;
    QGroupBox *group_transport_settings;
    QVBoxLayout *verticalLayout_5;
    QDoubleSpinBox *dsb_transport_bpm;
    QCheckBox *cb_transport_jack;
    QCheckBox *cb_transport_link;
    QSpacerItem *verticalSpacer;
    QTabWidget *tw_miniCanvas;
    QWidget *tab_3;
    QVBoxLayout *verticalLayout_7;
    CanvasPreviewFrame *miniCanvasPreview;

    void setupUi(QMainWindow *CarlaHostW)
    {
        if (CarlaHostW->objectName().isEmpty())
            CarlaHostW->setObjectName(QString::fromUtf8("CarlaHostW"));
        CarlaHostW->resize(1045, 716);
        act_file_new = new QAction(CarlaHostW);
        act_file_new->setObjectName(QString::fromUtf8("act_file_new"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/16x16/document-new.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_file_new->setIcon(icon);
        act_file_open = new QAction(CarlaHostW);
        act_file_open->setObjectName(QString::fromUtf8("act_file_open"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/16x16/document-open.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_file_open->setIcon(icon1);
        act_file_save = new QAction(CarlaHostW);
        act_file_save->setObjectName(QString::fromUtf8("act_file_save"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/16x16/document-save.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_file_save->setIcon(icon2);
        act_file_save_as = new QAction(CarlaHostW);
        act_file_save_as->setObjectName(QString::fromUtf8("act_file_save_as"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/16x16/document-save-as.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_file_save_as->setIcon(icon3);
        act_file_quit = new QAction(CarlaHostW);
        act_file_quit->setObjectName(QString::fromUtf8("act_file_quit"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/16x16/application-exit.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_file_quit->setIcon(icon4);
        act_engine_start = new QAction(CarlaHostW);
        act_engine_start->setObjectName(QString::fromUtf8("act_engine_start"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/16x16/media-playback-start.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_engine_start->setIcon(icon5);
        act_engine_stop = new QAction(CarlaHostW);
        act_engine_stop->setObjectName(QString::fromUtf8("act_engine_stop"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/16x16/media-playback-stop.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_engine_stop->setIcon(icon6);
        act_plugin_add = new QAction(CarlaHostW);
        act_plugin_add->setObjectName(QString::fromUtf8("act_plugin_add"));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/16x16/list-add.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_plugin_add->setIcon(icon7);
        act_plugin_remove_all = new QAction(CarlaHostW);
        act_plugin_remove_all->setObjectName(QString::fromUtf8("act_plugin_remove_all"));
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/16x16/edit-delete.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_plugin_remove_all->setIcon(icon8);
        act_plugins_enable = new QAction(CarlaHostW);
        act_plugins_enable->setObjectName(QString::fromUtf8("act_plugins_enable"));
        act_plugins_disable = new QAction(CarlaHostW);
        act_plugins_disable->setObjectName(QString::fromUtf8("act_plugins_disable"));
        act_plugins_bypass = new QAction(CarlaHostW);
        act_plugins_bypass->setObjectName(QString::fromUtf8("act_plugins_bypass"));
        act_plugins_wet100 = new QAction(CarlaHostW);
        act_plugins_wet100->setObjectName(QString::fromUtf8("act_plugins_wet100"));
        act_plugins_mute = new QAction(CarlaHostW);
        act_plugins_mute->setObjectName(QString::fromUtf8("act_plugins_mute"));
        act_plugins_volume100 = new QAction(CarlaHostW);
        act_plugins_volume100->setObjectName(QString::fromUtf8("act_plugins_volume100"));
        act_plugins_center = new QAction(CarlaHostW);
        act_plugins_center->setObjectName(QString::fromUtf8("act_plugins_center"));
        act_transport_play = new QAction(CarlaHostW);
        act_transport_play->setObjectName(QString::fromUtf8("act_transport_play"));
        act_transport_play->setCheckable(true);
        act_transport_play->setIcon(icon5);
        act_transport_stop = new QAction(CarlaHostW);
        act_transport_stop->setObjectName(QString::fromUtf8("act_transport_stop"));
        act_transport_stop->setIcon(icon6);
        act_transport_backwards = new QAction(CarlaHostW);
        act_transport_backwards->setObjectName(QString::fromUtf8("act_transport_backwards"));
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/16x16/media-seek-backward.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_transport_backwards->setIcon(icon9);
        act_transport_forwards = new QAction(CarlaHostW);
        act_transport_forwards->setObjectName(QString::fromUtf8("act_transport_forwards"));
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/16x16/media-seek-forward.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_transport_forwards->setIcon(icon10);
        act_canvas_arrange = new QAction(CarlaHostW);
        act_canvas_arrange->setObjectName(QString::fromUtf8("act_canvas_arrange"));
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/16x16/view-sort-ascending.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_canvas_arrange->setIcon(icon11);
        act_canvas_refresh = new QAction(CarlaHostW);
        act_canvas_refresh->setObjectName(QString::fromUtf8("act_canvas_refresh"));
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/16x16/view-refresh.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_canvas_refresh->setIcon(icon12);
        act_canvas_save_image = new QAction(CarlaHostW);
        act_canvas_save_image->setObjectName(QString::fromUtf8("act_canvas_save_image"));
        act_canvas_zoom_fit = new QAction(CarlaHostW);
        act_canvas_zoom_fit->setObjectName(QString::fromUtf8("act_canvas_zoom_fit"));
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/16x16/zoom-fit-best.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_canvas_zoom_fit->setIcon(icon13);
        act_canvas_zoom_in = new QAction(CarlaHostW);
        act_canvas_zoom_in->setObjectName(QString::fromUtf8("act_canvas_zoom_in"));
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/16x16/zoom-in.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_canvas_zoom_in->setIcon(icon14);
        act_canvas_zoom_out = new QAction(CarlaHostW);
        act_canvas_zoom_out->setObjectName(QString::fromUtf8("act_canvas_zoom_out"));
        QIcon icon15;
        icon15.addFile(QString::fromUtf8(":/16x16/zoom-out.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_canvas_zoom_out->setIcon(icon15);
        act_canvas_zoom_100 = new QAction(CarlaHostW);
        act_canvas_zoom_100->setObjectName(QString::fromUtf8("act_canvas_zoom_100"));
        QIcon icon16;
        icon16.addFile(QString::fromUtf8(":/16x16/zoom-original.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_canvas_zoom_100->setIcon(icon16);
        act_settings_show_toolbar = new QAction(CarlaHostW);
        act_settings_show_toolbar->setObjectName(QString::fromUtf8("act_settings_show_toolbar"));
        act_settings_show_toolbar->setCheckable(true);
        act_settings_configure = new QAction(CarlaHostW);
        act_settings_configure->setObjectName(QString::fromUtf8("act_settings_configure"));
        QIcon icon17;
        icon17.addFile(QString::fromUtf8(":/16x16/configure.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_settings_configure->setIcon(icon17);
        act_help_about = new QAction(CarlaHostW);
        act_help_about->setObjectName(QString::fromUtf8("act_help_about"));
        act_help_about_juce = new QAction(CarlaHostW);
        act_help_about_juce->setObjectName(QString::fromUtf8("act_help_about_juce"));
        act_help_about_qt = new QAction(CarlaHostW);
        act_help_about_qt->setObjectName(QString::fromUtf8("act_help_about_qt"));
        act_settings_show_meters = new QAction(CarlaHostW);
        act_settings_show_meters->setObjectName(QString::fromUtf8("act_settings_show_meters"));
        act_settings_show_meters->setCheckable(true);
        act_settings_show_keyboard = new QAction(CarlaHostW);
        act_settings_show_keyboard->setObjectName(QString::fromUtf8("act_settings_show_keyboard"));
        act_settings_show_keyboard->setCheckable(true);
        act_canvas_show_internal = new QAction(CarlaHostW);
        act_canvas_show_internal->setObjectName(QString::fromUtf8("act_canvas_show_internal"));
        act_canvas_show_internal->setCheckable(true);
        act_canvas_show_external = new QAction(CarlaHostW);
        act_canvas_show_external->setObjectName(QString::fromUtf8("act_canvas_show_external"));
        act_canvas_show_external->setCheckable(true);
        act_settings_show_time_panel = new QAction(CarlaHostW);
        act_settings_show_time_panel->setObjectName(QString::fromUtf8("act_settings_show_time_panel"));
        act_settings_show_time_panel->setCheckable(true);
        act_settings_show_side_panel = new QAction(CarlaHostW);
        act_settings_show_side_panel->setObjectName(QString::fromUtf8("act_settings_show_side_panel"));
        act_settings_show_side_panel->setCheckable(true);
        act_file_connect = new QAction(CarlaHostW);
        act_file_connect->setObjectName(QString::fromUtf8("act_file_connect"));
        QIcon icon18;
        icon18.addFile(QString::fromUtf8(":/16x16/network-connect.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_file_connect->setIcon(icon18);
        act_file_refresh = new QAction(CarlaHostW);
        act_file_refresh->setObjectName(QString::fromUtf8("act_file_refresh"));
        act_file_refresh->setIcon(icon12);
        act_plugins_compact = new QAction(CarlaHostW);
        act_plugins_compact->setObjectName(QString::fromUtf8("act_plugins_compact"));
        act_plugins_expand = new QAction(CarlaHostW);
        act_plugins_expand->setObjectName(QString::fromUtf8("act_plugins_expand"));
        act_secret_1 = new QAction(CarlaHostW);
        act_secret_1->setObjectName(QString::fromUtf8("act_secret_1"));
        act_secret_2 = new QAction(CarlaHostW);
        act_secret_2->setObjectName(QString::fromUtf8("act_secret_2"));
        act_secret_3 = new QAction(CarlaHostW);
        act_secret_3->setObjectName(QString::fromUtf8("act_secret_3"));
        act_secret_4 = new QAction(CarlaHostW);
        act_secret_4->setObjectName(QString::fromUtf8("act_secret_4"));
        act_secret_5 = new QAction(CarlaHostW);
        act_secret_5->setObjectName(QString::fromUtf8("act_secret_5"));
        act_plugin_add_jack = new QAction(CarlaHostW);
        act_plugin_add_jack->setObjectName(QString::fromUtf8("act_plugin_add_jack"));
        act_plugin_add_jack->setIcon(icon7);
        act_engine_config = new QAction(CarlaHostW);
        act_engine_config->setObjectName(QString::fromUtf8("act_engine_config"));
        act_engine_config->setIcon(icon17);
        act_engine_panic = new QAction(CarlaHostW);
        act_engine_panic->setObjectName(QString::fromUtf8("act_engine_panic"));
        QIcon icon19;
        icon19.addFile(QString::fromUtf8(":/16x16/dialog-warning.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        act_engine_panic->setIcon(icon19);
        act_engine_panel = new QAction(CarlaHostW);
        act_engine_panel->setObjectName(QString::fromUtf8("act_engine_panel"));
        centralwidget = new QWidget(CarlaHostW);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout_4 = new QVBoxLayout(centralwidget);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(0, 4, 0, 1);
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setTabPosition(QTabWidget::North);
        rack = new QWidget();
        rack->setObjectName(QString::fromUtf8("rack"));
        horizontalLayout_2 = new QHBoxLayout(rack);
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 1);
        frame = new QFrame(rack);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Sunken);
        horizontalLayout_10 = new QHBoxLayout(frame);
        horizontalLayout_10->setSpacing(0);
        horizontalLayout_10->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        pad_left = new QLabel(frame);
        pad_left->setObjectName(QString::fromUtf8("pad_left"));
        pad_left->setMinimumSize(QSize(25, 0));
        pad_left->setMaximumSize(QSize(25, 16777215));

        horizontalLayout_10->addWidget(pad_left);

        listWidget = new RackListWidget(frame);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));
        listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        listWidget->setFrameShape(QFrame::NoFrame);
        listWidget->setFrameShadow(QFrame::Plain);
        listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        horizontalLayout_10->addWidget(listWidget);

        pad_right = new QLabel(frame);
        pad_right->setObjectName(QString::fromUtf8("pad_right"));
        pad_right->setMinimumSize(QSize(25, 0));
        pad_right->setMaximumSize(QSize(25, 16777215));

        horizontalLayout_10->addWidget(pad_right);

        rackScrollBar = new QScrollBar(frame);
        rackScrollBar->setObjectName(QString::fromUtf8("rackScrollBar"));
        rackScrollBar->setOrientation(Qt::Vertical);

        horizontalLayout_10->addWidget(rackScrollBar);


        horizontalLayout_2->addWidget(frame);

        tabWidget->addTab(rack, QString());
        patchbay = new QWidget();
        patchbay->setObjectName(QString::fromUtf8("patchbay"));
        gridLayout = new QGridLayout(patchbay);
        gridLayout->setSpacing(1);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 1);
        graphicsView = new DraggableGraphicsView(patchbay);
        graphicsView->setObjectName(QString::fromUtf8("graphicsView"));
        graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

        gridLayout->addWidget(graphicsView, 0, 1, 1, 1);

        peak_out = new DigitalPeakMeter(patchbay);
        peak_out->setObjectName(QString::fromUtf8("peak_out"));

        gridLayout->addWidget(peak_out, 0, 2, 1, 1);

        peak_in = new DigitalPeakMeter(patchbay);
        peak_in->setObjectName(QString::fromUtf8("peak_in"));

        gridLayout->addWidget(peak_in, 0, 0, 1, 1);

        tabWidget->addTab(patchbay, QString());
        logs = new QWidget();
        logs->setObjectName(QString::fromUtf8("logs"));
        horizontalLayout_4 = new QHBoxLayout(logs);
        horizontalLayout_4->setSpacing(0);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 0, 0, 1);
        text_logs = new QPlainTextEdit(logs);
        text_logs->setObjectName(QString::fromUtf8("text_logs"));
        QFont font;
        font.setFamily(QString::fromUtf8("DejaVu Sans Mono"));
        text_logs->setFont(font);
        text_logs->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        text_logs->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        text_logs->setLineWrapMode(QPlainTextEdit::NoWrap);
        text_logs->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

        horizontalLayout_4->addWidget(text_logs);

        tabWidget->addTab(logs, QString());

        verticalLayout_4->addWidget(tabWidget);

        tw_statusbar = new QFrame(centralwidget);
        tw_statusbar->setObjectName(QString::fromUtf8("tw_statusbar"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(tw_statusbar->sizePolicy().hasHeightForWidth());
        tw_statusbar->setSizePolicy(sizePolicy);
        horizontalLayout_3 = new QHBoxLayout(tw_statusbar);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(6, 0, 6, 4);
        label_buffer_size = new QLabel(tw_statusbar);
        label_buffer_size->setObjectName(QString::fromUtf8("label_buffer_size"));

        horizontalLayout_3->addWidget(label_buffer_size);

        cb_buffer_size = new QComboBox(tw_statusbar);
        cb_buffer_size->setObjectName(QString::fromUtf8("cb_buffer_size"));

        horizontalLayout_3->addWidget(cb_buffer_size);

        line_1 = new QFrame(tw_statusbar);
        line_1->setObjectName(QString::fromUtf8("line_1"));
        line_1->setLineWidth(0);
        line_1->setMidLineWidth(1);
        line_1->setFrameShape(QFrame::VLine);
        line_1->setFrameShadow(QFrame::Sunken);

        horizontalLayout_3->addWidget(line_1);

        label_sample_rate = new QLabel(tw_statusbar);
        label_sample_rate->setObjectName(QString::fromUtf8("label_sample_rate"));

        horizontalLayout_3->addWidget(label_sample_rate);

        cb_sample_rate = new QComboBox(tw_statusbar);
        cb_sample_rate->setObjectName(QString::fromUtf8("cb_sample_rate"));

        horizontalLayout_3->addWidget(cb_sample_rate);

        line_2 = new QFrame(tw_statusbar);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setLineWidth(0);
        line_2->setMidLineWidth(1);
        line_2->setFrameShape(QFrame::VLine);
        line_2->setFrameShadow(QFrame::Sunken);

        horizontalLayout_3->addWidget(line_2);

        b_xruns = new QPushButton(tw_statusbar);
        b_xruns->setObjectName(QString::fromUtf8("b_xruns"));
        b_xruns->setFlat(true);

        horizontalLayout_3->addWidget(b_xruns);

        line_4 = new QFrame(tw_statusbar);
        line_4->setObjectName(QString::fromUtf8("line_4"));
        line_4->setLineWidth(0);
        line_4->setMidLineWidth(1);
        line_4->setFrameShape(QFrame::VLine);
        line_4->setFrameShadow(QFrame::Sunken);

        horizontalLayout_3->addWidget(line_4);

        pb_dsp_load = new QProgressBar(tw_statusbar);
        pb_dsp_load->setObjectName(QString::fromUtf8("pb_dsp_load"));
        pb_dsp_load->setValue(0);
        pb_dsp_load->setAlignment(Qt::AlignCenter);

        horizontalLayout_3->addWidget(pb_dsp_load);


        verticalLayout_4->addWidget(tw_statusbar);

        CarlaHostW->setCentralWidget(centralwidget);
        menubar = new QMenuBar(CarlaHostW);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1045, 31));
        menu_File = new QMenu(menubar);
        menu_File->setObjectName(QString::fromUtf8("menu_File"));
        menu_Engine = new QMenu(menubar);
        menu_Engine->setObjectName(QString::fromUtf8("menu_Engine"));
        menu_Plugin = new QMenu(menubar);
        menu_Plugin->setObjectName(QString::fromUtf8("menu_Plugin"));
        menu_PluginMacros = new QMenu(menu_Plugin);
        menu_PluginMacros->setObjectName(QString::fromUtf8("menu_PluginMacros"));
        menu_Canvas = new QMenu(menubar);
        menu_Canvas->setObjectName(QString::fromUtf8("menu_Canvas"));
        menu_Canvas_Zoom = new QMenu(menu_Canvas);
        menu_Canvas_Zoom->setObjectName(QString::fromUtf8("menu_Canvas_Zoom"));
        menu_Settings = new QMenu(menubar);
        menu_Settings->setObjectName(QString::fromUtf8("menu_Settings"));
        menu_Help = new QMenu(menubar);
        menu_Help->setObjectName(QString::fromUtf8("menu_Help"));
        CarlaHostW->setMenuBar(menubar);
        toolBar = new QToolBar(CarlaHostW);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        toolBar->setMovable(false);
        toolBar->setAllowedAreas(Qt::NoToolBarArea);
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        CarlaHostW->addToolBar(Qt::TopToolBarArea, toolBar);
        dockWidget = new QDockWidget(CarlaHostW);
        dockWidget->setObjectName(QString::fromUtf8("dockWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(10);
        sizePolicy1.setHeightForWidth(dockWidget->sizePolicy().hasHeightForWidth());
        dockWidget->setSizePolicy(sizePolicy1);
        dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        dockWidget->setAllowedAreas(Qt::NoDockWidgetArea);
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
        verticalLayout = new QVBoxLayout(dockWidgetContents);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(1, 5, 0, 1);
        splitter = new QSplitter(dockWidgetContents);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(Qt::Vertical);
        tabUtils = new QTabWidget(splitter);
        tabUtils->setObjectName(QString::fromUtf8("tabUtils"));
        QSizePolicy sizePolicy2(QSizePolicy::Ignored, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(10);
        sizePolicy2.setHeightForWidth(tabUtils->sizePolicy().hasHeightForWidth());
        tabUtils->setSizePolicy(sizePolicy2);
        tabUtils->setMinimumSize(QSize(210, 0));
        tabUtils->setAcceptDrops(false);
        tabUtils->setTabPosition(QTabWidget::West);
        disk = new QWidget();
        disk->setObjectName(QString::fromUtf8("disk"));
        disk->setAcceptDrops(false);
        verticalLayout_3 = new QVBoxLayout(disk);
        verticalLayout_3->setSpacing(1);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 1);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        cb_disk = new QComboBox(disk);
        cb_disk->addItem(QString());
        cb_disk->setObjectName(QString::fromUtf8("cb_disk"));

        horizontalLayout->addWidget(cb_disk);

        b_disk_add = new QToolButton(disk);
        b_disk_add->setObjectName(QString::fromUtf8("b_disk_add"));
        b_disk_add->setIcon(icon7);

        horizontalLayout->addWidget(b_disk_add);

        b_disk_remove = new QToolButton(disk);
        b_disk_remove->setObjectName(QString::fromUtf8("b_disk_remove"));
        b_disk_remove->setEnabled(false);
        QIcon icon20;
        icon20.addFile(QString::fromUtf8(":/16x16/list-remove.svgz"), QSize(), QIcon::Normal, QIcon::Off);
        b_disk_remove->setIcon(icon20);

        horizontalLayout->addWidget(b_disk_remove);


        verticalLayout_3->addLayout(horizontalLayout);

        fileTreeView = new QTreeView(disk);
        fileTreeView->setObjectName(QString::fromUtf8("fileTreeView"));
        fileTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        fileTreeView->setDragEnabled(true);
        fileTreeView->setDragDropMode(QAbstractItemView::DragOnly);

        verticalLayout_3->addWidget(fileTreeView);

        tabUtils->addTab(disk, QString());
        w_transport = new QWidget();
        w_transport->setObjectName(QString::fromUtf8("w_transport"));
        verticalLayout_6 = new QVBoxLayout(w_transport);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        group_transport_controls = new QGroupBox(w_transport);
        group_transport_controls->setObjectName(QString::fromUtf8("group_transport_controls"));
        horizontalLayout_5 = new QHBoxLayout(group_transport_controls);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        b_transport_play = new QPushButton(group_transport_controls);
        b_transport_play->setObjectName(QString::fromUtf8("b_transport_play"));
        b_transport_play->setIcon(icon5);
        b_transport_play->setCheckable(true);

        horizontalLayout_5->addWidget(b_transport_play);

        b_transport_stop = new QPushButton(group_transport_controls);
        b_transport_stop->setObjectName(QString::fromUtf8("b_transport_stop"));
        b_transport_stop->setIcon(icon6);

        horizontalLayout_5->addWidget(b_transport_stop);

        b_transport_backwards = new QPushButton(group_transport_controls);
        b_transport_backwards->setObjectName(QString::fromUtf8("b_transport_backwards"));
        b_transport_backwards->setIcon(icon9);
        b_transport_backwards->setAutoRepeat(true);

        horizontalLayout_5->addWidget(b_transport_backwards);

        b_transport_forwards = new QPushButton(group_transport_controls);
        b_transport_forwards->setObjectName(QString::fromUtf8("b_transport_forwards"));
        b_transport_forwards->setIcon(icon10);
        b_transport_forwards->setAutoRepeat(true);

        horizontalLayout_5->addWidget(b_transport_forwards);


        verticalLayout_6->addWidget(group_transport_controls);

        group_transport_info = new QGroupBox(w_transport);
        group_transport_info->setObjectName(QString::fromUtf8("group_transport_info"));
        verticalLayout_2 = new QVBoxLayout(group_transport_info);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        label_transport_frame = new QLabel(group_transport_info);
        label_transport_frame->setObjectName(QString::fromUtf8("label_transport_frame"));
        label_transport_frame->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_6->addWidget(label_transport_frame);

        l_transport_frame = new QLabel(group_transport_info);
        l_transport_frame->setObjectName(QString::fromUtf8("l_transport_frame"));
        l_transport_frame->setFont(font);

        horizontalLayout_6->addWidget(l_transport_frame);

        horizontalSpacer = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer);


        verticalLayout_2->addLayout(horizontalLayout_6);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        label_transport_time = new QLabel(group_transport_info);
        label_transport_time->setObjectName(QString::fromUtf8("label_transport_time"));
        label_transport_time->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_7->addWidget(label_transport_time);

        l_transport_time = new QLabel(group_transport_info);
        l_transport_time->setObjectName(QString::fromUtf8("l_transport_time"));
        l_transport_time->setFont(font);

        horizontalLayout_7->addWidget(l_transport_time);

        horizontalSpacer_2 = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_2);


        verticalLayout_2->addLayout(horizontalLayout_7);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        label_transport_bbt = new QLabel(group_transport_info);
        label_transport_bbt->setObjectName(QString::fromUtf8("label_transport_bbt"));
        label_transport_bbt->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_8->addWidget(label_transport_bbt);

        l_transport_bbt = new QLabel(group_transport_info);
        l_transport_bbt->setObjectName(QString::fromUtf8("l_transport_bbt"));
        l_transport_bbt->setFont(font);

        horizontalLayout_8->addWidget(l_transport_bbt);

        horizontalSpacer_3 = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_3);


        verticalLayout_2->addLayout(horizontalLayout_8);


        verticalLayout_6->addWidget(group_transport_info);

        group_transport_settings = new QGroupBox(w_transport);
        group_transport_settings->setObjectName(QString::fromUtf8("group_transport_settings"));
        verticalLayout_5 = new QVBoxLayout(group_transport_settings);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        dsb_transport_bpm = new QDoubleSpinBox(group_transport_settings);
        dsb_transport_bpm->setObjectName(QString::fromUtf8("dsb_transport_bpm"));
        dsb_transport_bpm->setDecimals(1);
        dsb_transport_bpm->setMinimum(20.000000000000000);
        dsb_transport_bpm->setMaximum(999.000000000000000);

        verticalLayout_5->addWidget(dsb_transport_bpm);

        cb_transport_jack = new QCheckBox(group_transport_settings);
        cb_transport_jack->setObjectName(QString::fromUtf8("cb_transport_jack"));

        verticalLayout_5->addWidget(cb_transport_jack);

        cb_transport_link = new QCheckBox(group_transport_settings);
        cb_transport_link->setObjectName(QString::fromUtf8("cb_transport_link"));

        verticalLayout_5->addWidget(cb_transport_link);


        verticalLayout_6->addWidget(group_transport_settings);

        verticalSpacer = new QSpacerItem(20, 188, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer);

        tabUtils->addTab(w_transport, QString());
        splitter->addWidget(tabUtils);
        tw_miniCanvas = new QTabWidget(splitter);
        tw_miniCanvas->setObjectName(QString::fromUtf8("tw_miniCanvas"));
        QSizePolicy sizePolicy3(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(tw_miniCanvas->sizePolicy().hasHeightForWidth());
        tw_miniCanvas->setSizePolicy(sizePolicy3);
        tw_miniCanvas->setTabPosition(QTabWidget::West);
        tw_miniCanvas->setProperty("tabBarAutoHide", QVariant(false));
        tab_3 = new QWidget();
        tab_3->setObjectName(QString::fromUtf8("tab_3"));
        sizePolicy3.setHeightForWidth(tab_3->sizePolicy().hasHeightForWidth());
        tab_3->setSizePolicy(sizePolicy3);
        verticalLayout_7 = new QVBoxLayout(tab_3);
        verticalLayout_7->setSpacing(0);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        verticalLayout_7->setSizeConstraint(QLayout::SetDefaultConstraint);
        verticalLayout_7->setContentsMargins(0, 0, 0, 1);
        miniCanvasPreview = new CanvasPreviewFrame(tab_3);
        miniCanvasPreview->setObjectName(QString::fromUtf8("miniCanvasPreview"));
        sizePolicy3.setHeightForWidth(miniCanvasPreview->sizePolicy().hasHeightForWidth());
        miniCanvasPreview->setSizePolicy(sizePolicy3);
        miniCanvasPreview->setMinimumSize(QSize(210, 162));
        miniCanvasPreview->setAcceptDrops(false);
        miniCanvasPreview->setFrameShape(QFrame::StyledPanel);
        miniCanvasPreview->setFrameShadow(QFrame::Sunken);
        miniCanvasPreview->setLineWidth(1);

        verticalLayout_7->addWidget(miniCanvasPreview);

        tw_miniCanvas->addTab(tab_3, QString());
        splitter->addWidget(tw_miniCanvas);

        verticalLayout->addWidget(splitter);

        dockWidget->setWidget(dockWidgetContents);
        CarlaHostW->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

        menubar->addAction(menu_File->menuAction());
        menubar->addAction(menu_Engine->menuAction());
        menubar->addAction(menu_Plugin->menuAction());
        menubar->addAction(menu_Canvas->menuAction());
        menubar->addAction(menu_Settings->menuAction());
        menubar->addAction(menu_Help->menuAction());
        menu_File->addAction(act_file_connect);
        menu_File->addAction(act_file_refresh);
        menu_File->addAction(act_file_new);
        menu_File->addAction(act_file_open);
        menu_File->addAction(act_file_save);
        menu_File->addAction(act_file_save_as);
        menu_File->addSeparator();
        menu_File->addAction(act_file_quit);
        menu_Engine->addAction(act_engine_start);
        menu_Engine->addAction(act_engine_stop);
        menu_Engine->addAction(act_engine_panic);
        menu_Engine->addSeparator();
        menu_Engine->addAction(act_engine_config);
        menu_Plugin->addAction(act_plugin_add);
        menu_Plugin->addAction(act_plugin_add_jack);
        menu_Plugin->addAction(act_plugin_remove_all);
        menu_Plugin->addSeparator();
        menu_Plugin->addAction(menu_PluginMacros->menuAction());
        menu_PluginMacros->addAction(act_plugins_enable);
        menu_PluginMacros->addAction(act_plugins_disable);
        menu_PluginMacros->addSeparator();
        menu_PluginMacros->addAction(act_plugins_volume100);
        menu_PluginMacros->addAction(act_plugins_mute);
        menu_PluginMacros->addSeparator();
        menu_PluginMacros->addAction(act_plugins_wet100);
        menu_PluginMacros->addAction(act_plugins_bypass);
        menu_PluginMacros->addSeparator();
        menu_PluginMacros->addAction(act_plugins_center);
        menu_PluginMacros->addSeparator();
        menu_PluginMacros->addAction(act_plugins_compact);
        menu_PluginMacros->addAction(act_plugins_expand);
        menu_Canvas->addAction(act_canvas_show_internal);
        menu_Canvas->addAction(act_canvas_show_external);
        menu_Canvas->addSeparator();
        menu_Canvas->addAction(act_canvas_arrange);
        menu_Canvas->addAction(act_canvas_refresh);
        menu_Canvas->addAction(menu_Canvas_Zoom->menuAction());
        menu_Canvas->addSeparator();
        menu_Canvas->addAction(act_canvas_save_image);
        menu_Canvas_Zoom->addAction(act_canvas_zoom_fit);
        menu_Canvas_Zoom->addSeparator();
        menu_Canvas_Zoom->addAction(act_canvas_zoom_in);
        menu_Canvas_Zoom->addAction(act_canvas_zoom_out);
        menu_Canvas_Zoom->addAction(act_canvas_zoom_100);
        menu_Settings->addAction(act_settings_show_toolbar);
        menu_Settings->addAction(act_settings_show_meters);
        menu_Settings->addAction(act_settings_show_keyboard);
        menu_Settings->addAction(act_settings_show_side_panel);
        menu_Settings->addSeparator();
        menu_Settings->addAction(act_settings_configure);
        menu_Help->addAction(act_help_about);
        menu_Help->addAction(act_help_about_juce);
        menu_Help->addAction(act_help_about_qt);
        toolBar->addAction(act_file_new);
        toolBar->addAction(act_file_open);
        toolBar->addAction(act_file_save);
        toolBar->addAction(act_file_save_as);
        toolBar->addAction(act_file_connect);
        toolBar->addAction(act_file_refresh);
        toolBar->addSeparator();
        toolBar->addAction(act_plugin_add);
        toolBar->addAction(act_plugin_add_jack);
        toolBar->addAction(act_plugin_remove_all);
        toolBar->addAction(act_engine_panic);
        toolBar->addSeparator();
        toolBar->addAction(act_settings_configure);

        retranslateUi(CarlaHostW);
        QObject::connect(act_file_quit, SIGNAL(triggered()), CarlaHostW, SLOT(close()));

        tabWidget->setCurrentIndex(0);
        tabUtils->setCurrentIndex(0);
        tw_miniCanvas->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(CarlaHostW);
    } // setupUi

    void retranslateUi(QMainWindow *CarlaHostW)
    {
        CarlaHostW->setWindowTitle(QCoreApplication::translate("CarlaHostW", "MainWindow", nullptr));
        act_file_new->setText(QCoreApplication::translate("CarlaHostW", "&New", nullptr));
#if QT_CONFIG(shortcut)
        act_file_new->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+N", nullptr));
#endif // QT_CONFIG(shortcut)
        act_file_open->setText(QCoreApplication::translate("CarlaHostW", "&Open...", nullptr));
        act_file_open->setIconText(QCoreApplication::translate("CarlaHostW", "Open...", nullptr));
#if QT_CONFIG(tooltip)
        act_file_open->setToolTip(QCoreApplication::translate("CarlaHostW", "Open...", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        act_file_open->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+O", nullptr));
#endif // QT_CONFIG(shortcut)
        act_file_save->setText(QCoreApplication::translate("CarlaHostW", "&Save", nullptr));
#if QT_CONFIG(shortcut)
        act_file_save->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        act_file_save_as->setText(QCoreApplication::translate("CarlaHostW", "Save &As...", nullptr));
        act_file_save_as->setIconText(QCoreApplication::translate("CarlaHostW", "Save As...", nullptr));
#if QT_CONFIG(tooltip)
        act_file_save_as->setToolTip(QCoreApplication::translate("CarlaHostW", "Save As...", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        act_file_save_as->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+Shift+S", nullptr));
#endif // QT_CONFIG(shortcut)
        act_file_quit->setText(QCoreApplication::translate("CarlaHostW", "&Quit", nullptr));
#if QT_CONFIG(shortcut)
        act_file_quit->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+Q", nullptr));
#endif // QT_CONFIG(shortcut)
        act_engine_start->setText(QCoreApplication::translate("CarlaHostW", "&Start", nullptr));
#if QT_CONFIG(shortcut)
        act_engine_start->setShortcut(QCoreApplication::translate("CarlaHostW", "F5", nullptr));
#endif // QT_CONFIG(shortcut)
        act_engine_stop->setText(QCoreApplication::translate("CarlaHostW", "St&op", nullptr));
#if QT_CONFIG(shortcut)
        act_engine_stop->setShortcut(QCoreApplication::translate("CarlaHostW", "F6", nullptr));
#endif // QT_CONFIG(shortcut)
        act_plugin_add->setText(QCoreApplication::translate("CarlaHostW", "&Add Plugin...", nullptr));
#if QT_CONFIG(shortcut)
        act_plugin_add->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+A", nullptr));
#endif // QT_CONFIG(shortcut)
        act_plugin_remove_all->setText(QCoreApplication::translate("CarlaHostW", "&Remove All", nullptr));
        act_plugins_enable->setText(QCoreApplication::translate("CarlaHostW", "Enable", nullptr));
        act_plugins_disable->setText(QCoreApplication::translate("CarlaHostW", "Disable", nullptr));
        act_plugins_bypass->setText(QCoreApplication::translate("CarlaHostW", "0% Wet (Bypass)", nullptr));
        act_plugins_wet100->setText(QCoreApplication::translate("CarlaHostW", "100% Wet", nullptr));
        act_plugins_mute->setText(QCoreApplication::translate("CarlaHostW", "0% Volume (Mute)", nullptr));
        act_plugins_volume100->setText(QCoreApplication::translate("CarlaHostW", "100% Volume", nullptr));
        act_plugins_center->setText(QCoreApplication::translate("CarlaHostW", "Center Balance", nullptr));
        act_transport_play->setText(QCoreApplication::translate("CarlaHostW", "&Play", nullptr));
#if QT_CONFIG(shortcut)
        act_transport_play->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+Shift+P", nullptr));
#endif // QT_CONFIG(shortcut)
        act_transport_stop->setText(QCoreApplication::translate("CarlaHostW", "&Stop", nullptr));
#if QT_CONFIG(shortcut)
        act_transport_stop->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+Shift+X", nullptr));
#endif // QT_CONFIG(shortcut)
        act_transport_backwards->setText(QCoreApplication::translate("CarlaHostW", "&Backwards", nullptr));
#if QT_CONFIG(shortcut)
        act_transport_backwards->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+Shift+B", nullptr));
#endif // QT_CONFIG(shortcut)
        act_transport_forwards->setText(QCoreApplication::translate("CarlaHostW", "&Forwards", nullptr));
#if QT_CONFIG(shortcut)
        act_transport_forwards->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+Shift+F", nullptr));
#endif // QT_CONFIG(shortcut)
        act_canvas_arrange->setText(QCoreApplication::translate("CarlaHostW", "&Arrange", nullptr));
#if QT_CONFIG(shortcut)
        act_canvas_arrange->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+G", nullptr));
#endif // QT_CONFIG(shortcut)
        act_canvas_refresh->setText(QCoreApplication::translate("CarlaHostW", "&Refresh", nullptr));
#if QT_CONFIG(shortcut)
        act_canvas_refresh->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+R", nullptr));
#endif // QT_CONFIG(shortcut)
        act_canvas_save_image->setText(QCoreApplication::translate("CarlaHostW", "Save &Image...", nullptr));
        act_canvas_zoom_fit->setText(QCoreApplication::translate("CarlaHostW", "Auto-Fit", nullptr));
#if QT_CONFIG(shortcut)
        act_canvas_zoom_fit->setShortcut(QCoreApplication::translate("CarlaHostW", "Home", nullptr));
#endif // QT_CONFIG(shortcut)
        act_canvas_zoom_in->setText(QCoreApplication::translate("CarlaHostW", "Zoom In", nullptr));
#if QT_CONFIG(shortcut)
        act_canvas_zoom_in->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl++", nullptr));
#endif // QT_CONFIG(shortcut)
        act_canvas_zoom_out->setText(QCoreApplication::translate("CarlaHostW", "Zoom Out", nullptr));
#if QT_CONFIG(shortcut)
        act_canvas_zoom_out->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+-", nullptr));
#endif // QT_CONFIG(shortcut)
        act_canvas_zoom_100->setText(QCoreApplication::translate("CarlaHostW", "Zoom 100%", nullptr));
#if QT_CONFIG(shortcut)
        act_canvas_zoom_100->setShortcut(QCoreApplication::translate("CarlaHostW", "Ctrl+1", nullptr));
#endif // QT_CONFIG(shortcut)
        act_settings_show_toolbar->setText(QCoreApplication::translate("CarlaHostW", "Show &Toolbar", nullptr));
        act_settings_configure->setText(QCoreApplication::translate("CarlaHostW", "&Configure Carla", nullptr));
        act_help_about->setText(QCoreApplication::translate("CarlaHostW", "&About", nullptr));
        act_help_about_juce->setText(QCoreApplication::translate("CarlaHostW", "About &JUCE", nullptr));
        act_help_about_qt->setText(QCoreApplication::translate("CarlaHostW", "About &Qt", nullptr));
        act_settings_show_meters->setText(QCoreApplication::translate("CarlaHostW", "Show Canvas &Meters", nullptr));
        act_settings_show_keyboard->setText(QCoreApplication::translate("CarlaHostW", "Show Canvas &Keyboard", nullptr));
        act_canvas_show_internal->setText(QCoreApplication::translate("CarlaHostW", "Show Internal", nullptr));
        act_canvas_show_external->setText(QCoreApplication::translate("CarlaHostW", "Show External", nullptr));
        act_settings_show_time_panel->setText(QCoreApplication::translate("CarlaHostW", "Show Time Panel", nullptr));
        act_settings_show_side_panel->setText(QCoreApplication::translate("CarlaHostW", "Show &Side Panel", nullptr));
        act_file_connect->setText(QCoreApplication::translate("CarlaHostW", "&Connect...", nullptr));
        act_file_refresh->setText(QCoreApplication::translate("CarlaHostW", "&Refresh", nullptr));
        act_plugins_compact->setText(QCoreApplication::translate("CarlaHostW", "Compact Slots", nullptr));
        act_plugins_expand->setText(QCoreApplication::translate("CarlaHostW", "Expand Slots", nullptr));
        act_secret_1->setText(QCoreApplication::translate("CarlaHostW", "Perform secret 1", nullptr));
        act_secret_2->setText(QCoreApplication::translate("CarlaHostW", "Perform secret 2", nullptr));
        act_secret_3->setText(QCoreApplication::translate("CarlaHostW", "Perform secret 3", nullptr));
        act_secret_4->setText(QCoreApplication::translate("CarlaHostW", "Perform secret 4", nullptr));
        act_secret_5->setText(QCoreApplication::translate("CarlaHostW", "Perform secret 5", nullptr));
        act_plugin_add_jack->setText(QCoreApplication::translate("CarlaHostW", "Add &JACK Application...", nullptr));
        act_engine_config->setText(QCoreApplication::translate("CarlaHostW", "&Configure driver...", nullptr));
        act_engine_panic->setText(QCoreApplication::translate("CarlaHostW", "Panic", nullptr));
        act_engine_panel->setText(QCoreApplication::translate("CarlaHostW", "Open custom driver panel...", nullptr));
        pad_left->setText(QString());
        pad_right->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(rack), QCoreApplication::translate("CarlaHostW", "Rack", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(patchbay), QCoreApplication::translate("CarlaHostW", "Patchbay", nullptr));
        text_logs->setPlainText(QCoreApplication::translate("CarlaHostW", "Loading...", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(logs), QCoreApplication::translate("CarlaHostW", "Logs", nullptr));
        label_buffer_size->setText(QCoreApplication::translate("CarlaHostW", "Buffer Size:", nullptr));
        label_sample_rate->setText(QCoreApplication::translate("CarlaHostW", "Sample Rate:", nullptr));
        b_xruns->setText(QCoreApplication::translate("CarlaHostW", "? Xruns", nullptr));
        pb_dsp_load->setFormat(QCoreApplication::translate("CarlaHostW", "DSP Load: %p%", nullptr));
        menu_File->setTitle(QCoreApplication::translate("CarlaHostW", "&File", nullptr));
        menu_Engine->setTitle(QCoreApplication::translate("CarlaHostW", "&Engine", nullptr));
        menu_Plugin->setTitle(QCoreApplication::translate("CarlaHostW", "&Plugin", nullptr));
        menu_PluginMacros->setTitle(QCoreApplication::translate("CarlaHostW", "Macros (all plugins)", nullptr));
        menu_Canvas->setTitle(QCoreApplication::translate("CarlaHostW", "&Canvas", nullptr));
        menu_Canvas_Zoom->setTitle(QCoreApplication::translate("CarlaHostW", "Zoom", nullptr));
        menu_Settings->setTitle(QCoreApplication::translate("CarlaHostW", "&Settings", nullptr));
        menu_Help->setTitle(QCoreApplication::translate("CarlaHostW", "&Help", nullptr));
        toolBar->setWindowTitle(QCoreApplication::translate("CarlaHostW", "toolBar", nullptr));
        cb_disk->setItemText(0, QCoreApplication::translate("CarlaHostW", "Home", nullptr));

        b_disk_add->setText(QString());
        b_disk_remove->setText(QString());
        tabUtils->setTabText(tabUtils->indexOf(disk), QCoreApplication::translate("CarlaHostW", "Disk", nullptr));
        group_transport_controls->setTitle(QCoreApplication::translate("CarlaHostW", "Playback Controls", nullptr));
        b_transport_play->setText(QString());
        b_transport_stop->setText(QString());
        b_transport_backwards->setText(QString());
        b_transport_forwards->setText(QString());
        group_transport_info->setTitle(QCoreApplication::translate("CarlaHostW", "Time Information", nullptr));
        label_transport_frame->setText(QCoreApplication::translate("CarlaHostW", "Frame:", nullptr));
        l_transport_frame->setText(QCoreApplication::translate("CarlaHostW", "000'000'000", nullptr));
        label_transport_time->setText(QCoreApplication::translate("CarlaHostW", "Time:", nullptr));
        l_transport_time->setText(QCoreApplication::translate("CarlaHostW", "00:00:00", nullptr));
        label_transport_bbt->setText(QCoreApplication::translate("CarlaHostW", "BBT:", nullptr));
        l_transport_bbt->setText(QCoreApplication::translate("CarlaHostW", "000|00|0000", nullptr));
        group_transport_settings->setTitle(QCoreApplication::translate("CarlaHostW", "Settings", nullptr));
        dsb_transport_bpm->setSuffix(QCoreApplication::translate("CarlaHostW", " BPM", nullptr));
        cb_transport_jack->setText(QCoreApplication::translate("CarlaHostW", "Use JACK Transport", nullptr));
        cb_transport_link->setText(QCoreApplication::translate("CarlaHostW", "Use Ableton Link", nullptr));
        tabUtils->setTabText(tabUtils->indexOf(w_transport), QCoreApplication::translate("CarlaHostW", "Transport", nullptr));
        tw_miniCanvas->setTabText(tw_miniCanvas->indexOf(tab_3), QString());
    } // retranslateUi

};

namespace Ui {
    class CarlaHostW: public Ui_CarlaHostW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CARLA_HOST_H
