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

#ifndef WATER_STRINGREF_H_INCLUDED
#define WATER_STRINGREF_H_INCLUDED

#include "String.h"

namespace water {

//==============================================================================
/**
    A simple class for holding temporary references to a string literal or String.

    Unlike a real String object, the StringRef does not allocate any memory or
    take ownership of the strings you give to it - it simply holds a reference to
    a string that has been allocated elsewhere.
    The main purpose of the class is to be used instead of a const String& as the type
    of function arguments where the caller may pass either a string literal or a String
    object. This means that when the called uses a string literal, there's no need
    for an temporary String object to be allocated, and this cuts down overheads
    substantially.

    Because the class is simply a wrapper around a pointer, you should always pass
    it by value, not by reference.

    @code
    void myStringFunction1 (const String&);
    void myStringFunction2 (StringRef);

    myStringFunction1 ("abc"); // Implicitly allocates a temporary String object.
    myStringFunction2 ("abc"); // Much faster, as no local allocations are needed.
    @endcode

    For examples of it in use, see the XmlElement or StringArray classes.

    Bear in mind that there are still many cases where it's better to use an argument
    which is a const String&. For example if the function stores the string or needs
    to internally create a String from the argument, then it's better for the original
    argument to already be a String.

    @see String
*/
class StringRef
{
public:
    /** Creates a StringRef from a raw string literal.
        The StringRef object does NOT take ownership or copy this data, so you must
        ensure that the data does not change during the lifetime of the StringRef.
        Note that this pointer not be null!
    */
    StringRef (const char* stringLiteral) noexcept;

    /** Creates a StringRef from a raw char pointer.
        The StringRef object does NOT take ownership or copy this data, so you must
        ensure that the data does not change during the lifetime of the StringRef.
    */
    StringRef (CharPointer_UTF8 stringLiteral) noexcept;

    /** Creates a StringRef from a String.
        The StringRef object does NOT take ownership or copy the data from the String,
        so you must ensure that the String is not modified or deleted during the lifetime
        of the StringRef.
    */
    StringRef (const String& string) noexcept;

    /** Creates a StringRef pointer to an empty string. */
    StringRef() noexcept;

    //==============================================================================
    /** Returns a raw pointer to the underlying string data. */
    operator const char*() const noexcept      { return text.getAddress(); }
    /** Returns a pointer to the underlying string data as a char pointer object. */
    operator CharPointer_UTF8() const noexcept { return text; }

    /** Returns true if the string is empty. */
    bool isEmpty() const noexcept                                       { return text.isEmpty(); }
    /** Returns true if the string is not empty. */
    bool isNotEmpty() const noexcept                                    { return ! text.isEmpty(); }
    /** Returns the number of characters in the string. */
    int length() const noexcept                                         { return (int) text.length(); }

    /** Retrieves a character by index. */
    char operator[] (int index) const noexcept                          { return text[index]; }

    /** Compares this StringRef with a String. */
    bool operator== (const String& s) const noexcept                    { return text.compare (s.getCharPointer()) == 0; }
    /** Compares this StringRef with a String. */
    bool operator!= (const String& s) const noexcept                    { return text.compare (s.getCharPointer()) != 0; }

    /** Case-sensitive comparison of two StringRefs. */
    bool operator== (StringRef s) const noexcept                        { return text.compare (s.text) == 0; }
    /** Case-sensitive comparison of two StringRefs. */
    bool operator!= (StringRef s) const noexcept                        { return text.compare (s.text) != 0; }

    //==============================================================================
    /** The text that is referenced. */
    CharPointer_UTF8 text;
};

//==============================================================================
/** Case-sensitive comparison of two strings. */
bool operator== (const String& string1, StringRef string2) noexcept;
/** Case-sensitive comparison of two strings. */
bool operator!= (const String& string1, StringRef string2) noexcept;

inline String operator+ (String s1, StringRef s2)           { return s1 += String (s2.text); }
inline String operator+ (StringRef s1, const String& s2)    { return String (s1.text) + s2; }
inline String operator+ (const char* s1, StringRef s2)      { return String (s1) + String (s2.text); }
inline String operator+ (StringRef s1, const char* s2)      { return String (s1.text) + String (s2); }

}

#endif // WATER_STRINGREF_H_INCLUDED
