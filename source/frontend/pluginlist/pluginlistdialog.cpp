/*
 * Carla plugin host
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "pluginlistdialog.hpp"

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
# pragma clang diagnostic ignored "-Wdeprecated-register"
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wclass-memaccess"
# pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include "ui_pluginlistdialog.h"
#include <QtCore/QList>

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "qcarlastring.hpp"
#include "qsafesettings.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaFrontend.h"
#include "CarlaUtils.h"

#include "CarlaString.hpp"

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
// Backwards-compatible horizontalAdvance/width call, depending on Qt version

static inline
int fontMetricsHorizontalAdvance(const QFontMetrics& fontMetrics, const QString& string)
{
#if QT_VERSION >= 0x50b00
    return fontMetrics.horizontalAdvance(string);
#else
    return fontMetrics.width(string);
#endif
}

// --------------------------------------------------------------------------------------------------------------------

typedef QList<PluginInfo> QPluginInfoList;

class QSafePluginListSettings : public QSafeSettings
{
public:
    inline QSafePluginListSettings()
        : QSafeSettings() {}

    inline QSafePluginListSettings(const QString& organization, const QString& application)
        : QSafeSettings(organization, application) {}

    QPluginInfoList valuePluginInfoList(const QString& key, QPluginInfoList defaultValue = {}) const
    {
        /*
        QVariant var(value(key, defaultValue));

        if (!var.isNull() && var.convert(QVariant::List) && var.isValid())
        {
            QList<QVariant> varVariant(var.toList());
            QPluginInfoList varReal;

            varReal.reserve(varVariant.size());

            for (QVariant v : varVariant)
            {
                CARLA_SAFE_ASSERT_BREAK(!v.isNull());
                CARLA_SAFE_ASSERT_BREAK(v.convert(QVariant::UserType));
                CARLA_SAFE_ASSERT_BREAK(v.isValid());
                // varReal.append(v.toU);
            }
        }
        */

        return defaultValue;
    }

    void setValue(const QString& key, const uint value)
    {
        QSafeSettings::setValue(key, value);
    }

    void setValue(const QString& key, const QPluginInfoList& value)
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

struct PluginListDialog::Self {
    enum TableIndex {
        TABLEWIDGET_ITEM_FAVORITE,
        TABLEWIDGET_ITEM_NAME,
        TABLEWIDGET_ITEM_LABEL,
        TABLEWIDGET_ITEM_MAKER,
        TABLEWIDGET_ITEM_BINARY,
    };

    // To be changed by parent
    bool hasLoadedLv2Plugins = false;

    Ui_PluginListDialog ui;
    HostSettings hostSettings;

    int fLastTableIndex;
    PluginListDialogResults fRetPlugin;
    QWidget* fRealParent;
    QStringList fFavoritePlugins;
    bool fFavoritePluginsChanged;

    QString fTrYes;
    QString fTrNo;
    QString fTrNative;

    Self() {}

    static Self& create()
    {
        Self* const self = new Self();
        return *self;
    }

    void createFavoritePluginDict()
    {
//         return {
//             'name'    : plugin['name'],
//             'build'   : plugin['build'],
//             'type'    : plugin['type'],
//             'filename': plugin['filename'],
//             'label'   : plugin['label'],
//             'uniqueId': plugin['uniqueId'],
//         }
    }

    void checkFilters()
    {
    }

    void addPluginToTable(uint plugin, const PluginType ptype)
    {
    }

    PluginInfo checkPluginCached(const CarlaCachedPluginInfo* const desc, const PluginType ptype)
    {
        PluginInfo pinfo = {};
        pinfo.build = BINARY_NATIVE;
        pinfo.type  = ptype;
        pinfo.hints = desc->hints;
        pinfo.name  = desc->name;
        pinfo.label = desc->label;
        pinfo.maker = desc->maker;
        pinfo.category = getPluginCategoryAsString(desc->category);

        pinfo.audioIns  = desc->audioIns;
        pinfo.audioOuts = desc->audioOuts;

        pinfo.cvIns  = desc->cvIns;
        pinfo.cvOuts = desc->cvOuts;

        pinfo.midiIns  = desc->midiIns;
        pinfo.midiOuts = desc->midiOuts;

        pinfo.parametersIns  = desc->parameterIns;
        pinfo.parametersOuts = desc->parameterOuts;

        switch (ptype)
        {
        case PLUGIN_LV2:
            {
                QString label(desc->label);
                pinfo.filename = label.split(CARLA_OS_SEP).first();
                pinfo.label = label.section(CARLA_OS_SEP, 1);
            }
            break;
        case PLUGIN_SFZ:
            pinfo.filename = pinfo.label;
            pinfo.label    = pinfo.name;
            break;
        default:
            break;
        }

        return pinfo;
    }

