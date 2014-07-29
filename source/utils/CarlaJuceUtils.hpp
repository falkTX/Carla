/*
 * Carla misc utils based on Juce
 * Copyright (C) 2013 Raw Material Software Ltd.
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_JUCE_UTILS_HPP_INCLUDED
#define CARLA_JUCE_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

/** A good old-fashioned C macro concatenation helper.
   This combines two items (which may themselves be macros) into a single string,
   avoiding the pitfalls of the ## macro operator.
*/
#define CARLA_JOIN_MACRO_HELPER(a, b) a ## b
#define CARLA_JOIN_MACRO(item1, item2) CARLA_JOIN_MACRO_HELPER(item1, item2)

/** This macro lets you embed a leak-detecting object inside a class.\n
    To use it, simply declare a CARLA_LEAK_DETECTOR(YourClassName) inside a private section
    of the class declaration. E.g.
    \code
    class MyClass
    {
    public:
        MyClass();
        void blahBlah();

    private:
        CARLA_LEAK_DETECTOR(MyClass)
    };
    \endcode
*/
#define CARLA_LEAK_DETECTOR(ClassName)                                            \
    friend class ::LeakedObjectDetector<ClassName>;                               \
    static const char* getLeakedObjectClassName() noexcept { return #ClassName; } \
    ::LeakedObjectDetector<ClassName> CARLA_JOIN_MACRO(leakDetector, __LINE__);

#define CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName) \
    CARLA_DECLARE_NON_COPY_CLASS(ClassName)                      \
    CARLA_LEAK_DETECTOR(ClassName)

//==============================================================================
/**
    Embedding an instance of this class inside another class can be used as a low-overhead
    way of detecting leaked instances.

    This class keeps an internal static count of the number of instances that are
    active, so that when the app is shutdown and the static destructors are called,
    it can check whether there are any left-over instances that may have been leaked.

    To use it, use the CARLA_LEAK_DETECTOR macro as a simple way to put one in your
    class declaration.
*/
template <class OwnerClass>
class LeakedObjectDetector
{
public:
    //==============================================================================
    LeakedObjectDetector() noexcept                            { ++(getCounter().numObjects); }
    LeakedObjectDetector(const LeakedObjectDetector&) noexcept { ++(getCounter().numObjects); }

    ~LeakedObjectDetector() noexcept
    {
        if (--(getCounter().numObjects) < 0)
        {
            /** If you hit this, then you've managed to delete more instances of this class than you've
                created.. That indicates that you're deleting some dangling pointers.

                Note that although this assertion will have been triggered during a destructor, it might
                not be this particular deletion that's at fault - the incorrect one may have happened
                at an earlier point in the program, and simply not been detected until now.

                Most errors like this are caused by using old-fashioned, non-RAII techniques for
                your object management. Tut, tut. Always, always use ScopedPointers, OwnedArrays,
                ReferenceCountedObjects, etc, and avoid the 'delete' operator at all costs!
            */
            carla_stderr2("*** Dangling pointer deletion! Class: '%s', Count: %i", getLeakedObjectClassName(), getCounter().numObjects);
        }
    }

private:
    //==============================================================================
    class LeakCounter
    {
    public:
        LeakCounter() noexcept
            : numObjects(0) {}

        ~LeakCounter() noexcept
        {
            if (numObjects > 0)
            {
                /** If you hit this, then you've leaked one or more objects of the type specified by
                    the 'OwnerClass' template parameter - the name should have been printed by the line above.

                    If you're leaking, it's probably because you're using old-fashioned, non-RAII techniques for
                    your object management. Tut, tut. Always, always use ScopedPointers, OwnedArrays,
                    ReferenceCountedObjects, etc, and avoid the 'delete' operator at all costs!
                */
                carla_stderr2("*** Leaked objects detected: %i instance(s) of class '%s'", numObjects, getLeakedObjectClassName());
            }
        }

        // this should be an atomic...
        volatile int numObjects;
    };

    static const char* getLeakedObjectClassName() noexcept
    {
        return OwnerClass::getLeakedObjectClassName();
    }

    static LeakCounter& getCounter() noexcept
    {
        static LeakCounter counter;
        return counter;
    }
};

//==============================================================================
/**
    Helper class providing an RAII-based mechanism for temporarily setting and
    then re-setting a value.

    E.g. @code
    int x = 1;

    {
        ScopedValueSetter setter (x, 2);

        // x is now 2
    }

    // x is now 1 again

    {
        ScopedValueSetter setter (x, 3, 4);

        // x is now 3
    }

    // x is now 4
    @endcode
*/
template <typename ValueType>
class ScopedValueSetter
{
public:
    /** Creates a ScopedValueSetter that will immediately change the specified value to the
        given new value, and will then reset it to its original value when this object is deleted.
        Must be used only for 'noexcept' compatible types.
    */
    ScopedValueSetter(ValueType& valueToSet, ValueType newValue) noexcept
        : value(valueToSet),
          originalValue(valueToSet)
    {
        valueToSet = newValue;
    }

    /** Creates a ScopedValueSetter that will immediately change the specified value to the
        given new value, and will then reset it to be valueWhenDeleted when this object is deleted.
    */
    ScopedValueSetter(ValueType& valueToSet, ValueType newValue, ValueType valueWhenDeleted) noexcept
        : value(valueToSet),
          originalValue(valueWhenDeleted)
    {
        valueToSet = newValue;
    }

    ~ScopedValueSetter() noexcept
    {
        value = originalValue;
    }

private:
    //==============================================================================
    ValueType& value;
    const ValueType originalValue;

    CARLA_DECLARE_NON_COPY_CLASS(ScopedValueSetter)
    CARLA_PREVENT_HEAP_ALLOCATION
};

#endif // CARLA_JUCE_UTILS_HPP_INCLUDED
