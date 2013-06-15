/*
 * Carla misc utils imported from Juce source code
 * Copyright (c) 2013 Raw Material Software Ltd.
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef __CARLA_JUCE_UTILS_HPP__
#define __CARLA_JUCE_UTILS_HPP__

#include "CarlaUtils.hpp"

#include <algorithm>

#define CARLA_DECLARE_NON_COPYABLE(ClassName) \
private:                                      \
    ClassName(ClassName&);                    \
    ClassName(const ClassName&);              \
    ClassName& operator=(const ClassName&);

/** This is a shorthand way of writing both a CARLA_DECLARE_NON_COPYABLE and
    CARLA_LEAK_DETECTOR macro for a class.
*/
#define CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName) \
    CARLA_DECLARE_NON_COPYABLE(ClassName)                        \
    CARLA_LEAK_DETECTOR(ClassName)

/** struct versions of the above. */
#define CARLA_DECLARE_NON_COPY_STRUCT(StructName) \
    StructName(StructName&) = delete;             \
    StructName(const StructName&) = delete;       \
    StructName& operator=(const StructName&) = delete;

#define CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(StructName) \
    CARLA_DECLARE_NON_COPY_STRUCT(StructName)                        \
    CARLA_LEAK_DETECTOR(StructName)

/** This macro can be added to class definitions to disable the use of new/delete to
    allocate the object on the heap, forcing it to only be used as a stack or member variable.
*/
#define CARLA_PREVENT_HEAP_ALLOCATION   \
private:                                \
    static void* operator new (size_t); \
    static void operator delete (void*);

/** A good old-fashioned C macro concatenation helper.
   This combines two items (which may themselves be macros) into a single string,
   avoiding the pitfalls of the ## macro operator.
*/
#define CARLA_JOIN_MACRO_HELPER(a, b) a ## b
#define CARLA_JOIN_MACRO(item1, item2)  CARLA_JOIN_MACRO_HELPER (item1, item2)

/** Remove unsupported macros */
#ifndef CARLA_PROPER_CPP11_SUPPORT
# undef CARLA_DECLARE_NON_COPY_STRUCT
# define CARLA_DECLARE_NON_COPY_STRUCT(...)
#endif

//==============================================================================
/**
    Embedding an instance of this class inside another class can be used as a low-overhead
    way of detecting leaked instances.

    This class keeps an internal static count of the number of instances that are
    active, so that when the app is shutdown and the static destructors are called,
    it can check whether there are any left-over instances that may have been leaked.

    To use it, use the CARLA_LEAK_DETECTOR macro as a simple way to put one in your
    class declaration. Have a look through the carla codebase for examples, it's used
    in most of the classes.
*/
template <class OwnerClass>
class LeakedObjectDetector
{
public:
    //==============================================================================
    LeakedObjectDetector() noexcept                                 { ++(getCounter().numObjects); }
    LeakedObjectDetector (const LeakedObjectDetector&) noexcept     { ++(getCounter().numObjects); }

    ~LeakedObjectDetector()
    {
        if (--(getCounter().numObjects) < 0)
        {
            carla_stderr("*** Dangling pointer deletion! Class: '%s'", getLeakedObjectClassName());

            /** If you hit this, then you've managed to delete more instances of this class than you've
                created.. That indicates that you're deleting some dangling pointers.

                Note that although this assertion will have been triggered during a destructor, it might
                not be this particular deletion that's at fault - the incorrect one may have happened
                at an earlier point in the program, and simply not been detected until now.

                Most errors like this are caused by using old-fashioned, non-RAII techniques for
                your object management. Tut, tut. Always, always use ScopedPointers, OwnedArrays,
                ReferenceCountedObjects, etc, and avoid the 'delete' operator at all costs!
            */
            //assert(false);
        }
    }

private:
    //==============================================================================
    class LeakCounter
    {
    public:
        LeakCounter() noexcept
        {
            numObjects = 0;
        }

        ~LeakCounter()
        {
            if (numObjects > 0)
            {
                carla_stderr("*** Leaked objects detected: %i instance(s) of class '%s'", numObjects, getLeakedObjectClassName());

                /** If you hit this, then you've leaked one or more objects of the type specified by
                    the 'OwnerClass' template parameter - the name should have been printed by the line above.

                    If you're leaking, it's probably because you're using old-fashioned, non-RAII techniques for
                    your object management. Tut, tut. Always, always use ScopedPointers, OwnedArrays,
                    ReferenceCountedObjects, etc, and avoid the 'delete' operator at all costs!
                */
                //assert(false);
            }
        }

