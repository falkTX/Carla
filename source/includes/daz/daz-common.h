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

#ifndef DAZ_COMMON_H_INCLUDED
#define DAZ_COMMON_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

/*!
 * @defgroup DAZPluginAPI DAZ Plugin API
 *
 * The DAZ Plugin API
 *
 * TODO: More complete description here.
 * @{
 */

/*!
 * Current API version.
 *
 * Hosts may load plugins that use old versions, but not newer.
 */
#define DAZ_API_VERSION 1

/*!
 * Symbol export.
 *
 * This makes sure the plugin and UI entry points are always visible,
 * regardless of compile settings.
 */
#ifdef _WIN32
# define DAZ_SYMBOL_EXPORT __declspec(dllexport)
#else
# define DAZ_SYMBOL_EXPORT __attribute__((visibility("default")))
#endif

/*!
 * Terminator character for property lists.
 */
#define DAZ_TERMINATOR ":"

/*!
 * Host mapped value of a string.
 * The value 0 is reserved as undefined.
 * @see PluginHostDescriptor::map_value(), UiHostDescriptor::map_value()
 */
typedef uint32_t mapped_value_t;

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DAZ_COMMON_H_INCLUDED */