    uint reAddInternalHelper(QSafePluginListSettings& settingsDB, const PluginType ptype, const char* const path)
    {
        QString ptypeStr, ptypeStrTr;

        switch (ptype)
        {
        case PLUGIN_INTERNAL:
            ptypeStr   = "Internal";
            ptypeStrTr = "Internal"; // self.tr("Internal")
            break;
        case PLUGIN_LV2:
            ptypeStr   = "LV2";
            ptypeStrTr = ptypeStr;
            break;
        case PLUGIN_AU:
            ptypeStr   = "AU";
            ptypeStrTr = ptypeStr;
            break;
        case PLUGIN_SFZ:
            ptypeStr   = "SFZ";
            ptypeStrTr = ptypeStr;
            break;
        // TODO(jsfx) what to do here?
        default:
            return 0;
        }

        QPluginInfoList plugins = settingsDB.valuePluginInfoList("Plugins/" + ptypeStr);
        uint pluginCount = settingsDB.valueUInt("PluginCount/" + ptypeStr, 0);

        if (ptype == PLUGIN_AU)
            carla_juce_init();

        const uint pluginCountNew = carla_get_cached_plugin_count(ptype, path);

        if (pluginCountNew != pluginCount || plugins.size() != pluginCount ||
            (plugins.size() > 0 && plugins[0].API != PLUGIN_QUERY_API_VERSION))
        {
            plugins.clear();
            pluginCount = pluginCountNew;

            QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents, 50);

            if (ptype == PLUGIN_AU)
                carla_juce_idle();

            for (uint i=0; i<pluginCountNew; ++i)
            {
                const CarlaCachedPluginInfo* const descInfo = carla_get_cached_plugin_info(ptype, i);

                if (descInfo == nullptr)
                    continue;

                const PluginInfo info = checkPluginCached(descInfo, ptype);
                plugins.append(info);

                if ((i % 50) == 0)
                {
                    QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents, 50);

                    if (ptype == PLUGIN_AU)
                        carla_juce_idle();
                }
            }

            settingsDB.setValue("Plugins/" + ptypeStr, plugins);
            settingsDB.setValue("PluginCount/" + ptypeStr, pluginCount);
        }

        if (ptype == PLUGIN_AU)
            carla_juce_cleanup();

        // prepare rows in advance
        ui.tableWidget->setRowCount(fLastTableIndex + plugins.size());

//         for plugin in plugins:
//             self._addPluginToTable(plugin, ptypeStrTr)

        return pluginCount;
    }

    void reAddPlugins()
    {
        QSafePluginListSettings settingsDB("falkTX", "CarlaPlugins5");

        fLastTableIndex = 0;
        ui.tableWidget->setSortingEnabled(false);
        ui.tableWidget->clearContents();

        QString LV2_PATH;
        {
            const QSafeSettings settings("falkTX", "Carla2");
            // LV2_PATH = splitter.join()
            // settings.valueStringList(CARLA_KEY_PATHS_LV2, CARLA_DEFAULT_LV2_PATH);
        }

        // ------------------------------------------------------------------------------------------------------------
        // plugins handled through backend

        const uint internalCount = reAddInternalHelper(settingsDB, PLUGIN_INTERNAL, "");
        const uint lv2Count = reAddInternalHelper(settingsDB, PLUGIN_LV2, LV2_PATH.toUtf8());
       #ifdef CARLA_OS_MAC
        const uint auCount = reAddInternalHelper(settingsDB, PLUGIN_AU, "");
       #else
        const uint auCount = 0;
       #endif

        // ------------------------------------------------------------------------------------------------------------
        // LADSPA

        QStringList ladspaPlugins;
        ladspaPlugins << settingsDB.valueStringList("Plugins/LADSPA_native");
        ladspaPlugins << settingsDB.valueStringList("Plugins/LADSPA_posix32");
        ladspaPlugins << settingsDB.valueStringList("Plugins/LADSPA_posix64");
        ladspaPlugins << settingsDB.valueStringList("Plugins/LADSPA_win32");
        ladspaPlugins << settingsDB.valueStringList("Plugins/LADSPA_win64");

    }
};

