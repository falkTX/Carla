// SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CARLA_UTILS_H_INCLUDED
#define CARLA_UTILS_H_INCLUDED

#include "CarlaBackend.h"

#ifdef __cplusplus
using CARLA_BACKEND_NAMESPACE::BinaryType;
using CARLA_BACKEND_NAMESPACE::EngineOption;
using CARLA_BACKEND_NAMESPACE::PluginCategory;
using CARLA_BACKEND_NAMESPACE::PluginType;
#endif

/*!
 * @defgroup CarlaUtilsAPI Carla Utils API
 *
 * The Carla Utils API.
 *
 * This API allows to call advanced features from Python.
 * @{
 */

/* --------------------------------------------------------------------------------------------------------------------
 * plugin discovery */

typedef void* CarlaPluginDiscoveryHandle;

/*!
 * Plugin discovery meta-data.
 * These are extra fields not required for loading plugins but still useful for showing to the user.
 */
typedef struct _CarlaPluginDiscoveryMetadata {
    /*!
     * Plugin name.
     */
    const char* name;

    /*!
     * Plugin author/maker.
     */
    const char* maker;

    /*!
     * Plugin category.
     */
    PluginCategory category;

    /*!
     * Plugin hints.
     * @see PluginHints
     */
    uint hints;

#ifdef __cplusplus
    /*!
     * C++ constructor.
     */
    CARLA_API _CarlaPluginDiscoveryMetadata() noexcept;
    CARLA_DECLARE_NON_COPYABLE(_CarlaPluginDiscoveryMetadata)
#endif

} CarlaPluginDiscoveryMetadata;

/*!
 * Plugin discovery input/output information.
 * These are extra fields not required for loading plugins but still useful for extra filtering.
 */
typedef struct _CarlaPluginDiscoveryIO {
    /*!
     * Number of audio inputs.
     */
    uint32_t audioIns;

    /*!
     * Number of audio outputs.
     */
    uint32_t audioOuts;

    /*!
     * Number of CV inputs.
     */
    uint32_t cvIns;

    /*!
     * Number of CV outputs.
     */
    uint32_t cvOuts;

    /*!
     * Number of MIDI inputs.
     */
    uint32_t midiIns;

    /*!
     * Number of MIDI outputs.
     */
    uint32_t midiOuts;

    /*!
     * Number of input parameters.
     */
    uint32_t parameterIns;

    /*!
     * Number of output parameters.
     */
    uint32_t parameterOuts;

#ifdef __cplusplus
    /*!
     * C++ constructor.
     */
    CARLA_API _CarlaPluginDiscoveryIO() noexcept;
    CARLA_DECLARE_NON_COPYABLE(_CarlaPluginDiscoveryIO)
#endif

} CarlaPluginDiscoveryIO;

/*!
 * Plugin discovery information.
 */
typedef struct _CarlaPluginDiscoveryInfo {
    /*!
     * Binary type.
     */
    BinaryType btype;

    /*!
     * Plugin type.
     */
    PluginType ptype;

    /*!
     * Plugin filename.
     */
    const char* filename;

    /*!
     * Plugin label/URI/Id.
     */
    const char* label;

    /*!
     * Plugin unique Id.
     */
    uint64_t uniqueId;

    /*!
     * Extra information, not required for load plugins.
     */
    CarlaPluginDiscoveryMetadata metadata;

    /*!
     * Extra information, not required for load plugins.
     */
    CarlaPluginDiscoveryIO io;

#ifdef __cplusplus
    /*!
     * C++ constructor.
     */
    CARLA_API _CarlaPluginDiscoveryInfo() noexcept;
    CARLA_DECLARE_NON_COPYABLE(_CarlaPluginDiscoveryInfo)
#endif

} CarlaPluginDiscoveryInfo;

/*!
 * Callback triggered when either a plugin has been found, or a scanned binary contains no plugins.
 *
 * For the case of plugins found while discovering @p info will be valid.
 * On formats where discovery is expensive, @p sha1 will contain a string hash related to the binary being scanned.
 *
 * When a plugin binary contains no actual plugins, @p info will be null but @p sha1 is valid.
 * This allows to mark a plugin binary as scanned, even without plugins, so we dont bother to check again next time.
 *
 * @note This callback might be triggered multiple times for a single binary, and thus for a single hash too.
 */
typedef void (*CarlaPluginDiscoveryCallback)(void* ptr, const CarlaPluginDiscoveryInfo* info, const char* sha1);

/*!
 * Optional callback triggered before starting to scan a plugin binary.
 * This allows to load plugin information from cache and skip scanning for that binary.
 * Return true to skip loading the binary specified by @p filename.
 */
typedef bool (*CarlaPluginCheckCacheCallback)(void* ptr, const char* filename, const char* sha1);

/*!
 * Start plugin discovery with the selected tool (must be absolute filename to executable file).
 * Different plugin types/formats must be scanned independently.
 * The path specified by @pluginPath can contain path separators (so ";" on Windows, ":" everywhere else).
 * The @p discoveryCb is required, while @p checkCacheCb is optional.
 * 
 * Returns a non-null handle if there are plugins to be scanned.
 * @a carla_plugin_discovery_idle must be called at regular intervals afterwards.
 */
