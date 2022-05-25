/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_SCOPED_SAFE_LOCALE_HPP_INCLUDED
#define DISTRHO_SCOPED_SAFE_LOCALE_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

#include <clocale>

#if ! (defined(DISTRHO_OS_HAIKU) || defined(DISTRHO_OS_MAC) || defined(DISTRHO_OS_WINDOWS))
# define DISTRHO_USE_NEWLOCALE
#endif

#if defined(DISTRHO_OS_WINDOWS) && __MINGW64_VERSION_MAJOR >= 5
# define DISTRHO_USE_CONFIGTHREADLOCALE
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// ScopedSafeLocale class definition

/**
   ScopedSafeLocale is a handy class for setting current locale to C on constructor, and revert back on destructor.
   It tries to be thread-safe, but it is not always possible.

   Put it inside a scope of code where string conversions happen to ensure they are consistent across many systems.
   For example:

   ```
      // stack buffer to put converted float value in
      char strbuf[0xff];

      {
          // safe locale operations during this scope
          const ScopedSafeLocale sl;
          snprintf(strbuf, 0xff, "%f", value);
      }

      // do something with `strbuf` now, locale is reverted and left just as it was before
   ```
 */
class ScopedSafeLocale {
public:
    /*
     * Constructor.
     * Current system locale will saved, while "C" is set as the next one to use.
     */
    inline ScopedSafeLocale() noexcept;

    /*
     * Destructor.
     * System locale will revert back to the one saved during constructor.
     */
    inline ~ScopedSafeLocale() noexcept;

private:
#ifdef DISTRHO_USE_NEWLOCALE
    locale_t newloc, oldloc;
#else
# ifdef DISTRHO_USE_CONFIGTHREADLOCALE
    const int oldthreadloc;
# endif
    char* const oldloc;
#endif

    DISTRHO_DECLARE_NON_COPYABLE(ScopedSafeLocale)
    DISTRHO_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------
// ScopedSafeLocale class implementation

#ifdef DISTRHO_USE_NEWLOCALE
static constexpr const locale_t kNullLocale = (locale_t)nullptr;
#endif

inline ScopedSafeLocale::ScopedSafeLocale() noexcept
#ifdef DISTRHO_USE_NEWLOCALE
    : newloc(::newlocale(LC_NUMERIC_MASK, "C", kNullLocale)),
      oldloc(newloc != kNullLocale ? ::uselocale(newloc) : kNullLocale) {}
#else
# ifdef DISTRHO_USE_CONFIGTHREADLOCALE
    : oldthreadloc(_configthreadlocale(_ENABLE_PER_THREAD_LOCALE)),
# else
    :
# endif
      oldloc(strdup(::setlocale(LC_NUMERIC, nullptr)))
{
    ::setlocale(LC_NUMERIC, "C");
}
#endif

inline ScopedSafeLocale::~ScopedSafeLocale() noexcept
{
#ifdef DISTRHO_USE_NEWLOCALE
    if (oldloc != kNullLocale)
        ::uselocale(oldloc);
    if (newloc != kNullLocale)
        ::freelocale(newloc);
#else // DISTRHO_USE_NEWLOCALE
    if (oldloc != nullptr)
    {
        ::setlocale(LC_NUMERIC, oldloc);
        std::free(oldloc);
    }

# ifdef DISTRHO_USE_CONFIGTHREADLOCALE
    if (oldthreadloc != -1)
        _configthreadlocale(oldthreadloc);
# endif
#endif // DISTRHO_USE_NEWLOCALE
}

// -----------------------------------------------------------------------

#undef DISTRHO_USE_CONFIGTHREADLOCALE
#undef DISTRHO_USE_NEWLOCALE

END_NAMESPACE_DISTRHO

#endif // DISTRHO_SCOPED_SAFE_LOCALE_HPP_INCLUDED
