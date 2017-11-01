/*
 * Carla misc utils based on Juce
 * Copyright (C) 2013 Raw Material Software Ltd.
 * Copyright (C) 2013-2015 Filipe Coelho <falktx@falktx.com>
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

#define DISTRHO_LEAK_DETECTOR_HPP_INCLUDED
#define DISTRHO_SCOPED_POINTER_HPP_INCLUDED
#define DISTRHO_LEAK_DETECTOR CARLA_LEAK_DETECTOR
#define DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR

#include "CarlaUtils.hpp"

#include <algorithm>

/** A good old-fashioned C macro concatenation helper.
    This combines two items (which may themselves be macros) into a single string,
    avoiding the pitfalls of the ## macro operator.
*/
#define CARLA_JOIN_MACRO_HELPER(a, b) a ## b
#define CARLA_JOIN_MACRO(item1, item2) CARLA_JOIN_MACRO_HELPER(item1, item2)

#ifdef DEBUG
/** This macro lets you embed a leak-detecting object inside a class.
    To use it, simply declare a CARLA_LEAK_DETECTOR(YourClassName) inside a private section
    of the class declaration. E.g.
    @code
    class MyClass
    {
    public:
        MyClass();
        void blahBlah();

    private:
        CARLA_LEAK_DETECTOR(MyClass)
    };
    @endcode
*/
# define CARLA_LEAK_DETECTOR(ClassName)                                           \
    friend class ::LeakedObjectDetector<ClassName>;                               \
    static const char* getLeakedObjectClassName() noexcept { return #ClassName; } \
    ::LeakedObjectDetector<ClassName> CARLA_JOIN_MACRO(leakDetector_, ClassName);

# define CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName) \
    CARLA_DECLARE_NON_COPY_CLASS(ClassName)                       \
    CARLA_LEAK_DETECTOR(ClassName)
#else
/** Don't use leak detection on release builds. */
# define CARLA_LEAK_DETECTOR(ClassName)
# define CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName) \
    CARLA_DECLARE_NON_COPY_CLASS(ClassName)
#endif

//=====================================================================================================================
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
    //=================================================================================================================
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
            carla_stderr2("*** Dangling pointer deletion! Class: '%s', Count: %i", getLeakedObjectClassName(),
                                                                                   getCounter().numObjects);
        }
    }

private:
    //=================================================================================================================
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
                carla_stderr2("*** Leaked objects detected: %i instance(s) of class '%s'", numObjects,
                                                                                           getLeakedObjectClassName());
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

//=====================================================================================================================
/**
    This class holds a pointer which is automatically deleted when this object goes
    out of scope.

    Once a pointer has been passed to a ScopedPointer, it will make sure that the pointer
    gets deleted when the ScopedPointer is deleted. Using the ScopedPointer on the stack or
    as member variables is a good way to use RAII to avoid accidentally leaking dynamically
    created objects.

    A ScopedPointer can be used in pretty much the same way that you'd use a normal pointer
    to an object. If you use the assignment operator to assign a different object to a
    ScopedPointer, the old one will be automatically deleted.

    A const ScopedPointer is guaranteed not to lose ownership of its object or change the
    object to which it points during its lifetime. This means that making a copy of a const
    ScopedPointer is impossible, as that would involve the new copy taking ownership from the
    old one.

    If you need to get a pointer out of a ScopedPointer without it being deleted, you
    can use the release() method.

    Something to note is the main difference between this class and the std::auto_ptr class,
    which is that ScopedPointer provides a cast-to-object operator, wheras std::auto_ptr
    requires that you always call get() to retrieve the pointer. The advantages of providing
    the cast is that you don't need to call get(), so can use the ScopedPointer in pretty much
    exactly the same way as a raw pointer. The disadvantage is that the compiler is free to
    use the cast in unexpected and sometimes dangerous ways - in particular, it becomes difficult
    to return a ScopedPointer as the result of a function. To avoid this causing errors,
    ScopedPointer contains an overloaded constructor that should cause a syntax error in these
    circumstances, but it does mean that instead of returning a ScopedPointer from a function,
    you'd need to return a raw pointer (or use a std::auto_ptr instead).
*/
template<class ObjectType>
class ScopedPointer
{
public:
    //=================================================================================================================
    /** Creates a ScopedPointer containing a null pointer. */
    ScopedPointer() noexcept
        : object(nullptr) {}

    /** Creates a ScopedPointer that owns the specified object. */
    ScopedPointer(ObjectType* const objectToTakePossessionOf) noexcept
        : object(objectToTakePossessionOf) {}

