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
public:
    //==============================================================================
    /** Creates an empty array. */
    ArrayAllocationBase() noexcept
        : numAllocated (0)
    {
    }

    /** Destructor. */
    ~ArrayAllocationBase() noexcept
    {
    }

   #if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
    ArrayAllocationBase (ArrayAllocationBase<ElementType>&& other) noexcept
        : elements (static_cast<HeapBlock<ElementType>&&> (other.elements)),
          numAllocated (other.numAllocated)
    {
    }

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

        @param numElements  the number of elements that are needed
    */
    bool setAllocatedSize (const int numElements) noexcept
    {
        if (numAllocated != numElements)
        {
            if (numElements > 0)
            {
                if (! elements.realloc ((size_t) numElements))
                    return false;
            }
            else
            {
                elements.free();
            }

            numAllocated = numElements;
        }

        return true;
    }

    /** Increases the amount of storage allocated if it is less than a given amount.

        This will retain any data currently held in the array, but will add
        extra space at the end to make sure there it's at least as big as the size
        passed in. If it's already bigger, no action is taken.

        @param minNumElements  the minimum number of elements that are needed
    */
    bool ensureAllocatedSize (const int minNumElements) noexcept
    {
        if (minNumElements > numAllocated)
            return setAllocatedSize ((minNumElements + minNumElements / 2 + 8) & ~7);

        return true;
    }

    /** Minimises the amount of storage allocated so that it's no more than
        the given number of elements.
    */
    bool shrinkToNoMoreThan (const int maxNumElements) noexcept
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

    //==============================================================================
    HeapBlock<ElementType> elements;
    int numAllocated;

private:
    CARLA_DECLARE_NON_COPY_CLASS (ArrayAllocationBase)
};

}

#endif // WATER_ARRAYALLOCATIONBASE_H_INCLUDED
