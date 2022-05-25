/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2022 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#ifndef WATER_IDENTIFIER_H_INCLUDED
#define WATER_IDENTIFIER_H_INCLUDED

#include "String.h"

namespace water {

//==============================================================================
/**
    Represents a string identifier, designed for accessing properties by name.

    Comparing two Identifier objects is very fast (an O(1) operation), but creating
    them can be slower than just using a String directly, so the optimal way to use them
    is to keep some static Identifier objects for the things you use often.

    @see NamedValueSet, ValueTree
*/
class Identifier
{
public:
    /** Creates a null identifier. */
    Identifier() noexcept;

    /** Creates an identifier with a specified name.
        Because this name may need to be used in contexts such as script variables or XML
        tags, it must only contain ascii letters and digits, or the underscore character.
    */
    Identifier (const char* name);

    /** Creates an identifier with a specified name.
        Because this name may need to be used in contexts such as script variables or XML
        tags, it must only contain ascii letters and digits, or the underscore character.
    */
    Identifier (const std::string& name);

    /** Creates an identifier with a specified name.
        Because this name may need to be used in contexts such as script variables or XML
        tags, it must only contain ascii letters and digits, or the underscore character.
    */
    Identifier (const char* nameStart, const char* nameEnd);

    /** Creates a copy of another identifier. */
    Identifier (const Identifier& other) noexcept;

    /** Creates a copy of another identifier. */
    Identifier& operator= (const Identifier& other) noexcept;

    /** Destructor */
    ~Identifier() noexcept;

    /** Compares two identifiers. This is a very fast operation. */
    inline bool operator== (const Identifier& other) const noexcept     { return name == other.name; }

    /** Compares two identifiers. This is a very fast operation. */
    inline bool operator!= (const Identifier& other) const noexcept     { return name != other.name; }

    /** Compares the identifier with a string. */
    inline bool operator== (const char* other) const noexcept           { return name == other; }

    /** Compares the identifier with a string. */
    inline bool operator!= (const char* other) const noexcept           { return name != other; }

    /** Compares the identifier with a string. */
    inline bool operator<  (const char* other) const noexcept           { return name <  other; }

    /** Compares the identifier with a string. */
    inline bool operator<= (const char* other) const noexcept           { return name <= other; }

    /** Compares the identifier with a string. */
    inline bool operator>  (const char* other) const noexcept           { return name >  other; }

    /** Compares the identifier with a string. */
    inline bool operator>= (const char* other) const noexcept           { return name >= other; }

    /** Returns this identifier as a string. */
    const std::string& toString() const noexcept                        { return name; }

    /** Returns this identifier as a StringRef. */
    operator StringRef() const noexcept                                 { return StringRef(name.c_str()); }

    /** Returns true if this Identifier is not null */
    bool isValid() const noexcept                                       { return !name.empty(); }

    /** Returns true if this Identifier is null */
    bool isNull() const noexcept                                        { return name.empty(); }

private:
    std::string name;
};

}

#endif // WATER_IDENTIFIER_H_INCLUDED