    /** Creates a ScopedPointer that takes its pointer from another ScopedPointer.

        Because a pointer can only belong to one ScopedPointer, this transfers
        the pointer from the other object to this one, and the other object is reset to
        be a null pointer.
    */
    ScopedPointer(ScopedPointer& objectToTransferFrom) noexcept
        : object(objectToTransferFrom.object)
    {
        objectToTransferFrom.object = nullptr;
    }

    /** Destructor.
        This will delete the object that this ScopedPointer currently refers to.
    */
    ~ScopedPointer()
    {
        delete object;
    }

    /** Changes this ScopedPointer to point to a new object.

        Because a pointer can only belong to one ScopedPointer, this transfers
        the pointer from the other object to this one, and the other object is reset to
        be a null pointer.

        If this ScopedPointer already points to an object, that object
        will first be deleted.
    */
    ScopedPointer& operator=(ScopedPointer& objectToTransferFrom)
    {
        if (this != objectToTransferFrom.getAddress())
        {
            // Two ScopedPointers should never be able to refer to the same object - if
            // this happens, you must have done something dodgy!
            CARLA_SAFE_ASSERT_RETURN(object == nullptr || object != objectToTransferFrom.object, *this);

            ObjectType* const oldObject = object;
            object = objectToTransferFrom.object;
            objectToTransferFrom.object = nullptr;
            delete oldObject;
        }

        return *this;
    }

    /** Changes this ScopedPointer to point to a new object.

        If this ScopedPointer already points to an object, that object
        will first be deleted.

        The pointer that you pass in may be a nullptr.
    */
    ScopedPointer& operator=(ObjectType* const newObjectToTakePossessionOf)
    {
        if (object != newObjectToTakePossessionOf)
        {
            ObjectType* const oldObject = object;
            object = newObjectToTakePossessionOf;
            delete oldObject;
        }

        return *this;
    }

    //=================================================================================================================
    /** Returns the object that this ScopedPointer refers to. */
    operator ObjectType*() const noexcept   { return object; }

    /** Returns the object that this ScopedPointer refers to. */
    ObjectType* get() const noexcept        { return object; }

    /** Returns the object that this ScopedPointer refers to. */
    ObjectType& operator*() const noexcept  { return *object; }

    /** Lets you access methods and properties of the object that this ScopedPointer refers to. */
    ObjectType* operator->() const noexcept { return object; }

    //=================================================================================================================
    /** Removes the current object from this ScopedPointer without deleting it.
        This will return the current object, and set the ScopedPointer to a null pointer.
    */
    ObjectType* release() noexcept          { ObjectType* const o = object; object = nullptr; return o; }

    //=================================================================================================================
    /** Swaps this object with that of another ScopedPointer.
        The two objects simply exchange their pointers.
    */
    void swapWith(ScopedPointer<ObjectType>& other) noexcept
    {
        // Two ScopedPointers should never be able to refer to the same object - if
        // this happens, you must have done something dodgy!
        CARLA_SAFE_ASSERT_RETURN(object != other.object || this == other.getAddress() || object == nullptr,);

        std::swap(object, other.object);
    }

private:
    //=================================================================================================================
    ObjectType* object;

    // (Required as an alternative to the overloaded & operator).
    const ScopedPointer* getAddress() const noexcept { return this; }

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ScopedPointer(const ScopedPointer&) = delete;
    ScopedPointer& operator=(const ScopedPointer&) = delete;
#else
    ScopedPointer(const ScopedPointer&);
    ScopedPointer& operator=(const ScopedPointer&);
#endif
};

//=====================================================================================================================
/** Compares a ScopedPointer with another pointer.
    This can be handy for checking whether this is a null pointer.
*/
template<class ObjectType>
bool operator==(const ScopedPointer<ObjectType>& pointer1, ObjectType* const pointer2) noexcept
{
    return static_cast<ObjectType*>(pointer1) == pointer2;
}

/** Compares a ScopedPointer with another pointer.
    This can be handy for checking whether this is a null pointer.
*/
template<class ObjectType>
bool operator!=(const ScopedPointer<ObjectType>& pointer1, ObjectType* const pointer2) noexcept
{
    return static_cast<ObjectType*>(pointer1) != pointer2;
}

//=====================================================================================================================
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
    //=================================================================================================================
    ValueType& value;
    const ValueType originalValue;

    CARLA_DECLARE_NON_COPY_CLASS(ScopedValueSetter)
    CARLA_PREVENT_HEAP_ALLOCATION
};

//=====================================================================================================================

#endif // CARLA_JUCE_UTILS_HPP_INCLUDED