CARLA_PLUGIN_EXPORT CarlaPluginDiscoveryHandle carla_plugin_discovery_start(const char* discoveryTool,
                                                                            BinaryType btype,
                                                                            PluginType ptype,
                                                                            const char* pluginPath,
                                                                            CarlaPluginDiscoveryCallback discoveryCb,
                                                                            CarlaPluginCheckCacheCallback checkCacheCb,
                                                                            void* callbackPtr);

/*!
 * Continue discovering plugins, triggering callbacks along the way.
 * Returns false when there is nothing else to scan, then you MUST call @a carla_plugin_discovery_stop.
 */
CARLA_PLUGIN_EXPORT bool carla_plugin_discovery_idle(CarlaPluginDiscoveryHandle handle);

/*!
 * Skip the current plugin being discovered.
 * Carla automatically skips a plugin if 30s have passed without a reply from the discovery side,
 * this function allows to manually abort earlier than that.
 */
CARLA_PLUGIN_EXPORT void carla_plugin_discovery_skip(CarlaPluginDiscoveryHandle handle);

/*!
 * Stop plugin discovery.
 * Can be called early while the scanning is still active.
 */
CARLA_PLUGIN_EXPORT void carla_plugin_discovery_stop(CarlaPluginDiscoveryHandle handle);

/*!
 * Set a plugin discovery setting, to be applied globally.
 */
CARLA_PLUGIN_EXPORT void carla_plugin_discovery_set_option(EngineOption option, int value, const char* valueStr);

/* --------------------------------------------------------------------------------------------------------------------
 * cached plugins */

/*!
 * Information about a cached plugin.
 * @see carla_get_cached_plugin_info()
 */
typedef struct _CarlaCachedPluginInfo {
    /*!
     * Wherever the data in this struct is valid.
     * For performance reasons, plugins are only checked on request,
     *  and as such, the count vs number of really valid plugins might not match.
     * Use this field to skip on plugins which cannot be loaded in Carla.
     */
    bool valid;

    /*!
     * Plugin category.
     */
    PluginCategory category;

    /*!
     * Plugin hints.
     * @see PluginHints
     */
    uint hints;

    /*!
     * Number of audio inputs.
     */
    uint32_t audioIns;

    /*!
     * Number of audio outputs.
     */
    uint32_t audioOuts;

    /*!
     * Number of CV inputs.
     */
    uint32_t cvIns;

    /*!
     * Number of CV outputs.
     */
    uint32_t cvOuts;

    /*!
     * Number of MIDI inputs.
     */
    uint32_t midiIns;

    /*!
     * Number of MIDI outputs.
     */
    uint32_t midiOuts;

    /*!
     * Number of input parameters.
     */
    uint32_t parameterIns;

    /*!
     * Number of output parameters.
     */
    uint32_t parameterOuts;

    /*!
     * Plugin name.
     */
    const char* name;

    /*!
     * Plugin label.
     */
    const char* label;

    /*!
     * Plugin author/maker.
     */
    const char* maker;

    /*!
     * Plugin copyright/license.
     */
    const char* copyright;

#ifdef __cplusplus
    /*!
     * C++ constructor.
     */
    CARLA_API _CarlaCachedPluginInfo() noexcept;
    CARLA_DECLARE_NON_COPYABLE(_CarlaCachedPluginInfo)
#endif

} CarlaCachedPluginInfo;

/*!
 * Get how many cached plugins are available.
 * Internal, LV2 and SFZ plugin formats are cached and can be discovered via this function.
 * Do not call this for any other plugin formats.
 */
CARLA_PLUGIN_EXPORT uint carla_get_cached_plugin_count(PluginType ptype, const char* pluginPath);

/*!
 * Get information about a cached plugin.
 */
CARLA_PLUGIN_EXPORT const CarlaCachedPluginInfo* carla_get_cached_plugin_info(PluginType ptype, uint index);

#ifndef CARLA_HOST_H_INCLUDED
/* --------------------------------------------------------------------------------------------------------------------
 * information */

/*!
 * Get the complete license text of used third-party code and features.
 * Returned string is in basic html format.
 */
CARLA_PLUGIN_EXPORT const char* carla_get_complete_license_text(void);

/*!
 * Get the list of supported file extensions in carla_load_file().
 */
CARLA_PLUGIN_EXPORT const char* const* carla_get_supported_file_extensions(void);

/*!
 * Get the list of supported features in the current Carla build.
 */
CARLA_PLUGIN_EXPORT const char* const* carla_get_supported_features(void);

/*!
 * Get the absolute filename of this carla library.
 */
CARLA_PLUGIN_EXPORT const char* carla_get_library_filename(void);

/*!
 * Get the folder where this carla library resides.
 */
CARLA_PLUGIN_EXPORT const char* carla_get_library_folder(void);
#endif

/* --------------------------------------------------------------------------------------------------------------------
 * pipes */

