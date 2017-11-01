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

enum VariantStreamMarkers
{
    varMarker_Int       = 1,
    varMarker_BoolTrue  = 2,
    varMarker_BoolFalse = 3,
    varMarker_Double    = 4,
    varMarker_String    = 5,
    varMarker_Int64     = 6,
    varMarker_Undefined = 9
};

//==============================================================================
class var::VariantType
{
public:
    VariantType() noexcept {}
    virtual ~VariantType() noexcept {}

    virtual int toInt (const ValueUnion&) const noexcept                        { return 0; }
    virtual int64 toInt64 (const ValueUnion&) const noexcept                    { return 0; }
    virtual double toDouble (const ValueUnion&) const noexcept                  { return 0; }
    virtual String toString (const ValueUnion&) const                           { return String(); }
    virtual bool toBool (const ValueUnion&) const noexcept                      { return false; }
    virtual var clone (const var& original) const                               { return original; }

    virtual bool isVoid() const noexcept      { return false; }
    virtual bool isUndefined() const noexcept { return false; }
    virtual bool isInt() const noexcept       { return false; }
    virtual bool isInt64() const noexcept     { return false; }
    virtual bool isBool() const noexcept      { return false; }
    virtual bool isDouble() const noexcept    { return false; }
    virtual bool isString() const noexcept    { return false; }

    virtual void cleanUp (ValueUnion&) const noexcept {}
    virtual void createCopy (ValueUnion& dest, const ValueUnion& source) const      { dest = source; }
    virtual bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept = 0;
};

//==============================================================================
class var::VariantType_Void  : public var::VariantType
{
public:
    VariantType_Void() noexcept {}
    static const VariantType_Void instance;

    bool isVoid() const noexcept override   { return true; }
    bool equals (const ValueUnion&, const ValueUnion&, const VariantType& otherType) const noexcept override { return otherType.isVoid() || otherType.isUndefined(); }
};

//==============================================================================
class var::VariantType_Undefined  : public var::VariantType
{
public:
    VariantType_Undefined() noexcept {}
    static const VariantType_Undefined instance;

    bool isUndefined() const noexcept override           { return true; }
    String toString (const ValueUnion&) const override   { return "undefined"; }
    bool equals (const ValueUnion&, const ValueUnion&, const VariantType& otherType) const noexcept override { return otherType.isVoid() || otherType.isUndefined(); }
};

//==============================================================================
class var::VariantType_Int  : public var::VariantType
{
public:
    VariantType_Int() noexcept {}
    static const VariantType_Int instance;

    int toInt (const ValueUnion& data) const noexcept override       { return data.intValue; };
    int64 toInt64 (const ValueUnion& data) const noexcept override   { return (int64) data.intValue; };
    double toDouble (const ValueUnion& data) const noexcept override { return (double) data.intValue; }
    String toString (const ValueUnion& data) const override          { return String (data.intValue); }
    bool toBool (const ValueUnion& data) const noexcept override     { return data.intValue != 0; }
    bool isInt() const noexcept override                             { return true; }

    bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept override
    {
        if (otherType.isDouble() || otherType.isInt64() || otherType.isString())
            return otherType.equals (otherData, data, *this);

        return otherType.toInt (otherData) == data.intValue;
    }
};

//==============================================================================
class var::VariantType_Int64  : public var::VariantType
{
public:
    VariantType_Int64() noexcept {}
    static const VariantType_Int64 instance;

    int toInt (const ValueUnion& data) const noexcept override       { return (int) data.int64Value; };
    int64 toInt64 (const ValueUnion& data) const noexcept override   { return data.int64Value; };
    double toDouble (const ValueUnion& data) const noexcept override { return (double) data.int64Value; }
    String toString (const ValueUnion& data) const override          { return String (data.int64Value); }
    bool toBool (const ValueUnion& data) const noexcept override     { return data.int64Value != 0; }
    bool isInt64() const noexcept override                           { return true; }

    bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept override
    {
        if (otherType.isDouble() || otherType.isString())
            return otherType.equals (otherData, data, *this);

        return otherType.toInt64 (otherData) == data.int64Value;
    }
};

