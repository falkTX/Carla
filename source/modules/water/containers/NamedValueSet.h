/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2018 Filipe Coelho <falktx@falktx.com>

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

#ifndef WATER_NAMEDVALUESET_H_INCLUDED
#define WATER_NAMEDVALUESET_H_INCLUDED

#include "Array.h"
#include "Variant.h"
#include "../text/Identifier.h"

namespace water {

//==============================================================================
/** Holds a set of named var objects.

    This can be used as a basic structure to hold a set of var object, which can
    be retrieved by using their identifier.
*/
class NamedValueSet
{
public:
    /** Creates an empty set. */
    NamedValueSet() noexcept;

    /** Creates a copy of another set. */
    NamedValueSet (const NamedValueSet&);

    /** Replaces this set with a copy of another set. */
    NamedValueSet& operator= (const NamedValueSet&);

    /** Destructor. */
    ~NamedValueSet() noexcept;

    bool operator== (const NamedValueSet&) const;
    bool operator!= (const NamedValueSet&) const;

    //==============================================================================
    struct NamedValue
    {
        NamedValue() noexcept : name(), value() {}
        NamedValue (const Identifier& n, const var& v) : name (n), value (v) {}
        NamedValue (const NamedValue& other) : name (other.name), value (other.value) {}

        bool operator== (const NamedValue& other) const noexcept   { return name == other.name && value == other.value; }
        bool operator!= (const NamedValue& other) const noexcept   { return ! operator== (other); }

        Identifier name;
        var value;
    };

    NamedValueSet::NamedValue* begin() { return values.begin(); }
    NamedValueSet::NamedValue* end()   { return values.end();   }

    //==============================================================================

    /** Returns the total number of values that the set contains. */
    int size() const noexcept;

    /** Returns true if the set is empty. */
    bool isEmpty() const noexcept;

    /** Returns the value of a named item.
        If the name isn't found, this will return a void variant.
        @see getProperty
    */
    const var& operator[] (const Identifier& name) const noexcept;

    /** Tries to return the named value, but if no such value is found, this will
        instead return the supplied default value.
    */
    var getWithDefault (const Identifier& name, const var& defaultReturnValue) const;

    /** Changes or adds a named value.
        @returns    true if a value was changed or added; false if the
                    value was already set the value passed-in.
    */
    bool set (const Identifier& name, const var& newValue);

    /** Returns true if the set contains an item with the specified name. */
    bool contains (const Identifier& name) const noexcept;

    /** Removes a value from the set.
        @returns    true if a value was removed; false if there was no value
                    with the name that was given.
    */
    bool remove (const Identifier& name);

    /** Returns the name of the value at a given index.
        The index must be between 0 and size() - 1.
    */
    Identifier getName (int index) const noexcept;

    /** Returns a pointer to the var that holds a named value, or null if there is
        no value with this name.

        Do not use this method unless you really need access to the internal var object
        for some reason - for normal reading and writing always prefer operator[]() and set().
    */
    var* getVarPointer (const Identifier& name) const noexcept;

    /** Returns the value of the item at a given index.
        The index must be between 0 and size() - 1.
    */
    const var& getValueAt (int index) const noexcept;

    /** Returns the value of the item at a given index.
        The index must be between 0 and size() - 1, or this will return a nullptr
    */
    var* getVarPointerAt (int index) const noexcept;

    /** Returns the index of the given name, or -1 if it's not found. */
    int indexOf (const Identifier& name) const noexcept;

    /** Removes all values. */
    void clear();

private:
    //==============================================================================
    Array<NamedValue> values;
};

}

#endif // WATER_NAMEDVALUESET_H_INCLUDED