typedef void* CarlaPipeClientHandle;

/*!
 * Callback for when a message has been received (in the context of carla_pipe_client_idle()).
 * If extra data is required, use any of the carla_pipe_client_readlineblock* functions.
 */
typedef void (*CarlaPipeCallbackFunc)(void* ptr, const char* msg);

/*!
 * Create and connect to pipes as used by a server.
 * @a argv must match the arguments set the by server.
 */
CARLA_PLUGIN_EXPORT CarlaPipeClientHandle carla_pipe_client_new(const char* argv[],
                                                                CarlaPipeCallbackFunc callbackFunc,
                                                                void* callbackPtr);

/*!
 * Check the pipe for new messages and send them to the callback function.
 */
CARLA_PLUGIN_EXPORT void carla_pipe_client_idle(CarlaPipeClientHandle handle);

/*!
 * Check if the pipe is running.
 */
CARLA_PLUGIN_EXPORT bool carla_pipe_client_is_running(CarlaPipeClientHandle handle);

/*!
 * Lock the pipe write mutex.
 */
CARLA_PLUGIN_EXPORT void carla_pipe_client_lock(CarlaPipeClientHandle handle);

/*!
 * Unlock the pipe write mutex.
 */
CARLA_PLUGIN_EXPORT void carla_pipe_client_unlock(CarlaPipeClientHandle handle);

/*!
 * Read the next line as a string.
 */
CARLA_PLUGIN_EXPORT const char* carla_pipe_client_readlineblock(CarlaPipeClientHandle handle, uint timeout);

/*!
 * Read the next line as a boolean.
 */
CARLA_PLUGIN_EXPORT bool carla_pipe_client_readlineblock_bool(CarlaPipeClientHandle handle, uint timeout);

/*!
 * Read the next line as an integer.
 */
CARLA_PLUGIN_EXPORT int carla_pipe_client_readlineblock_int(CarlaPipeClientHandle handle, uint timeout);

/*!
 * Read the next line as a floating point number (double precision).
 */
CARLA_PLUGIN_EXPORT double carla_pipe_client_readlineblock_float(CarlaPipeClientHandle handle, uint timeout);

/*!
 * Write a valid message.
 * A valid message has only one '\n' character and it's at the end.
 */
CARLA_PLUGIN_EXPORT bool carla_pipe_client_write_msg(CarlaPipeClientHandle handle, const char* msg);

/*!
 * Write and fix a message.
 */
CARLA_PLUGIN_EXPORT bool carla_pipe_client_write_and_fix_msg(CarlaPipeClientHandle handle, const char* msg);

/*!
 * Sync all messages currently in cache.
 * This call will forcely write any messages in cache to any relevant IO.
 */
CARLA_PLUGIN_EXPORT bool carla_pipe_client_sync(CarlaPipeClientHandle handle);

/*!
 * Convenience call for doing both sync and unlock in one-go.
 */
CARLA_PLUGIN_EXPORT bool carla_pipe_client_sync_and_unlock(CarlaPipeClientHandle handle);

/*!
 * Destroy a previously created pipes instance.
 */
CARLA_PLUGIN_EXPORT void carla_pipe_client_destroy(CarlaPipeClientHandle handle);

/* DEPRECATED use carla_pipe_client_sync */
CARLA_PLUGIN_EXPORT bool carla_pipe_client_flush(CarlaPipeClientHandle handle);

/* DEPRECATED use carla_pipe_client_sync_and_unlock */
CARLA_PLUGIN_EXPORT bool carla_pipe_client_flush_and_unlock(CarlaPipeClientHandle handle);

/* --------------------------------------------------------------------------------------------------------------------
 * system stuff */

/*!
 * Flush stdout or stderr.
 */
CARLA_PLUGIN_EXPORT void carla_fflush(bool err);

/*!
 * Print the string @a string to stdout or stderr.
 */
CARLA_PLUGIN_EXPORT void carla_fputs(bool err, const char* string);

/*!
 * Set the current process name to @a name.
 */
CARLA_PLUGIN_EXPORT void carla_set_process_name(const char* name);

/* --------------------------------------------------------------------------------------------------------------------
 * window control */

/*!
 * Get the global/desktop scale factor.
 */
CARLA_PLUGIN_EXPORT double carla_get_desktop_scale_factor(void);

CARLA_PLUGIN_EXPORT int carla_cocoa_get_window(void* nsViewPtr);
CARLA_PLUGIN_EXPORT void carla_cocoa_set_transient_window_for(void* nsViewChild, void* nsViewParent);

CARLA_PLUGIN_EXPORT void carla_x11_reparent_window(uintptr_t winId1, uintptr_t winId2);

CARLA_PLUGIN_EXPORT void carla_x11_move_window(uintptr_t winId, int x, int y);

CARLA_PLUGIN_EXPORT int* carla_x11_get_window_pos(uintptr_t winId);

/* ----------------------------------------------------------------------------------------------------------------- */

/** @} */

#endif /* CARLA_UTILS_H_INCLUDED */