//==============================================================================
class var::VariantType_Double   : public var::VariantType
{
public:
    VariantType_Double() noexcept {}
    static const VariantType_Double instance;

    int toInt (const ValueUnion& data) const noexcept override       { return (int) data.doubleValue; };
    int64 toInt64 (const ValueUnion& data) const noexcept override   { return (int64) data.doubleValue; };
    double toDouble (const ValueUnion& data) const noexcept override { return data.doubleValue; }
    String toString (const ValueUnion& data) const override          { return String (data.doubleValue, 20); }
    bool toBool (const ValueUnion& data) const noexcept override     { return data.doubleValue != 0; }
    bool isDouble() const noexcept override                          { return true; }

    bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept override
    {
        return std::abs (otherType.toDouble (otherData) - data.doubleValue) < std::numeric_limits<double>::epsilon();
    }
};

//==============================================================================
class var::VariantType_Bool   : public var::VariantType
{
public:
    VariantType_Bool() noexcept {}
    static const VariantType_Bool instance;

    int toInt (const ValueUnion& data) const noexcept override       { return data.boolValue ? 1 : 0; };
    int64 toInt64 (const ValueUnion& data) const noexcept override   { return data.boolValue ? 1 : 0; };
    double toDouble (const ValueUnion& data) const noexcept override { return data.boolValue ? 1.0 : 0.0; }
    String toString (const ValueUnion& data) const override          { return String::charToString (data.boolValue ? (juce_wchar) '1' : (juce_wchar) '0'); }
    bool toBool (const ValueUnion& data) const noexcept override     { return data.boolValue; }
    bool isBool() const noexcept override                            { return true; }

    bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept override
    {
        return otherType.toBool (otherData) == data.boolValue;
    }
};

//==============================================================================
class var::VariantType_String   : public var::VariantType
{
public:
    VariantType_String() noexcept {}
    static const VariantType_String instance;

    void cleanUp (ValueUnion& data) const noexcept override                       { getString (data)-> ~String(); }
    void createCopy (ValueUnion& dest, const ValueUnion& source) const override   { new (dest.stringValue) String (*getString (source)); }

    bool isString() const noexcept override                          { return true; }
    int toInt (const ValueUnion& data) const noexcept override       { return getString (data)->getIntValue(); };
    int64 toInt64 (const ValueUnion& data) const noexcept override   { return getString (data)->getLargeIntValue(); };
    double toDouble (const ValueUnion& data) const noexcept override { return getString (data)->getDoubleValue(); }
    String toString (const ValueUnion& data) const override          { return *getString (data); }
    bool toBool (const ValueUnion& data) const noexcept override     { return getString (data)->getIntValue() != 0
                                                                           || getString (data)->trim().equalsIgnoreCase ("true")
                                                                           || getString (data)->trim().equalsIgnoreCase ("yes"); }

    bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept override
    {
        return otherType.toString (otherData) == *getString (data);
    }

private:
    static inline const String* getString (const ValueUnion& data) noexcept { return reinterpret_cast<const String*> (data.stringValue); }
    static inline String* getString (ValueUnion& data) noexcept             { return reinterpret_cast<String*> (data.stringValue); }
};

//==============================================================================
const var::VariantType_Void         var::VariantType_Void::instance;
const var::VariantType_Undefined    var::VariantType_Undefined::instance;
const var::VariantType_Int          var::VariantType_Int::instance;
const var::VariantType_Int64        var::VariantType_Int64::instance;
const var::VariantType_Bool         var::VariantType_Bool::instance;
const var::VariantType_Double       var::VariantType_Double::instance;
const var::VariantType_String       var::VariantType_String::instance;


//==============================================================================
var::var() noexcept : type (&VariantType_Void::instance) {}
var::var (const VariantType& t) noexcept  : type (&t) {}
var::~var() noexcept  { type->cleanUp (value); }

//==============================================================================
var::var (const var& valueToCopy)  : type (valueToCopy.type)
{
    type->createCopy (value, valueToCopy.value);
}

