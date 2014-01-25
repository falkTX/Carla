/*
 * DAZ - Digital Audio with Zero dependencies
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2013 Harry van Haaren <harryhaaren@gmail.com>
 * Copyright (C) 2013 Jonathan Moore Liles <male@tuxfamily.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DAZ_UI_H_INCLUDED
#define DAZ_UI_H_INCLUDED

#include "daz-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @defgroup DAZPluginAPI
 * @{
 */

/*!
 * @defgroup UiFeatures UI Features
 *
 * A list of UI features or hints.
 *
 * Custom features are allowed, as long as they are lowercase and contain ASCII characters only.
 * The host can decide if it can load the UI or not based on this information.
 *
 * Multiple features can be set by using ":" in between them.
 * @{
 */

/*!
 * Supports sample rate changes on-the-fly.
 *
 * If unset, the host will re-initiate the UI when the sample rate changes.
 */
#define UI_FEATURE_SAMPLE_RATE_CHANGES ":sample_rate_changes"

/*!
 * Uses open_file() and/or save_file() functions.
 */
#define UI_FEATURE_OPEN_SAVE ":open_save"

/*!
 * Uses send_plugin_msg() function.
 */
#define UI_FEATURE_SEND_MSG ":send_msg"

/** @} */

/*!
 * @defgroup HostDispatcherOpcodes Host Dispatcher Opcodes
 *
 * Opcodes dispatched by the UI to report and request information from the host.
 *
 * The opcodes are mapped into MappedValue by the host.
 * @see HostDescriptor::dispatcher()
 * @{
 */

/*!
 * Tell the host the UI can't be shown, uses nothing.
 */
#define HOST_OPCODE_UI_UNAVAILABLE "ui_unavailable"

/** @} */

/*!
 * @defgroup UiDispatcherOpcodes UI Dispatcher Opcodes
 *
 * Opcodes dispatched by the host to report changes to the UI.
 *
 * The opcodes are mapped into MappedValue by the host.
 * @see UiDescriptor::dispatcher()
 * @{
 */

/*!
 * Message received, uses value as size, ptr for contents.
 */
#define UI_OPCODE_MSG_RECEIVED "msg_received"

/*!
 * Audio sample rate changed, uses opt, returns 1 if supported.
 *
 * @see UiHostDescriptor::sample_rate
 */
#define UI_OPCODE_SAMPLE_RATE_CHANGED "sample_rate_changed"

/*!
 * Offline mode changed, uses value (0=off, 1=on).
 *
 * @see UiHostDescriptor::is_offline
 */
#define UI_OPCODE_OFFLINE_CHANGED "offline_changed"

/*!
 * UI title changed, uses ptr.
 *
 * @see UiHostDescriptor::ui_title
 */
#define UI_OPCODE_TITLE_CHANGED "ui_title_thanged"

/** @} */

/*!
 * Opaque UI handle.
 */
typedef void* UiHandle;

/*!
 * Opaque UI host handle.
 */
typedef void* UiHostHandle;

/*!
 * UiHostDescriptor
 */
typedef struct {
    /*!
     * Opaque UI host handle.
     */
    UiHostHandle handle;

    /*!
     * Full filepath to the UI *.daz bundle.
     */
    const char* bundle_dir;

    /*!
     * Host desired UI title.
     */
    const char* ui_title;

    /*!
     * Current audio sample rate.
     */
    double sample_rate;

    /*!
     * Wherever the host is currently processing offline.
     */
    bool is_offline;

    /* probably better if only allowed during instantiate() */
    mapped_value_t (*map_value)(UiHostHandle handle, const char* valueStr);
    const char*    (*unmap_value)(UiHostHandle handle, mapped_value_t value);

    /*!
     * Inform the host about a parameter change.
     */
    void (*parameter_changed)(UiHostHandle handle, uint32_t index, float value);

    /*!
     * Inform the host about a/the MIDI program change.
     *
     * @note: Only synths make use the of @a channel argument.
     */
    void (*midi_program_changed)(UiHostHandle handle, uint8_t channel, uint32_t bank, uint32_t program);

    /*!
     * Inform the host the UI has been closed.
     *
     * After calling this the UI should not do any operation and simply wait
     * until the host calls UiDescriptor::cleanup().
     */
    void (*closed)(UiHostHandle handle);

    /* TODO: add some msgbox call */

    /* ui must set "opensave" feature to use these */
    const char* (*open_file)(UiHostHandle handle, bool isDir, const char* title, const char* filter);
    const char* (*save_file)(UiHostHandle handle, bool isDir, const char* title, const char* filter);

    /* ui must set "sendmsg" feature to use this */
    bool (*send_plugin_msg)(UiHostHandle handle, const void* data, size_t size);

    /* uses HostDispatcherOpcodes */
    intptr_t (*dispatcher)(UiHostHandle handle, mapped_value_t opcode, int32_t index, intptr_t value, void* ptr, float opt);

} UiHostDescriptor;

/*!
 * UiDescriptor
 */
typedef struct {
    const int api;              /*!< Must be set to DAZ_API_VERSION. */
    const char* const features; /*!< Features. @see UiFeatures */
    const char* const author;   /*!< Author this UI applies to. */
    const char* const label;    /*!< Label this UI applies to, can only contain letters, numbers and "_". May be null, in which case represents all UIs for @a maker. */

    UiHandle (*instantiate)(const UiHostDescriptor* host);
    void     (*cleanup)(UiHandle handle);

    void (*show)(UiHandle handle, bool show);
    void (*idle)(UiHandle handle);

    void (*set_parameter)(UiHandle handle, uint32_t index, float value);
    void (*set_midi_program)(UiHandle handle, uint8_t channel, uint32_t bank, uint32_t program);
    void (*set_state)(UiHandle handle, const char* state);

    /* uses UiDispatcherOpcodes */
    intptr_t (*dispatcher)(UiHandle handle, mapped_value_t opcode, int32_t index, intptr_t value, void* ptr, float opt);

} UiDescriptor;

/*!
 * UI entry point function used by the UI.
 */
DAZ_SYMBOL_EXPORT
const UiDescriptor* daz_get_ui_descriptor(uint32_t index);

/*!
 * UI entry point function used by the host.
 */
typedef const UiDescriptor* (*daz_get_ui_descriptor_fn)(uint32_t index);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DAZ_UI_H_INCLUDED */
