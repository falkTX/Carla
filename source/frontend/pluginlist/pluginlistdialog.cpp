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
// Plugin List Dialog

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
    HostSettings hostSettings = {};

    int fLastTableIndex = 0;
    PluginListDialogResults fRetPlugin = {};
    QWidget* const fRealParent;
    QStringList fFavoritePlugins;
    bool fFavoritePluginsChanged = false;

    const QString fTrYes;
    const QString fTrNo;
    const QString fTrNative;

    Self(QWidget* const parent)
        : fRealParent(parent),
          fTrYes(tr("Yes")),
          fTrNo(tr("No")),
          fTrNative(tr("Native")) {}

    static Self& create(QWidget* const parent)
    {
        Self* const self = new Self(parent);
        return *self;
    }

    void createFavoritePluginDict()
    {
#if 0
        return {
            'name'    : plugin['name'],
            'build'   : plugin['build'],
            'type'    : plugin['type'],
            'filename': plugin['filename'],
            'label'   : plugin['label'],
            'uniqueId': plugin['uniqueId'],
        }
#endif
    }

    void checkFilters()
    {
#if 0
        text = self.ui.lineEdit.text().lower()

        hideEffects     = not self.ui.ch_effects.isChecked()
        hideInstruments = not self.ui.ch_instruments.isChecked()
        hideMidi        = not self.ui.ch_midi.isChecked()
        hideOther       = not self.ui.ch_other.isChecked()

        hideInternal = not self.ui.ch_internal.isChecked()
        hideLadspa   = not self.ui.ch_ladspa.isChecked()
        hideDssi     = not self.ui.ch_dssi.isChecked()
        hideLV2      = not self.ui.ch_lv2.isChecked()
        hideVST2     = not self.ui.ch_vst.isChecked()
        hideVST3     = not self.ui.ch_vst3.isChecked()
        hideCLAP     = not self.ui.ch_clap.isChecked()
        hideAU       = not self.ui.ch_au.isChecked()
        hideJSFX     = not self.ui.ch_jsfx.isChecked()
        hideKits     = not self.ui.ch_kits.isChecked()

        hideNative  = not self.ui.ch_native.isChecked()
        hideBridged = not self.ui.ch_bridged.isChecked()
        hideBridgedWine = not self.ui.ch_bridged_wine.isChecked()

        hideNonFavs   = self.ui.ch_favorites.isChecked()
        hideNonRtSafe = self.ui.ch_rtsafe.isChecked()
        hideNonCV     = self.ui.ch_cv.isChecked()
        hideNonGui    = self.ui.ch_gui.isChecked()
        hideNonIDisp  = self.ui.ch_inline_display.isChecked()
        hideNonStereo = self.ui.ch_stereo.isChecked()

        if HAIKU or LINUX or MACOS:
            nativeBins = [BINARY_POSIX32, BINARY_POSIX64]
            wineBins   = [BINARY_WIN32, BINARY_WIN64]
        elif WINDOWS:
            nativeBins = [BINARY_WIN32, BINARY_WIN64]
            wineBins   = []
        else:
            nativeBins = []
            wineBins   = []

        self.ui.tableWidget.setRowCount(self.fLastTableIndex)

        for i in range(self.fLastTableIndex):
            plugin = self.ui.tableWidget.item(i, self.TABLEWIDGET_ITEM_NAME).data(Qt.UserRole+1)
            ptext  = self.ui.tableWidget.item(i, self.TABLEWIDGET_ITEM_NAME).data(Qt.UserRole+2)
            aIns   = plugin['audio.ins']
            aOuts  = plugin['audio.outs']
            cvIns  = plugin['cv.ins']
            cvOuts = plugin['cv.outs']
            mIns   = plugin['midi.ins']
            mOuts  = plugin['midi.outs']
            phints = plugin['hints']
            ptype  = plugin['type']
            categ  = plugin['category']
            isSynth  = bool(phints & PLUGIN_IS_SYNTH)
            isEffect = bool(aIns > 0 < aOuts and not isSynth)
            isMidi   = bool(aIns == 0 and aOuts == 0 and mIns > 0 < mOuts)
            isKit    = bool(ptype in (PLUGIN_SF2, PLUGIN_SFZ))
            isOther  = bool(not (isEffect or isSynth or isMidi or isKit))
            isNative = bool(plugin['build'] == BINARY_NATIVE)
            isRtSafe = bool(phints & PLUGIN_IS_RTSAFE)
            isStereo = bool(aIns == 2 and aOuts == 2) or (isSynth and aOuts == 2)
            hasCV    = bool(cvIns + cvOuts > 0)
            hasGui   = bool(phints & PLUGIN_HAS_CUSTOM_UI)
            hasIDisp = bool(phints & PLUGIN_HAS_INLINE_DISPLAY)

            isBridged = bool(not isNative and plugin['build'] in nativeBins)
            isBridgedWine = bool(not isNative and plugin['build'] in wineBins)

            if hideEffects and isEffect:
                self.ui.tableWidget.hideRow(i)
            elif hideInstruments and isSynth:
                self.ui.tableWidget.hideRow(i)
            elif hideMidi and isMidi:
                self.ui.tableWidget.hideRow(i)
            elif hideOther and isOther:
                self.ui.tableWidget.hideRow(i)
            elif hideKits and isKit:
                self.ui.tableWidget.hideRow(i)
            elif hideInternal and ptype == PLUGIN_INTERNAL:
                self.ui.tableWidget.hideRow(i)
            elif hideLadspa and ptype == PLUGIN_LADSPA:
                self.ui.tableWidget.hideRow(i)
            elif hideDssi and ptype == PLUGIN_DSSI:
                self.ui.tableWidget.hideRow(i)
            elif hideLV2 and ptype == PLUGIN_LV2:
                self.ui.tableWidget.hideRow(i)
            elif hideVST2 and ptype == PLUGIN_VST2:
                self.ui.tableWidget.hideRow(i)
            elif hideVST3 and ptype == PLUGIN_VST3:
                self.ui.tableWidget.hideRow(i)
            elif hideCLAP and ptype == PLUGIN_CLAP:
                self.ui.tableWidget.hideRow(i)
            elif hideAU and ptype == PLUGIN_AU:
                self.ui.tableWidget.hideRow(i)
            elif hideJSFX and ptype == PLUGIN_JSFX:
                self.ui.tableWidget.hideRow(i)
            elif hideNative and isNative:
                self.ui.tableWidget.hideRow(i)
            elif hideBridged and isBridged:
                self.ui.tableWidget.hideRow(i)
            elif hideBridgedWine and isBridgedWine:
                self.ui.tableWidget.hideRow(i)
            elif hideNonRtSafe and not isRtSafe:
                self.ui.tableWidget.hideRow(i)
            elif hideNonCV and not hasCV:
                self.ui.tableWidget.hideRow(i)
            elif hideNonGui and not hasGui:
                self.ui.tableWidget.hideRow(i)
            elif hideNonIDisp and not hasIDisp:
                self.ui.tableWidget.hideRow(i)
            elif hideNonStereo and not isStereo:
                self.ui.tableWidget.hideRow(i)
            elif text and not all(t in ptext for t in text.strip().split(' ')):
                self.ui.tableWidget.hideRow(i)
            elif hideNonFavs and self._createFavoritePluginDict(plugin) not in self.fFavoritePlugins:
                self.ui.tableWidget.hideRow(i)
            elif (self.ui.ch_cat_all.isChecked() or
                  (self.ui.ch_cat_delay.isChecked() and categ == "delay") or
                  (self.ui.ch_cat_distortion.isChecked() and categ == "distortion") or
                  (self.ui.ch_cat_dynamics.isChecked() and categ == "dynamics") or
                  (self.ui.ch_cat_eq.isChecked() and categ == "eq") or
                  (self.ui.ch_cat_filter.isChecked() and categ == "filter") or
                  (self.ui.ch_cat_modulator.isChecked() and categ == "modulator") or
                  (self.ui.ch_cat_synth.isChecked() and categ == "synth") or
                  (self.ui.ch_cat_utility.isChecked() and categ == "utility") or
                  (self.ui.ch_cat_other.isChecked() and categ == "other")):
                self.ui.tableWidget.showRow(i)
            else:
                self.ui.tableWidget.hideRow(i)
#endif
    }

    void addPluginToTable(uint plugin, const PluginType ptype)
    {
#if 0
        if plugin['API'] != PLUGIN_QUERY_API_VERSION:
            return
        if ptype in (self.tr("Internal"), "LV2", "SF2", "SFZ", "JSFX"):
            plugin['build'] = BINARY_NATIVE

        index = self.fLastTableIndex

        isFav = bool(self._createFavoritePluginDict(plugin) in self.fFavoritePlugins)
        favItem = QTableWidgetItem()
        favItem.setCheckState(Qt.Checked if isFav else Qt.Unchecked)
        favItem.setText(" " if isFav else "  ")

        pluginText = (plugin['name']+plugin['label']+plugin['maker']+plugin['filename']).lower()
        self.ui.tableWidget.setItem(index, self.TABLEWIDGET_ITEM_FAVORITE, favItem)
        self.ui.tableWidget.setItem(index, self.TABLEWIDGET_ITEM_NAME, QTableWidgetItem(plugin['name']))
        self.ui.tableWidget.setItem(index, self.TABLEWIDGET_ITEM_LABEL, QTableWidgetItem(plugin['label']))
        self.ui.tableWidget.setItem(index, self.TABLEWIDGET_ITEM_MAKER, QTableWidgetItem(plugin['maker']))
        self.ui.tableWidget.setItem(index, self.TABLEWIDGET_ITEM_BINARY, QTableWidgetItem(os.path.basename(plugin['filename'])))
        self.ui.tableWidget.item(index, self.TABLEWIDGET_ITEM_NAME).setData(Qt.UserRole+1, plugin)
        self.ui.tableWidget.item(index, self.TABLEWIDGET_ITEM_NAME).setData(Qt.UserRole+2, pluginText)

        self.fLastTableIndex += 1
#endif
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

#if 0
        for plugin in plugins:
            self._addPluginToTable(plugin, ptypeStrTr)
#endif

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
#if 0
            LV2_PATH = splitter.join()
            settings.valueStringList(CARLA_KEY_PATHS_LV2, CARLA_DEFAULT_LV2_PATH);
#endif
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

#if 0
        # ----------------------------------------------------------------------------------------------------
        # DSSI

        dssiPlugins  = []
        dssiPlugins += settingsDB.value("Plugins/DSSI_native", [], list)
        dssiPlugins += settingsDB.value("Plugins/DSSI_posix32", [], list)
        dssiPlugins += settingsDB.value("Plugins/DSSI_posix64", [], list)
        dssiPlugins += settingsDB.value("Plugins/DSSI_win32", [], list)
        dssiPlugins += settingsDB.value("Plugins/DSSI_win64", [], list)

        # ----------------------------------------------------------------------------------------------------
        # VST2

        vst2Plugins  = []
        vst2Plugins += settingsDB.value("Plugins/VST2_native", [], list)
        vst2Plugins += settingsDB.value("Plugins/VST2_posix32", [], list)
        vst2Plugins += settingsDB.value("Plugins/VST2_posix64", [], list)
        vst2Plugins += settingsDB.value("Plugins/VST2_win32", [], list)
        vst2Plugins += settingsDB.value("Plugins/VST2_win64", [], list)

        # ----------------------------------------------------------------------------------------------------
        # VST3

        vst3Plugins  = []
        vst3Plugins += settingsDB.value("Plugins/VST3_native", [], list)
        vst3Plugins += settingsDB.value("Plugins/VST3_posix32", [], list)
        vst3Plugins += settingsDB.value("Plugins/VST3_posix64", [], list)
        vst3Plugins += settingsDB.value("Plugins/VST3_win32", [], list)
        vst3Plugins += settingsDB.value("Plugins/VST3_win64", [], list)

        # ----------------------------------------------------------------------------------------------------
        # CLAP

        clapPlugins  = []
        clapPlugins += settingsDB.value("Plugins/CLAP_native", [], list)
        clapPlugins += settingsDB.value("Plugins/CLAP_posix32", [], list)
        clapPlugins += settingsDB.value("Plugins/CLAP_posix64", [], list)
        clapPlugins += settingsDB.value("Plugins/CLAP_win32", [], list)
        clapPlugins += settingsDB.value("Plugins/CLAP_win64", [], list)

        # ----------------------------------------------------------------------------------------------------
        # AU (extra non-cached)

        auPlugins32 = settingsDB.value("Plugins/AU_posix32", [], list) if MACOS else []

        # ----------------------------------------------------------------------------------------------------
        # JSFX

        jsfxPlugins = settingsDB.value("Plugins/JSFX", [], list)

        # ----------------------------------------------------------------------------------------------------
        # Kits

        sf2s = settingsDB.value("Plugins/SF2", [], list)
        sfzs = settingsDB.value("Plugins/SFZ", [], list)

        # ----------------------------------------------------------------------------------------------------
        # count plugins first, so we can create rows in advance

        ladspaCount = 0
        dssiCount   = 0
        vstCount    = 0
        vst3Count   = 0
        clapCount   = 0
        au32Count   = 0
        jsfxCount   = len(jsfxPlugins)
        sf2Count    = 0
        sfzCount    = len(sfzs)

        for plugins in ladspaPlugins:
            ladspaCount += len(plugins)

        for plugins in dssiPlugins:
            dssiCount += len(plugins)

        for plugins in vst2Plugins:
            vstCount += len(plugins)

        for plugins in vst3Plugins:
            vst3Count += len(plugins)

        for plugins in clapPlugins:
            clapCount += len(plugins)

        for plugins in auPlugins32:
            au32Count += len(plugins)

        for plugins in sf2s:
            sf2Count += len(plugins)

        self.ui.tableWidget.setRowCount(self.fLastTableIndex +
                                        ladspaCount + dssiCount + vstCount + vst3Count + clapCount +
                                        auCount + au32Count + jsfxCount + sf2Count + sfzCount)

        if MACOS:
            self.ui.label.setText(self.tr("Have %i Internal, %i LADSPA, %i DSSI, %i LV2, %i VST2, %i VST3, %i CLAP, %i AudioUnit and %i JSFX plugins, plus %i Sound Kits" % (
                                          internalCount, ladspaCount, dssiCount, lv2Count, vstCount, vst3Count, clapCount, auCount+au32Count, jsfxCount, sf2Count+sfzCount)))
        else:
            self.ui.label.setText(self.tr("Have %i Internal, %i LADSPA, %i DSSI, %i LV2, %i VST2, %i VST3, %i CLAP and %i JSFX plugins, plus %i Sound Kits" % (
                                          internalCount, ladspaCount, dssiCount, lv2Count, vstCount, vst3Count, clapCount, jsfxCount, sf2Count+sfzCount)))

        # ----------------------------------------------------------------------------------------------------
        # now add all plugins to the table

        for plugins in ladspaPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "LADSPA")

        for plugins in dssiPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "DSSI")

        for plugins in vst2Plugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "VST2")

        for plugins in vst3Plugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "VST3")

        for plugins in clapPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "CLAP")

        for plugins in auPlugins32:
            for plugin in plugins:
                self._addPluginToTable(plugin, "AU")

        for plugin in jsfxPlugins:
            self._addPluginToTable(plugin, "JSFX")

        for sf2 in sf2s:
            for sf2_i in sf2:
                self._addPluginToTable(sf2_i, "SF2")

        for sfz in sfzs:
            self._addPluginToTable(sfz, "SFZ")

        # ----------------------------------------------------------------------------------------------------