PluginListDialog::PluginListDialog(QWidget* const parent, const HostSettings& hostSettings)
    : QDialog(parent),
      self(Self::create())
{
    self.ui.setupUi(this);
    self.hostSettings = hostSettings;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal stuff

    self.fLastTableIndex = 0;
    self.fRetPlugin  = {};
    self.fRealParent = parent;
    self.fFavoritePlugins.clear();
    self.fFavoritePluginsChanged = false;

    self.fTrYes    = tr("Yes");
    self.fTrNo     = tr("No");
    self.fTrNative = tr("Native");

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up GUI

    self.ui.b_add->setEnabled(false);
    addAction(self.ui.act_focus_search);
    // TODO
    // self.ui.act_focus_search.triggered.connect(self.slot_focusSearchFieldAndSelectAll);

   #if BINARY_NATIVE == BINARY_POSIX32 || BINARY_NATIVE == BINARY_WIN32
    self.ui.ch_bridged->setText(tr("Bridged (64bit)"));
   #else
    self.ui.ch_bridged->setText(tr("Bridged (32bit)"));
   #endif

   #if !(defined(CARLA_OS_LINUX) || defined(CARLA_OS_MAC))
    self.ui.ch_bridged_wine->setChecked(false);
    self.ui.ch_bridged_wine->setEnabled(false);
   #endif

   #ifdef CARLA_OS_MAC
    setWindowModality(Qt::WindowModal);
   #else
    self.ui.ch_au->setChecked(false);
    self.ui.ch_au->setEnabled(false);
    self.ui.ch_au->setVisible(false);
   #endif

    self.ui.tab_info->tabBar()->hide();
    self.ui.tab_reqs->tabBar()->hide();
    // FIXME, why /2 needed?
    self.ui.tab_info->setMinimumWidth(self.ui.la_id->width()/2 +
                                      fontMetricsHorizontalAdvance(self.ui.l_id->fontMetrics(), "9999999999") + 6*3);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ----------------------------------------------------------------------------------------------------------------
    // Load settings

    loadSettings();

    // ----------------------------------------------------------------------------------------------------------------
    // Disable bridges if not enabled in settings

    // NOTE: We Assume win32 carla build will not run win64 plugins
//     if (WINDOWS and not kIs64bit) or not host.showPluginBridges:
//         self.ui.ch_native.setChecked(True)
//         self.ui.ch_native.setEnabled(False)
//         self.ui.ch_native.setVisible(True)
//         self.ui.ch_bridged.setChecked(False)
//         self.ui.ch_bridged.setEnabled(False)
//         self.ui.ch_bridged.setVisible(False)
//         self.ui.ch_bridged_wine.setChecked(False)
//         self.ui.ch_bridged_wine.setEnabled(False)
//         self.ui.ch_bridged_wine.setVisible(False)
//
//     elif not host.showWineBridges:
//         self.ui.ch_bridged_wine.setChecked(False)
//         self.ui.ch_bridged_wine.setEnabled(False)
//         self.ui.ch_bridged_wine.setVisible(False)

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up Icons

    if (hostSettings.useSystemIcons)
    {
//         self.ui.b_add.setIcon(getIcon('list-add', 16, 'svgz'))
//         self.ui.b_cancel.setIcon(getIcon('dialog-cancel', 16, 'svgz'))
//         self.ui.b_clear_filters.setIcon(getIcon('edit-clear', 16, 'svgz'))
//         self.ui.b_refresh.setIcon(getIcon('view-refresh', 16, 'svgz'))
        QTableWidgetItem* const hhi = self.ui.tableWidget->horizontalHeaderItem(self.TABLEWIDGET_ITEM_FAVORITE);
//         hhi.setIcon(getIcon('bookmarks', 16, 'svgz'))
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

//     self.finished.connect(self.slot_saveSettings)
//     self.ui.b_add.clicked.connect(self.slot_addPlugin)
//     self.ui.b_cancel.clicked.connect(self.reject)
//     self.ui.b_refresh.clicked.connect(self.slot_refreshPlugins)
//     self.ui.b_clear_filters.clicked.connect(self.slot_clearFilters)
//     self.ui.lineEdit.textChanged.connect(self.slot_checkFilters)
//     self.ui.tableWidget.currentCellChanged.connect(self.slot_checkPlugin)
//     self.ui.tableWidget.cellClicked.connect(self.slot_cellClicked)
//     self.ui.tableWidget.cellDoubleClicked.connect(self.slot_cellDoubleClicked)
//
//     self.ui.ch_internal.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_ladspa.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_dssi.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_lv2.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_vst.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_vst3.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_clap.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_au.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_jsfx.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_kits.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_effects.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_instruments.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_midi.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_other.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_native.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_bridged.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_bridged_wine.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_favorites.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_rtsafe.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_cv.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_gui.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_inline_display.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_stereo.clicked.connect(self.slot_checkFilters)
//     self.ui.ch_cat_all.clicked.connect(self.slot_checkFiltersCategoryAll)
//     self.ui.ch_cat_delay.clicked.connect(self.slot_checkFiltersCategorySpecific)
//     self.ui.ch_cat_distortion.clicked.connect(self.slot_checkFiltersCategorySpecific)
//     self.ui.ch_cat_dynamics.clicked.connect(self.slot_checkFiltersCategorySpecific)
//     self.ui.ch_cat_eq.clicked.connect(self.slot_checkFiltersCategorySpecific)
//     self.ui.ch_cat_filter.clicked.connect(self.slot_checkFiltersCategorySpecific)
//     self.ui.ch_cat_modulator.clicked.connect(self.slot_checkFiltersCategorySpecific)
//     self.ui.ch_cat_synth.clicked.connect(self.slot_checkFiltersCategorySpecific)
//     self.ui.ch_cat_utility.clicked.connect(self.slot_checkFiltersCategorySpecific)
//     self.ui.ch_cat_other.clicked.connect(self.slot_checkFiltersCategorySpecific)

    // ----------------------------------------------------------------------------------------------------------------
    // Post-connect setup

    self.reAddPlugins();
    slot_focusSearchFieldAndSelectAll();
}

PluginListDialog::~PluginListDialog()
{
    delete &self;
}

// --------------------------------------------------------------------------------------------------------------------
// public methods

// --------------------------------------------------------------------------------------------------------------------
// protected methods

void PluginListDialog::showEvent(QShowEvent* const event)
{
    slot_focusSearchFieldAndSelectAll();
    QDialog::showEvent(event);
}

// --------------------------------------------------------------------------------------------------------------------
// private methods

void PluginListDialog::loadSettings()
{
    const QSafeSettings settings("falkTX", "CarlaDatabase2");
    self.fFavoritePlugins = settings.valueStringList("PluginDatabase/Favorites");
    self.fFavoritePluginsChanged = false;

    restoreGeometry(settings.valueByteArray("PluginDatabase/Geometry"));
    self.ui.ch_effects->setChecked(settings.valueBool("PluginDatabase/ShowEffects", true));
    self.ui.ch_instruments->setChecked(settings.valueBool("PluginDatabase/ShowInstruments", true));
    self.ui.ch_midi->setChecked(settings.valueBool("PluginDatabase/ShowMIDI", true));
    self.ui.ch_other->setChecked(settings.valueBool("PluginDatabase/ShowOther", true));
    self.ui.ch_internal->setChecked(settings.valueBool("PluginDatabase/ShowInternal", true));
    self.ui.ch_ladspa->setChecked(settings.valueBool("PluginDatabase/ShowLADSPA", true));
    self.ui.ch_dssi->setChecked(settings.valueBool("PluginDatabase/ShowDSSI", true));
    self.ui.ch_lv2->setChecked(settings.valueBool("PluginDatabase/ShowLV2", true));
    self.ui.ch_vst->setChecked(settings.valueBool("PluginDatabase/ShowVST2", true));
    self.ui.ch_vst3->setChecked(settings.valueBool("PluginDatabase/ShowVST3", true));
    self.ui.ch_clap->setChecked(settings.valueBool("PluginDatabase/ShowCLAP", true));
   #ifdef CARLA_OS_MAC
    self.ui.ch_au->setChecked(settings.valueBool("PluginDatabase/ShowAU", true));
   #endif
    self.ui.ch_jsfx->setChecked(settings.valueBool("PluginDatabase/ShowJSFX", true));
    self.ui.ch_kits->setChecked(settings.valueBool("PluginDatabase/ShowKits", true));
    self.ui.ch_native->setChecked(settings.valueBool("PluginDatabase/ShowNative", true));
    self.ui.ch_bridged->setChecked(settings.valueBool("PluginDatabase/ShowBridged", true));
    self.ui.ch_bridged_wine->setChecked(settings.valueBool("PluginDatabase/ShowBridgedWine", true));
    self.ui.ch_favorites->setChecked(settings.valueBool("PluginDatabase/ShowFavorites", false));
    self.ui.ch_rtsafe->setChecked(settings.valueBool("PluginDatabase/ShowRtSafe", false));
    self.ui.ch_cv->setChecked(settings.valueBool("PluginDatabase/ShowHasCV", false));
    self.ui.ch_gui->setChecked(settings.valueBool("PluginDatabase/ShowHasGUI", false));
    self.ui.ch_inline_display->setChecked(settings.valueBool("PluginDatabase/ShowHasInlineDisplay", false));
    self.ui.ch_stereo->setChecked(settings.valueBool("PluginDatabase/ShowStereoOnly", false));
    self.ui.lineEdit->setText(settings.valueString("PluginDatabase/SearchText", ""));

    QString categoryhash = settings.valueString("PluginDatabase/ShowCategory", "all");
    if (categoryhash == "all" or categoryhash.length() < 2)
    {
        self.ui.ch_cat_all->setChecked(true);
        self.ui.ch_cat_delay->setChecked(false);
        self.ui.ch_cat_distortion->setChecked(false);
        self.ui.ch_cat_dynamics->setChecked(false);
        self.ui.ch_cat_eq->setChecked(false);
        self.ui.ch_cat_filter->setChecked(false);
        self.ui.ch_cat_modulator->setChecked(false);
        self.ui.ch_cat_synth->setChecked(false);
        self.ui.ch_cat_utility->setChecked(false);
        self.ui.ch_cat_other->setChecked(false);
    }
    else
    {
        self.ui.ch_cat_all->setChecked(false);
        self.ui.ch_cat_delay->setChecked(categoryhash.contains(":delay:"));
        self.ui.ch_cat_distortion->setChecked(categoryhash.contains(":distortion:"));
        self.ui.ch_cat_dynamics->setChecked(categoryhash.contains(":dynamics:"));
        self.ui.ch_cat_eq->setChecked(categoryhash.contains(":eq:"));
        self.ui.ch_cat_filter->setChecked(categoryhash.contains(":filter:"));
        self.ui.ch_cat_modulator->setChecked(categoryhash.contains(":modulator:"));
        self.ui.ch_cat_synth->setChecked(categoryhash.contains(":synth:"));
        self.ui.ch_cat_utility->setChecked(categoryhash.contains(":utility:"));
        self.ui.ch_cat_other->setChecked(categoryhash.contains(":other:"));
    }

    QByteArray tableGeometry = settings.valueByteArray("PluginDatabase/TableGeometry_6");
    QHeaderView* const horizontalHeader = self.ui.tableWidget->horizontalHeader();
    if (! tableGeometry.isNull())
    {
        horizontalHeader->restoreState(tableGeometry);
    }
    else
    {
        horizontalHeader->setSectionResizeMode(self.TABLEWIDGET_ITEM_FAVORITE, QHeaderView::Fixed);
        self.ui.tableWidget->setColumnWidth(self.TABLEWIDGET_ITEM_FAVORITE, 24);
        self.ui.tableWidget->setColumnWidth(self.TABLEWIDGET_ITEM_NAME, 250);
        self.ui.tableWidget->setColumnWidth(self.TABLEWIDGET_ITEM_LABEL, 200);
        self.ui.tableWidget->setColumnWidth(self.TABLEWIDGET_ITEM_MAKER, 150);
        self.ui.tableWidget->sortByColumn(self.TABLEWIDGET_ITEM_NAME, Qt::AscendingOrder);
    }
}

// -----------------------------------------------------------------------------------------------------------------
// private slots

void PluginListDialog::slot_cellClicked(int row, int column)
{
    if (column == self.TABLEWIDGET_ITEM_FAVORITE)
    {
        QTableWidgetItem* const widget = self.ui.tableWidget->item(row, self.TABLEWIDGET_ITEM_FAVORITE);
        // plugin = self.ui.tableWidget.item(row, self.TABLEWIDGET_ITEM_NAME).data(Qt.UserRole+1);
        // plugin = self._createFavoritePluginDict(plugin);

        if (widget->checkState() == Qt::Checked)
        {
//             if (not plugin in self.fFavoritePlugins)
//             {
//                 self.fFavoritePlugins.append(plugin);
//                 self.fFavoritePluginsChanged = true;
//             }
        }
        else
        {
//             try:
//                 self.fFavoritePlugins.remove(plugin);
//                 self.fFavoritePluginsChanged = True;
//             except ValueError:
//                 pass
        }
    }
}

void PluginListDialog::slot_cellDoubleClicked(int, int column)
{
    if (column != self.TABLEWIDGET_ITEM_FAVORITE)
        slot_addPlugin();
}

void PluginListDialog::slot_focusSearchFieldAndSelectAll()
{
    self.ui.lineEdit->setFocus();
    self.ui.lineEdit->selectAll();
}

void PluginListDialog::slot_addPlugin()
{
    if (self.ui.tableWidget->currentRow() >= 0)
    {
//         self.fRetPlugin = self.ui.tableWidget->item(self.ui.tableWidget->currentRow(),
//                                                     self.TABLEWIDGET_ITEM_NAME).data(Qt.UserRole+1);
        accept();
    }
    else
    {
        reject();
    }
}

void PluginListDialog::slot_checkPlugin(int row)
{
    if (row >= 0)
    {
        self.ui.b_add->setEnabled(true);
//         auto plugin = self.ui.tableWidget.item(self.ui.tableWidget.currentRow(),
//                                                self.TABLEWIDGET_ITEM_NAME).data(Qt::UserRole+1);
//
//         const bool isSynth  = plugin['hints'] & PLUGIN_IS_SYNTH;
//         const bool isEffect = plugin['audio.ins'] > 0 && plugin['audio.outs'] > 0 && !isSynth;
//         const bool isMidi   = plugin['audio.ins'] == 0 &&
//                               plugin['audio.outs'] == 0 &&
//                               plugin['midi.ins'] > 0 < plugin['midi.outs'];
//         // const bool isKit    = plugin['type'] in (PLUGIN_SF2, PLUGIN_SFZ);
//         // const bool isOther  = ! (isEffect || isSynth || isMidi || isKit);

        QString ptype;
//         /**/ if (isSynth)
//             ptype = "Instrument";
//         else if (isEffect)
//             ptype = "Effect";
//         else if (isMidi)
//             ptype = "MIDI Plugin";
//         else
//             ptype = "Other";

        QString parch;
//         /**/ if (plugin['build'] == BINARY_NATIVE)
//             parch = self.fTrNative;
//         else if (plugin['build'] == BINARY_POSIX32)
//             parch = "posix32";
//         else if (plugin['build'] == BINARY_POSIX64)
//             parch = "posix64";
//         else if (plugin['build'] == BINARY_WIN32)
//             parch = "win32";
//         else if (plugin['build'] == BINARY_WIN64)
//             parch = "win64";
//         else if (plugin['build'] == BINARY_OTHER)
//             parch = tr("Other");
//         else if (plugin['build'] == BINARY_WIN32)
//             parch = tr("Unknown");

//         self.ui.l_format->setText(getPluginTypeAsString(plugin['type']));
        self.ui.l_type->setText(ptype);
        self.ui.l_arch->setText(parch);
//         self.ui.l_id->setText(str(plugin['uniqueId']));
//         self.ui.l_ains->setText(str(plugin['audio.ins']));
//         self.ui.l_aouts->setText(str(plugin['audio.outs']));
//         self.ui.l_cvins->setText(str(plugin['cv.ins']));
//         self.ui.l_cvouts->setText(str(plugin['cv.outs']));
//         self.ui.l_mins->setText(str(plugin['midi.ins']));
//         self.ui.l_mouts->setText(str(plugin['midi.outs']));
//         self.ui.l_pins->setText(str(plugin['parameters.ins']));
//         self.ui.l_pouts->setText(str(plugin['parameters.outs']));
//         self.ui.l_gui->setText(plugin['hints'] & PLUGIN_HAS_CUSTOM_UI ? self.fTrYes : self.fTrNo);
//         self.ui.l_idisp->setText(plugin['hints'] & PLUGIN_HAS_INLINE_DISPLAY ? self.fTrYes : self.fTrNo);
//         self.ui.l_bridged->setText(plugin['hints'] & PLUGIN_IS_BRIDGE ? self.fTrYes : self.fTrNo);
//         self.ui.l_synth->setText(isSynth ? self.fTrYes : self.fTrNo);
    }
    else
    {
        self.ui.b_add->setEnabled(false);
        self.ui.l_format->setText("---");
        self.ui.l_type->setText("---");
        self.ui.l_arch->setText("---");
        self.ui.l_id->setText("---");
        self.ui.l_ains->setText("---");
        self.ui.l_aouts->setText("---");
        self.ui.l_cvins->setText("---");
        self.ui.l_cvouts->setText("---");
        self.ui.l_mins->setText("---");
        self.ui.l_mouts->setText("---");
        self.ui.l_pins->setText("---");
        self.ui.l_pouts->setText("---");
        self.ui.l_gui->setText("---");
        self.ui.l_idisp->setText("---");
        self.ui.l_bridged->setText("---");
        self.ui.l_synth->setText("---");
    }
}

void PluginListDialog::slot_checkFilters()
{
    // TODO
}

void PluginListDialog::slot_checkFiltersCategoryAll(const bool clicked)
{
    const bool notClicked = !clicked;
    self.ui.ch_cat_delay->setChecked(notClicked);
    self.ui.ch_cat_distortion->setChecked(notClicked);
    self.ui.ch_cat_dynamics->setChecked(notClicked);
    self.ui.ch_cat_eq->setChecked(notClicked);
    self.ui.ch_cat_filter->setChecked(notClicked);
    self.ui.ch_cat_modulator->setChecked(notClicked);
    self.ui.ch_cat_synth->setChecked(notClicked);
    self.ui.ch_cat_utility->setChecked(notClicked);
    self.ui.ch_cat_other->setChecked(notClicked);
    slot_checkFilters();
}

void PluginListDialog::slot_checkFiltersCategorySpecific(bool clicked)
{
    if (clicked)
    {
        self.ui.ch_cat_all->setChecked(false);
    }
    else if (! (self.ui.ch_cat_delay->isChecked() ||
                self.ui.ch_cat_distortion->isChecked() ||
                self.ui.ch_cat_dynamics->isChecked() ||
                self.ui.ch_cat_eq->isChecked() ||
                self.ui.ch_cat_filter->isChecked() ||
                self.ui.ch_cat_modulator->isChecked() ||
                self.ui.ch_cat_synth->isChecked() ||
                self.ui.ch_cat_utility->isChecked() ||
                self.ui.ch_cat_other->isChecked()))
    {
        self.ui.ch_cat_all->setChecked(true);
    }
    slot_checkFilters();
}

void PluginListDialog::slot_refreshPlugins()
{
//     if (PluginRefreshW(this, self.hostSettings, self.hasLoadedLv2Plugins).exec())
//         reAddPlugins();
}

void PluginListDialog::slot_clearFilters()
{
    blockSignals(true);

    self.ui.ch_internal->setChecked(true);
    self.ui.ch_ladspa->setChecked(true);
    self.ui.ch_dssi->setChecked(true);
    self.ui.ch_lv2->setChecked(true);
    self.ui.ch_vst->setChecked(true);
    self.ui.ch_vst3->setChecked(true);
    self.ui.ch_clap->setChecked(true);
    self.ui.ch_jsfx->setChecked(true);
    self.ui.ch_kits->setChecked(true);

    self.ui.ch_instruments->setChecked(true);
    self.ui.ch_effects->setChecked(true);
    self.ui.ch_midi->setChecked(true);
    self.ui.ch_other->setChecked(true);

    self.ui.ch_native->setChecked(true);
    self.ui.ch_bridged->setChecked(false);
    self.ui.ch_bridged_wine->setChecked(false);

    self.ui.ch_favorites->setChecked(false);
    self.ui.ch_rtsafe->setChecked(false);
    self.ui.ch_stereo->setChecked(false);
    self.ui.ch_cv->setChecked(false);
    self.ui.ch_gui->setChecked(false);
    self.ui.ch_inline_display->setChecked(false);

    if (self.ui.ch_au->isEnabled())
        self.ui.ch_au->setChecked(true);

    self.ui.ch_cat_all->setChecked(true);
    self.ui.ch_cat_delay->setChecked(false);
    self.ui.ch_cat_distortion->setChecked(false);
    self.ui.ch_cat_dynamics->setChecked(false);
    self.ui.ch_cat_eq->setChecked(false);
    self.ui.ch_cat_filter->setChecked(false);
    self.ui.ch_cat_modulator->setChecked(false);
    self.ui.ch_cat_synth->setChecked(false);
    self.ui.ch_cat_utility->setChecked(false);
    self.ui.ch_cat_other->setChecked(false);

    self.ui.lineEdit->clear();

    blockSignals(false);

    slot_checkFilters();
}

// --------------------------------------------------------------------------------------------------------------------

void PluginListDialog::slot_saveSettings()
{
    QSafeSettings settings("falkTX", "CarlaDatabase2");
    settings.setValue("PluginDatabase/Geometry", saveGeometry());
    settings.setValue("PluginDatabase/TableGeometry_6", self.ui.tableWidget->horizontalHeader()->saveState());
    settings.setValue("PluginDatabase/ShowEffects", self.ui.ch_effects->isChecked());
    settings.setValue("PluginDatabase/ShowInstruments", self.ui.ch_instruments->isChecked());
    settings.setValue("PluginDatabase/ShowMIDI", self.ui.ch_midi->isChecked());
    settings.setValue("PluginDatabase/ShowOther", self.ui.ch_other->isChecked());
    settings.setValue("PluginDatabase/ShowInternal", self.ui.ch_internal->isChecked());
    settings.setValue("PluginDatabase/ShowLADSPA", self.ui.ch_ladspa->isChecked());
    settings.setValue("PluginDatabase/ShowDSSI", self.ui.ch_dssi->isChecked());
    settings.setValue("PluginDatabase/ShowLV2", self.ui.ch_lv2->isChecked());
    settings.setValue("PluginDatabase/ShowVST2", self.ui.ch_vst->isChecked());
    settings.setValue("PluginDatabase/ShowVST3", self.ui.ch_vst3->isChecked());
    settings.setValue("PluginDatabase/ShowCLAP", self.ui.ch_clap->isChecked());
    settings.setValue("PluginDatabase/ShowAU", self.ui.ch_au->isChecked());
    settings.setValue("PluginDatabase/ShowJSFX", self.ui.ch_jsfx->isChecked());
    settings.setValue("PluginDatabase/ShowKits", self.ui.ch_kits->isChecked());
    settings.setValue("PluginDatabase/ShowNative", self.ui.ch_native->isChecked());
    settings.setValue("PluginDatabase/ShowBridged", self.ui.ch_bridged->isChecked());
    settings.setValue("PluginDatabase/ShowBridgedWine", self.ui.ch_bridged_wine->isChecked());
    settings.setValue("PluginDatabase/ShowFavorites", self.ui.ch_favorites->isChecked());
    settings.setValue("PluginDatabase/ShowRtSafe", self.ui.ch_rtsafe->isChecked());
    settings.setValue("PluginDatabase/ShowHasCV", self.ui.ch_cv->isChecked());
    settings.setValue("PluginDatabase/ShowHasGUI", self.ui.ch_gui->isChecked());
    settings.setValue("PluginDatabase/ShowHasInlineDisplay", self.ui.ch_inline_display->isChecked());
    settings.setValue("PluginDatabase/ShowStereoOnly", self.ui.ch_stereo->isChecked());
    settings.setValue("PluginDatabase/SearchText", self.ui.lineEdit->text());

    if (self.ui.ch_cat_all->isChecked())
    {
        settings.setValue("PluginDatabase/ShowCategory", "all");
    }
    else
    {
        QCarlaString categoryhash;
        if (self.ui.ch_cat_delay->isChecked())
            categoryhash += ":delay";
        if (self.ui.ch_cat_distortion->isChecked())
            categoryhash += ":distortion";
        if (self.ui.ch_cat_dynamics->isChecked())
            categoryhash += ":dynamics";
        if (self.ui.ch_cat_eq->isChecked())
            categoryhash += ":eq";
        if (self.ui.ch_cat_filter->isChecked())
            categoryhash += ":filter";
        if (self.ui.ch_cat_modulator->isChecked())
            categoryhash += ":modulator";
        if (self.ui.ch_cat_synth->isChecked())
            categoryhash += ":synth";
        if (self.ui.ch_cat_utility->isChecked())
            categoryhash += ":utility";
        if (self.ui.ch_cat_other->isChecked())
            categoryhash += ":other";
        if (categoryhash.isNotEmpty())
            categoryhash += ":";
        settings.setValue("PluginDatabase/ShowCategory", categoryhash);
    }

    if (self.fFavoritePluginsChanged)
        settings.setValue("PluginDatabase/Favorites", self.fFavoritePlugins);
}

// --------------------------------------------------------------------------------------------------------------------

PluginListDialogResults*
carla_frontend_createAndExecPluginListDialog(void* const parent/*, const HostSettings& hostSettings*/)
{
    const HostSettings hostSettings = {};
    PluginListDialog gui(reinterpret_cast<QWidget*>(parent), hostSettings);

    if (gui.exec())
    {
        static PluginListDialogResults ret = {};
        static CarlaString retBinary;

        // TODO

        return &ret;
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