        volatile int numObjects;
    };

    static const char* getLeakedObjectClassName()
    {
        return OwnerClass::getLeakedObjectClassName();
    }

    static LeakCounter& getCounter() noexcept
    {
        static LeakCounter counter;
        return counter;
    }
};

#define CARLA_LEAK_DETECTOR(OwnerClass) \
    friend class LeakedObjectDetector<OwnerClass>; \
    static const char* getLeakedObjectClassName() noexcept { return #OwnerClass; } \
    LeakedObjectDetector<OwnerClass> CARLA_JOIN_MACRO (leakDetector, __LINE__);


//==============================================================================
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

    Important note: The class is designed to hold a pointer to an object, NOT to an array!
    It calls delete on its payload, not delete[], so do not give it an array to hold! For
    that kind of purpose, you should be using HeapBlock or Array instead.

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
template <class ObjectType>
class ScopedPointer
{
public:
    //==============================================================================
    /** Creates a ScopedPointer containing a null pointer. */
    ScopedPointer() noexcept
        : object(nullptr)
    {
    }

    /** Creates a ScopedPointer that owns the specified object. */
    ScopedPointer(ObjectType* const objectToTakePossessionOf) noexcept
        : object(objectToTakePossessionOf)
    {
    }

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
            assert(object == nullptr || object != objectToTransferFrom.object);

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

    //==============================================================================
    /** Returns the object that this ScopedPointer refers to. */
    operator ObjectType*() const noexcept   { return object; }

    /** Returns the object that this ScopedPointer refers to. */
    ObjectType* get() const noexcept        { return object; }

    /** Returns the object that this ScopedPointer refers to. */
    ObjectType& operator*() const noexcept  { return *object; }

    /** Lets you access methods and properties of the object that this ScopedPointer refers to. */
    ObjectType* operator->() const noexcept { return object; }

    //==============================================================================
    /** Removes the current object from this ScopedPointer without deleting it.
        This will return the current object, and set the ScopedPointer to a null pointer.
    */
    ObjectType* release() noexcept          { ObjectType* const o = object; object = nullptr; return o; }

    //==============================================================================
    /** Swaps this object with that of another ScopedPointer.
        The two objects simply exchange their pointers.
    */
    void swapWith(ScopedPointer<ObjectType>& other) noexcept
    {
        // Two ScopedPointers should never be able to refer to the same object - if
        // this happens, you must have done something dodgy!
        assert(object != other.object || this == other.getAddress());

        std::swap(object, other.object);
    }

private:
    //==============================================================================
    ObjectType* object;

    // (Required as an alternative to the overloaded & operator).
    const ScopedPointer* getAddress() const { return this; }

#if ! defined(CARLA_CC_MSVC)  // (MSVC can't deal with multiple copy constructors)
    /* The copy constructors are private to stop people accidentally copying a const ScopedPointer
       (the compiler would let you do so by implicitly casting the source to its raw object pointer).

       A side effect of this is that in a compiler that doesn't support C++11, you may hit an
       error when you write something like this:

          ScopedPointer<MyClass> m = new MyClass();  // Compile error: copy constructor is private.

       Even though the compiler would normally ignore the assignment here, it can't do so when the
       copy constructor is private. It's very easy to fix though - just write it like this:

          ScopedPointer<MyClass> m (new MyClass());  // Compiles OK

       It's probably best to use the latter form when writing your object declarations anyway, as
       this is a better representation of the code that you actually want the compiler to produce.
    */
    ScopedPointer(const ScopedPointer&);
    ScopedPointer& operator=(const ScopedPointer&);
#endif
};

//==============================================================================
/** Compares a ScopedPointer with another pointer.
    This can be handy for checking whether this is a null pointer.
*/
template <class ObjectType>
bool operator==(const ScopedPointer<ObjectType>& pointer1, ObjectType* const pointer2) noexcept
{
    return static_cast<ObjectType*>(pointer1) == pointer2;
}

/** Compares a ScopedPointer with another pointer.
    This can be handy for checking whether this is a null pointer.
*/
template <class ObjectType>
bool operator!=(const ScopedPointer<ObjectType>& pointer1, ObjectType* const pointer2) noexcept
{
    return static_cast<ObjectType*>(pointer1) != pointer2;
}

#endif // __CARLA_JUCE_UTILS_HPP__