#endif
        ui.tableWidget->setSortingEnabled(true);
        checkFilters();

#if 0
        self.slot_checkPlugin(self.ui.tableWidget.currentRow())
#endif
    }
};

PluginListDialog::PluginListDialog(QWidget* const parent, const HostSettings& hostSettings)
    : QDialog(parent),
      self(Self::create(parent))
{
    self.ui.setupUi(this);
    self.hostSettings = hostSettings;

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up GUI

    self.ui.b_add->setEnabled(false);
    addAction(self.ui.act_focus_search);
    QObject::connect(self.ui.act_focus_search, &QAction::triggered,
                     this, &PluginListDialog::slot_focusSearchFieldAndSelectAll);

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

#if 0
    // NOTE: We Assume win32 carla build will not run win64 plugins
    if (WINDOWS and not kIs64bit) or not host.showPluginBridges:
        self.ui.ch_native.setChecked(True)
        self.ui.ch_native.setEnabled(False)
        self.ui.ch_native.setVisible(True)
        self.ui.ch_bridged.setChecked(False)
        self.ui.ch_bridged.setEnabled(False)
        self.ui.ch_bridged.setVisible(False)
        self.ui.ch_bridged_wine.setChecked(False)
        self.ui.ch_bridged_wine.setEnabled(False)
        self.ui.ch_bridged_wine.setVisible(False)

    elif not host.showWineBridges:
        self.ui.ch_bridged_wine.setChecked(False)
        self.ui.ch_bridged_wine.setEnabled(False)
        self.ui.ch_bridged_wine.setVisible(False)
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up Icons

    if (hostSettings.useSystemIcons)
    {
#if 0
        self.ui.b_add.setIcon(getIcon('list-add', 16, 'svgz'))
        self.ui.b_cancel.setIcon(getIcon('dialog-cancel', 16, 'svgz'))
        self.ui.b_clear_filters.setIcon(getIcon('edit-clear', 16, 'svgz'))
        self.ui.b_refresh.setIcon(getIcon('view-refresh', 16, 'svgz'))
        QTableWidgetItem* const hhi = self.ui.tableWidget->horizontalHeaderItem(self.TABLEWIDGET_ITEM_FAVORITE);
        hhi.setIcon(getIcon('bookmarks', 16, 'svgz'))
#endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    QObject::connect(this, &QDialog::finished, this, &PluginListDialog::slot_saveSettings);
    QObject::connect(self.ui.b_add, &QPushButton::clicked, this, &PluginListDialog::slot_addPlugin);
    QObject::connect(self.ui.b_cancel, &QPushButton::clicked, this, &QDialog::reject);

    QObject::connect(self.ui.b_refresh, &QPushButton::clicked, this, &PluginListDialog::slot_refreshPlugins);
    QObject::connect(self.ui.b_clear_filters, &QPushButton::clicked, this, &PluginListDialog::slot_clearFilters);
    QObject::connect(self.ui.lineEdit, &QLineEdit::textChanged, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.tableWidget, &QTableWidget::currentCellChanged,
                     this, &PluginListDialog::slot_checkPlugin);
    QObject::connect(self.ui.tableWidget, &QTableWidget::cellClicked,
                     this, &PluginListDialog::slot_cellClicked);
    QObject::connect(self.ui.tableWidget, &QTableWidget::cellDoubleClicked,
                     this, &PluginListDialog::slot_cellDoubleClicked);

    QObject::connect(self.ui.ch_internal, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_ladspa, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_dssi, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_lv2, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_vst, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_vst3, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_clap, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_au, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_jsfx, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_kits, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_effects, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_instruments, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_midi, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_other, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_native, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_bridged, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_bridged_wine, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_favorites, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_rtsafe, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_cv, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_gui, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_inline_display, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_stereo, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_cat_all, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategoryAll);
    QObject::connect(self.ui.ch_cat_delay, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_distortion, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_dynamics, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_eq, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_filter, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_modulator, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_synth, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_utility, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_other, &QCheckBox::clicked, this,
                     &PluginListDialog::slot_checkFiltersCategorySpecific);

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

    QString categories = settings.valueString("PluginDatabase/ShowCategory", "all");
    if (categories == "all" or categories.length() < 2)
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
        self.ui.ch_cat_delay->setChecked(categories.contains(":delay:"));
        self.ui.ch_cat_distortion->setChecked(categories.contains(":distortion:"));
        self.ui.ch_cat_dynamics->setChecked(categories.contains(":dynamics:"));
        self.ui.ch_cat_eq->setChecked(categories.contains(":eq:"));
        self.ui.ch_cat_filter->setChecked(categories.contains(":filter:"));
        self.ui.ch_cat_modulator->setChecked(categories.contains(":modulator:"));
        self.ui.ch_cat_synth->setChecked(categories.contains(":synth:"));
        self.ui.ch_cat_utility->setChecked(categories.contains(":utility:"));
        self.ui.ch_cat_other->setChecked(categories.contains(":other:"));
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
#if 0
        plugin = self.ui.tableWidget.item(row, self.TABLEWIDGET_ITEM_NAME).data(Qt::UserRole+1);
        plugin = self._createFavoritePluginDict(plugin);
#endif

        if (widget->checkState() == Qt::Checked)
        {
#if 0
            if (not plugin in self.fFavoritePlugins)
            {
                self.fFavoritePlugins.append(plugin);
                self.fFavoritePluginsChanged = true;
            }
#endif
        }
        else
        {
#if 0
            try:
                self.fFavoritePlugins.remove(plugin);
                self.fFavoritePluginsChanged = True;
            except ValueError:
                pass
#endif
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
#if 0
        self.fRetPlugin = self.ui.tableWidget->item(self.ui.tableWidget->currentRow(),
                                                    self.TABLEWIDGET_ITEM_NAME).data(Qt.UserRole+1);
#endif
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
#if 0
        auto plugin = self.ui.tableWidget.item(self.ui.tableWidget.currentRow(),
                                               self.TABLEWIDGET_ITEM_NAME).data(Qt::UserRole+1);

        const bool isSynth  = plugin['hints'] & PLUGIN_IS_SYNTH;
        const bool isEffect = plugin['audio.ins'] > 0 && plugin['audio.outs'] > 0 && !isSynth;
        const bool isMidi   = plugin['audio.ins'] == 0 &&
                              plugin['audio.outs'] == 0 &&
                              plugin['midi.ins'] > 0 < plugin['midi.outs'];
        // const bool isKit    = plugin['type'] in (PLUGIN_SF2, PLUGIN_SFZ);
        // const bool isOther  = ! (isEffect || isSynth || isMidi || isKit);
#endif

        QString ptype;
#if 0
        /**/ if (isSynth)
            ptype = "Instrument";
        else if (isEffect)
            ptype = "Effect";
        else if (isMidi)
            ptype = "MIDI Plugin";
        else
            ptype = "Other";
#endif

        QString parch;
#if 0
        /**/ if (plugin['build'] == BINARY_NATIVE)
            parch = self.fTrNative;
        else if (plugin['build'] == BINARY_POSIX32)
            parch = "posix32";
        else if (plugin['build'] == BINARY_POSIX64)
            parch = "posix64";
        else if (plugin['build'] == BINARY_WIN32)
            parch = "win32";
        else if (plugin['build'] == BINARY_WIN64)
            parch = "win64";
        else if (plugin['build'] == BINARY_OTHER)
            parch = tr("Other");
        else if (plugin['build'] == BINARY_WIN32)
            parch = tr("Unknown");

        self.ui.l_format->setText(getPluginTypeAsString(plugin['type']));
#endif

        self.ui.l_type->setText(ptype);
        self.ui.l_arch->setText(parch);
#if 0
        self.ui.l_id->setText(str(plugin['uniqueId']));
        self.ui.l_ains->setText(str(plugin['audio.ins']));
        self.ui.l_aouts->setText(str(plugin['audio.outs']));
        self.ui.l_cvins->setText(str(plugin['cv.ins']));
        self.ui.l_cvouts->setText(str(plugin['cv.outs']));
        self.ui.l_mins->setText(str(plugin['midi.ins']));
        self.ui.l_mouts->setText(str(plugin['midi.outs']));
        self.ui.l_pins->setText(str(plugin['parameters.ins']));
        self.ui.l_pouts->setText(str(plugin['parameters.outs']));
        self.ui.l_gui->setText(plugin['hints'] & PLUGIN_HAS_CUSTOM_UI ? self.fTrYes : self.fTrNo);
        self.ui.l_idisp->setText(plugin['hints'] & PLUGIN_HAS_INLINE_DISPLAY ? self.fTrYes : self.fTrNo);
        self.ui.l_bridged->setText(plugin['hints'] & PLUGIN_IS_BRIDGE ? self.fTrYes : self.fTrNo);
        self.ui.l_synth->setText(isSynth ? self.fTrYes : self.fTrNo);
#endif
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
    self.checkFilters();
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
    self.checkFilters();
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
    self.checkFilters();
}

void PluginListDialog::slot_refreshPlugins()
{
#if 0
    if (PluginRefreshW(this, self.hostSettings, self.hasLoadedLv2Plugins).exec())
        reAddPlugins();
#endif
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

    self.checkFilters();
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
        QCarlaString categories;
        if (self.ui.ch_cat_delay->isChecked())
            categories += ":delay";
        if (self.ui.ch_cat_distortion->isChecked())
            categories += ":distortion";
        if (self.ui.ch_cat_dynamics->isChecked())
            categories += ":dynamics";
        if (self.ui.ch_cat_eq->isChecked())
            categories += ":eq";
        if (self.ui.ch_cat_filter->isChecked())
            categories += ":filter";
        if (self.ui.ch_cat_modulator->isChecked())
            categories += ":modulator";
        if (self.ui.ch_cat_synth->isChecked())
            categories += ":synth";
        if (self.ui.ch_cat_utility->isChecked())
            categories += ":utility";
        if (self.ui.ch_cat_other->isChecked())
            categories += ":other";
        if (categories.isNotEmpty())
            categories += ":";
        settings.setValue("PluginDatabase/ShowCategory", categories);
    }

    if (self.fFavoritePluginsChanged)
        settings.setValue("PluginDatabase/Favorites", self.fFavoritePlugins);
}

// --------------------------------------------------------------------------------------------------------------------

const PluginListDialogResults*
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
