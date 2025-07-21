// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "carla_frontend.h"

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

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

typedef struct _CarlaPluginDiscoveryInfo CarlaPluginDiscoveryInfo;
typedef struct _HostSettings HostSettings;

// --------------------------------------------------------------------------------------------------------------------

// base details, nicely packed and POD-only so we can directly use as binary
struct PluginInfoHeader {
    uint16_t build;
    uint16_t type;
    uint32_t hints;
    uint64_t uniqueId;
    uint16_t audioIns;
    uint16_t audioOuts;
    uint16_t cvIns;
    uint16_t cvOuts;
    uint16_t midiIns;
    uint16_t midiOuts;
    uint16_t parameterIns;
    uint16_t parameterOuts;
};

// full details, now with non-POD types
struct PluginInfo : PluginInfoHeader {
    QString category;
    QString filename;
    QString name;
    QString label;
    QString maker;
};

// --------------------------------------------------------------------------------------------------------------------
// Plugin List Dialog

class PluginListDialog : public QDialog
{
    enum TableIndex {
        TW_FAVORITE,
        TW_NAME,
        TW_LABEL,
        TW_MAKER,
        TW_BINARY,
    };

    enum UserRoles {
        UR_PLUGIN_INFO = 1,
        UR_SEARCH_TEXT,
    };

    struct PrivateData;
    PrivateData *const p;

    Ui_PluginListDialog ui;

    // ----------------------------------------------------------------------------------------------------------------
    // public methods

public:
    explicit PluginListDialog(QWidget* parent, const HostSettings* hostSettings);
    ~PluginListDialog() override;

    const PluginInfo& getSelectedPluginInfo() const;
    void addPluginInfo(const CarlaPluginDiscoveryInfo* info, const char* sha1sum);
    bool checkPluginCache(const char* filename, const char* sha1sum);
    void setPluginPath(PluginType ptype, const char* path);

    // ----------------------------------------------------------------------------------------------------------------
    // protected methods

protected:
    void done(int) override;
    void showEvent(QShowEvent*) override;
    void timerEvent(QTimerEvent*) override;

    // ----------------------------------------------------------------------------------------------------------------
    // private methods

private:
    void addPluginsToTable();
    void loadSettings();

    // ----------------------------------------------------------------------------------------------------------------
    // private slots

private Q_SLOTS:
    void cellClicked(int row, int column);
    void cellDoubleClicked(int row, int column);
    void focusSearchFieldAndSelectAll();
    void checkFilters();
    void checkFiltersCategoryAll(bool clicked);
    void checkFiltersCategorySpecific(bool clicked);
    void clearFilters();
    void checkPlugin(int row);
    void refreshPlugins();
    void refreshPluginsStart();
    void refreshPluginsStop();
    void refreshPluginsSkip();
    void saveSettings();
};

// --------------------------------------------------------------------------------------------------------------------