var::var (const int v) noexcept       : type (&VariantType_Int::instance)    { value.intValue = v; }
var::var (const int64 v) noexcept     : type (&VariantType_Int64::instance)  { value.int64Value = v; }
var::var (const bool v) noexcept      : type (&VariantType_Bool::instance)   { value.boolValue = v; }
var::var (const double v) noexcept    : type (&VariantType_Double::instance) { value.doubleValue = v; }
var::var (const String& v)            : type (&VariantType_String::instance) { new (value.stringValue) String (v); }
var::var (const char* const v)        : type (&VariantType_String::instance) { new (value.stringValue) String (v); }

var var::undefined() noexcept           { return var (VariantType_Undefined::instance); }

//==============================================================================
bool var::isVoid() const noexcept       { return type->isVoid(); }
bool var::isUndefined() const noexcept  { return type->isUndefined(); }
bool var::isInt() const noexcept        { return type->isInt(); }
bool var::isInt64() const noexcept      { return type->isInt64(); }
bool var::isBool() const noexcept       { return type->isBool(); }
bool var::isDouble() const noexcept     { return type->isDouble(); }
bool var::isString() const noexcept     { return type->isString(); }

var::operator int() const noexcept                      { return type->toInt (value); }
var::operator int64() const noexcept                    { return type->toInt64 (value); }
var::operator bool() const noexcept                     { return type->toBool (value); }
var::operator float() const noexcept                    { return (float) type->toDouble (value); }
var::operator double() const noexcept                   { return type->toDouble (value); }
String var::toString() const                            { return type->toString (value); }
var::operator String() const                            { return type->toString (value); }

//==============================================================================
void var::swapWith (var& other) noexcept
{
    std::swap (type, other.type);
    std::swap (value, other.value);
}

var& var::operator= (const var& v)               { type->cleanUp (value); type = v.type; type->createCopy (value, v.value); return *this; }
var& var::operator= (const int v)                { type->cleanUp (value); type = &VariantType_Int::instance; value.intValue = v; return *this; }
var& var::operator= (const int64 v)              { type->cleanUp (value); type = &VariantType_Int64::instance; value.int64Value = v; return *this; }
var& var::operator= (const bool v)               { type->cleanUp (value); type = &VariantType_Bool::instance; value.boolValue = v; return *this; }
var& var::operator= (const double v)             { type->cleanUp (value); type = &VariantType_Double::instance; value.doubleValue = v; return *this; }
var& var::operator= (const char* const v)        { type->cleanUp (value); type = &VariantType_String::instance; new (value.stringValue) String (v); return *this; }
var& var::operator= (const String& v)            { type->cleanUp (value); type = &VariantType_String::instance; new (value.stringValue) String (v); return *this; }

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
var::var (var&& other) noexcept
    : type (other.type),
      value (other.value)
{
    other.type = &VariantType_Void::instance;
}

var& var::operator= (var&& other) noexcept
{
    swapWith (other);
    return *this;
}

var::var (String&& v)  : type (&VariantType_String::instance)
{
    new (value.stringValue) String (static_cast<String&&> (v));
}

var& var::operator= (String&& v)
{
    type->cleanUp (value);
    type = &VariantType_String::instance;
    new (value.stringValue) String (static_cast<String&&> (v));
    return *this;
}
#endif

//==============================================================================
bool var::equals (const var& other) const noexcept
{
    return type->equals (value, other.value, *other.type);
}

bool var::equalsWithSameType (const var& other) const noexcept
{
    return type == other.type && equals (other);
}

bool var::hasSameTypeAs (const var& other) const noexcept
{
    return type == other.type;
}

bool operator== (const var& v1, const var& v2) noexcept     { return v1.equals (v2); }
bool operator!= (const var& v1, const var& v2) noexcept     { return ! v1.equals (v2); }
bool operator== (const var& v1, const String& v2)           { return v1.toString() == v2; }
bool operator!= (const var& v1, const String& v2)           { return v1.toString() != v2; }
bool operator== (const var& v1, const char* const v2)       { return v1.toString() == v2; }
bool operator!= (const var& v1, const char* const v2)       { return v1.toString() != v2; }

//==============================================================================
var var::clone() const noexcept
{
    return type->clone (*this);
}
