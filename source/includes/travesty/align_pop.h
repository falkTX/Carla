/*
 * travesty, pure C VST3-compatible interface
 * Copyright (C) 2021 Filipe Coelho <falktx@falktx.com>
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

#if defined(__APPLE__)
# if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wpragma-pack"
# elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 460
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunknown-warning-option"
#  pragma GCC diagnostic ignored "-Wpragma-pack"
# endif
#endif

#if defined(__APPLE__) || defined(_WIN32)
# pragma pack(pop)
#endif

#if defined(__APPLE__)
# if defined(__clang__)
#  pragma clang diagnostic pop
# elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 460
#  pragma GCC diagnostic pop
# endif
#endif
