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

#ifndef WATER_ARRAYALLOCATIONBASE_H_INCLUDED
#define WATER_ARRAYALLOCATIONBASE_H_INCLUDED

#include "../memory/HeapBlock.h"

#include "CarlaUtils.hpp"

namespace water {

//==============================================================================
/**
    Implements some basic array storage allocation functions.

    This class isn't really for public use - it's used by the other
    array classes, but might come in handy for some purposes.

    @see Array, OwnedArray, ReferenceCountedArray
*/
template <class ElementType>
class ArrayAllocationBase
{
#if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
private:
   #if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 5
    template <typename T>
    struct IsTriviallyCopyable : std::integral_constant<bool, false> {};
   #else
    template <typename T>
    using IsTriviallyCopyable = std::is_trivially_copyable<T>;
   #endif

    template <typename T>
    using TriviallyCopyableVoid = typename std::enable_if<IsTriviallyCopyable<T>::value, void>::type;

    template <typename T>
    using TriviallyCopyableBool = typename std::enable_if<IsTriviallyCopyable<T>::value, bool>::type;

    template <typename T>
    using NonTriviallyCopyableVoid = typename std::enable_if<! IsTriviallyCopyable<T>::value, void>::type;

    template <typename T>
    using NonTriviallyCopyableBool = typename std::enable_if<! IsTriviallyCopyable<T>::value, bool>::type;
#endif // WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS

public:
    //==============================================================================
    /** Creates an empty array. */
    ArrayAllocationBase() noexcept
        : elements(),
          numAllocated(0)
    {
    }

    /** Destructor. */
    ~ArrayAllocationBase() noexcept
    {
    }

   #if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
    ArrayAllocationBase (ArrayAllocationBase<ElementType>&& other) noexcept
        : elements (static_cast<HeapBlock<ElementType>&&> (other.elements)),
          numAllocated (other.numAllocated) {}

    ArrayAllocationBase& operator= (ArrayAllocationBase<ElementType>&& other) noexcept
    {
        elements = static_cast<HeapBlock<ElementType>&&> (other.elements);
        numAllocated = other.numAllocated;
        return *this;
    }
   #endif

    //==============================================================================
    /** Changes the amount of storage allocated.

        This will retain any data currently held in the array, and either add or
        remove extra space at the end.

        @param numNewElements  the number of elements that are needed
    */
   #if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
    template <typename T = ElementType> TriviallyCopyableBool<T>
   #else
    bool
   #endif
    setAllocatedSize (const size_t numNewElements) noexcept
    {
        if (numAllocated != numNewElements)
        {
            if (numNewElements > 0)
            {
                if (! elements.realloc ((size_t) numNewElements))
                    return false;
            }
            else
            {
                elements.free();
            }

            numAllocated = numNewElements;
        }

        return true;
    }

   #if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
    template <typename T = ElementType>
    NonTriviallyCopyableBool<T> setAllocatedSize (const size_t numNewElements) noexcept
    {
        if (numAllocated != numNewElements)
        {
            if (numNewElements > 0)
            {
                HeapBlock<ElementType> newElements;

                if (! newElements.malloc (numNewElements))
                    return false;

                for (size_t i = 0; i < numNewElements; ++i)
                {
                    if (i < numAllocated)
                    {
                        new (newElements + i) ElementType (std::move (elements[i]));
                        elements[i].~ElementType();
                    }
                    else
                    {
                        new (newElements + i) ElementType ();
                    }
                }

                elements = std::move (newElements);
            }
            else
            {
                elements.free();
            }

            numAllocated = numNewElements;
        }

        return true;
    }
   #endif

    /** Increases the amount of storage allocated if it is less than a given amount.

        This will retain any data currently held in the array, but will add
        extra space at the end to make sure there it's at least as big as the size
        passed in. If it's already bigger, no action is taken.

        @param minNumElements  the minimum number of elements that are needed
    */
    bool ensureAllocatedSize (const size_t minNumElements) noexcept
    {
        if (minNumElements > numAllocated)
            return setAllocatedSize ((minNumElements + minNumElements / 2U + 8U) & ~7U);

        return true;
    }

    /** Minimises the amount of storage allocated so that it's no more than
        the given number of elements.
    */
    bool shrinkToNoMoreThan (const size_t maxNumElements) noexcept
    {
        if (maxNumElements < numAllocated)
            return setAllocatedSize (maxNumElements);

        return true;
    }

    /** Swap the contents of two objects. */
    void swapWith (ArrayAllocationBase <ElementType>& other) noexcept
    {
        elements.swapWith (other.elements);
        std::swap (numAllocated, other.numAllocated);
    }

   #if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
    template <typename T = ElementType> TriviallyCopyableVoid<T>
   #else
    void
   #endif
    moveMemory (ElementType* target, const ElementType* source, const size_t numElements) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(target != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(source != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(target != source,);
        CARLA_SAFE_ASSERT_RETURN(numElements != 0,);

        std::memmove (target, source, ((size_t) numElements) * sizeof (ElementType));
    }

   #if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
    template <typename T = ElementType>
    NonTriviallyCopyableVoid<T> moveMemory (ElementType* target, const ElementType* source, const size_t numElements) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(target != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(source != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(target != source,);
        CARLA_SAFE_ASSERT_RETURN(numElements != 0,);

        if (target > source)
        {
            for (size_t i = 0; i < numElements; ++i)
            {
                moveElement (target, std::move (*source));
                ++target;
                ++source;
            }
        }
        else
        {
            for (size_t i = 0; i < numElements; ++i)
            {
                moveElement (target, std::move (*source));
                --target;
                --source;
            }
        }
    }

    void moveElement (ElementType* destination, const ElementType&& source)
    {
        destination->~ElementType();
        new (destination) ElementType (std::move (source));
    }
   #endif

    //==============================================================================
    HeapBlock<ElementType> elements;
    size_t numAllocated;

private:
    CARLA_DECLARE_NON_COPY_CLASS (ArrayAllocationBase)
};

}

#endif // WATER_ARRAYALLOCATIONBASE_H_INCLUDED
