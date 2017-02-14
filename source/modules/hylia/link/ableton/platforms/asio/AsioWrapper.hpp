/* Copyright 2016, Ableton AG, Berlin. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  If you would like to incorporate Link into a proprietary software application,
 *  please contact <link-devs@ableton.com>.
 */

#pragma once

/*!
 * \brief Wrapper file for AsioStandalone library
 *
 * This file includes all necessary headers from the AsioStandalone library which are used
 * by Link.
 */

#pragma push_macro("ASIO_STANDALONE")
#define ASIO_STANDALONE 1

#pragma push_macro("ASIO_NO_TYPEID")
#define ASIO_NO_TYPEID 1

#if LINK_PLATFORM_WINDOWS
#pragma push_macro("INCL_EXTRA_HTON_FUNCTIONS")
#define INCL_EXTRA_HTON_FUNCTIONS 1
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#if __has_warning("-Wcomma")
#pragma clang diagnostic ignored "-Wcomma"
#endif
#endif

#if defined(_MSC_VER)
#pragma warning(push)
// C4191: 'operator/operation': unsafe conversion from 'type of expression' to
// 'type required'
#pragma warning(disable : 4191)
// C4548: expression before comma has no effect; expected expression with side-effect
#pragma warning(disable : 4548)
// C4619: #pragma warning : there is no warning number 'number'
#pragma warning(disable : 4619)
// C4675: 'function' : resolved overload was found by argument-dependent lookup
#pragma warning(disable : 4675)
#endif

#include <asio.hpp>
#include <asio/system_timer.hpp>

#if LINK_PLATFORM_WINDOWS
#pragma pop_macro("INCL_EXTRA_HTON_FUNCTIONS")
#endif

#pragma pop_macro("ASIO_STANDALONE")
#pragma pop_macro("ASIO_NO_TYPEID")

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
