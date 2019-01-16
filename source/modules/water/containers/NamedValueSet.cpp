/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>

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

#include "NamedValueSet.h"

namespace water {

//==============================================================================
NamedValueSet::NamedValueSet() noexcept
{
}

NamedValueSet::NamedValueSet (const NamedValueSet& other)
   : values (other.values)
{
}

NamedValueSet& NamedValueSet::operator= (const NamedValueSet& other)
{
    clear();
    values = other.values;
    return *this;
}

#if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
NamedValueSet::NamedValueSet (NamedValueSet&& other) noexcept
    : values (static_cast<Array<NamedValue>&&> (other.values))
{
}

NamedValueSet& NamedValueSet::operator= (NamedValueSet&& other) noexcept
{
    other.values.swapWith (values);
    return *this;
}
#endif

NamedValueSet::~NamedValueSet() noexcept
{
}

void NamedValueSet::clear()
{
    values.clear();
}

bool NamedValueSet::operator== (const NamedValueSet& other) const
{
    return values == other.values;
}

bool NamedValueSet::operator!= (const NamedValueSet& other) const
{
    return ! operator== (other);
}

int NamedValueSet::size() const noexcept
{
    return values.size();
}

bool NamedValueSet::isEmpty() const noexcept
{
    return values.isEmpty();
}

static const var& getNullVarRef() noexcept
{
    static var nullVar;
    return nullVar;
}

const var& NamedValueSet::operator[] (const Identifier& name) const noexcept
{
    if (const var* v = getVarPointer (name))
        return *v;

    return getNullVarRef();
}

var NamedValueSet::getWithDefault (const Identifier& name, const var& defaultReturnValue) const
{
    if (const var* const v = getVarPointer (name))
        return *v;

    return defaultReturnValue;
}

var* NamedValueSet::getVarPointer (const Identifier& name) const noexcept
{
    for (NamedValue* e = values.end(), *i = values.begin(); i != e; ++i)
        if (i->name == name)
            return &(i->value);

    return nullptr;
}

#if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
bool NamedValueSet::set (const Identifier& name, var&& newValue)
{
    if (var* const v = getVarPointer (name))
    {
        if (v->equalsWithSameType (newValue))
            return false;

        *v = static_cast<var&&> (newValue);
        return true;
    }

    values.add (NamedValue (name, static_cast<var&&> (newValue)));
    return true;
}
#endif

bool NamedValueSet::set (const Identifier& name, const var& newValue)
{
    if (var* const v = getVarPointer (name))
    {
        if (v->equalsWithSameType (newValue))
            return false;

        *v = newValue;
        return true;
    }

    values.add (NamedValue (name, newValue));
    return true;
}

bool NamedValueSet::contains (const Identifier& name) const noexcept
{
    return getVarPointer (name) != nullptr;
}

int NamedValueSet::indexOf (const Identifier& name) const noexcept
{
    const int numValues = values.size();

    for (int i = 0; i < numValues; ++i)
        if (values.getReference(i).name == name)
            return i;

    return -1;
}

bool NamedValueSet::remove (const Identifier& name)
{
    const int numValues = values.size();

    for (int i = 0; i < numValues; ++i)
    {
        if (values.getReference(i).name == name)
        {
            values.remove (i);
            return true;
        }
    }

    return false;
}

Identifier NamedValueSet::getName (const int index) const noexcept
{
    if (isPositiveAndBelow (index, values.size()))
        return values.getReference (index).name;

    wassertfalse;
    return Identifier();
}

const var& NamedValueSet::getValueAt (const int index) const noexcept
{
    if (isPositiveAndBelow (index, values.size()))
        return values.getReference (index).value;

    wassertfalse;
    return getNullVarRef();
}

var* NamedValueSet::getVarPointerAt (int index) const noexcept
{
    if (isPositiveAndBelow (index, values.size()))
        return &(values.getReference (index).value);

    return nullptr;
}

}
