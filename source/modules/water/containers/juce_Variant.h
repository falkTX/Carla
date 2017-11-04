/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2016 - ROLI Ltd.

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

   -----------------------------------------------------------------------------

   To release a closed-source product which uses other parts of JUCE not
   licensed under the ISC terms, commercial licenses are available: visit
   www.juce.com for more information.

  ==============================================================================
*/

#ifndef JUCE_VARIANT_H_INCLUDED
#define JUCE_VARIANT_H_INCLUDED

namespace water {

//==============================================================================
/**
    A variant class, that can be used to hold a range of primitive values.

    A var object can hold a range of simple primitive values, strings, or
    any kind of ReferenceCountedObject. The var class is intended to act like
    the kind of values used in dynamic scripting languages.

    You can save/load var objects either in a small, proprietary binary format
    using writeToStream()/readFromStream(), or as JSON by using the JSON class.

    @see JSON, DynamicObject
*/
class var
{
public:
    //==============================================================================
    /** Creates a void variant. */
    var() noexcept;

    /** Destructor. */
    ~var() noexcept;

    var (const var& valueToCopy);
    var (int value) noexcept;
    var (int64 value) noexcept;
    var (bool value) noexcept;
    var (double value) noexcept;
    var (const char* value);
    var (const String& value);

    var& operator= (const var& valueToCopy);
    var& operator= (int value);
    var& operator= (int64 value);
    var& operator= (bool value);
    var& operator= (double value);
    var& operator= (const char* value);
    var& operator= (const String& value);

   #if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
    var (var&&) noexcept;
    var (String&&);
    var& operator= (var&&) noexcept;
    var& operator= (String&&);
   #endif

    void swapWith (var& other) noexcept;

    /** Returns a var object that can be used where you need the javascript "undefined" value. */
    static var undefined() noexcept;

    //==============================================================================
    operator int() const noexcept;
    operator int64() const noexcept;
    operator bool() const noexcept;
    operator float() const noexcept;
    operator double() const noexcept;
    operator String() const;
    String toString() const;

    //==============================================================================
    bool isVoid() const noexcept;
    bool isUndefined() const noexcept;
    bool isInt() const noexcept;
    bool isInt64() const noexcept;
    bool isBool() const noexcept;
    bool isDouble() const noexcept;
    bool isString() const noexcept;

    /** Returns true if this var has the same value as the one supplied.
        Note that this ignores the type, so a string var "123" and an integer var with the
        value 123 are considered to be equal.
        @see equalsWithSameType
    */
    bool equals (const var& other) const noexcept;

    /** Returns true if this var has the same value and type as the one supplied.
        This differs from equals() because e.g. "123" and 123 will be considered different.
        @see equals
    */
    bool equalsWithSameType (const var& other) const noexcept;

    /** Returns true if this var has the same type as the one supplied. */
    bool hasSameTypeAs (const var& other) const noexcept;

    /** Returns a deep copy of this object.
        For simple types this just returns a copy, but if the object contains any arrays
        or DynamicObjects, they will be cloned (recursively).
    */
    var clone() const noexcept;

private:
    //==============================================================================
    class VariantType;            friend class VariantType;
    class VariantType_Void;       friend class VariantType_Void;
    class VariantType_Undefined;  friend class VariantType_Undefined;
    class VariantType_Int;        friend class VariantType_Int;
    class VariantType_Int64;      friend class VariantType_Int64;
    class VariantType_Double;     friend class VariantType_Double;
    class VariantType_Bool;       friend class VariantType_Bool;
    class VariantType_String;     friend class VariantType_String;

    union ValueUnion
    {
        int intValue;
        int64 int64Value;
        bool boolValue;
        double doubleValue;
        char stringValue [sizeof (String)];
    };

    const VariantType* type;
    ValueUnion value;

    Array<var>* convertToArray();
    var (const VariantType&) noexcept;
};

/** Compares the values of two var objects, using the var::equals() comparison. */
bool operator== (const var&, const var&) noexcept;
/** Compares the values of two var objects, using the var::equals() comparison. */
bool operator!= (const var&, const var&) noexcept;
bool operator== (const var&, const String&);
bool operator!= (const var&, const String&);
bool operator== (const var&, const char*);
bool operator!= (const var&, const char*);

//==============================================================================
/** This template-overloaded class can be used to convert between var and custom types. */
template <typename Type>
struct VariantConverter
{
    static Type fromVar (const var& v)             { return static_cast<Type> (v); }
    static var toVar (const Type& t)               { return t; }
};

/** This template-overloaded class can be used to convert between var and custom types. */
template <>
struct VariantConverter<String>
{
    static String fromVar (const var& v)           { return v.toString(); }
    static var toVar (const String& s)             { return s; }
};

}

#endif   // JUCE_VARIANT_H_INCLUDED
