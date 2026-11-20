/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2025 Filipe Coelho <falktx@falktx.com>

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

#ifndef WATER_OWNEDARRAY_H_INCLUDED
#define WATER_OWNEDARRAY_H_INCLUDED

#include "ArrayAllocationBase.h"

#include <memory>

namespace water {

//==============================================================================
/** An array designed for holding objects.

    This holds a list of pointers to objects, and will automatically
    delete the objects when they are removed from the array, or when the
    array is itself deleted.

    Declare it in the form:  OwnedArray<MyObjectClass>

    ..and then add new objects, e.g.   myOwnedArray.add (new MyObjectClass());

    After adding objects, they are 'owned' by the array and will be deleted when
    removed or replaced.

    To make all the array's methods thread-safe, pass in "CriticalSection" as the templated
    TypeOfCriticalSectionToUse parameter, instead of the default DummyCriticalSection.

    @see Array, ReferenceCountedArray, StringArray, CriticalSection
*/
template <class ObjectClass>

class OwnedArray
{
public:
    //==============================================================================
    /** Creates an empty array. */
    OwnedArray() noexcept
        : numUsed (0)
    {
    }

    /** Deletes the array and also deletes any objects inside it.

        To get rid of the array without deleting its objects, use its
        clear (false) method before deleting it.
    */
    ~OwnedArray()
    {
        deleteAllObjects();
    }

    //==============================================================================
    /** Clears the array, optionally deleting the objects inside it first. */
    void clear (bool deleteObjects = true)
    {
        if (deleteObjects)
            deleteAllObjects();

        data.setAllocatedSize (0);
        numUsed = 0;
    }

    //==============================================================================
    /** Clears the array, optionally deleting the objects inside it first. */
    void clearQuick (bool deleteObjects)
    {
        if (deleteObjects)
            deleteAllObjects();

        numUsed = 0;
    }

    //==============================================================================
    /** Returns the number of items currently in the array.
        @see operator[]
    */
    inline size_t size() const noexcept
    {
        return numUsed;
    }

    /** Returns true if the array is empty, false otherwise. */
    inline bool isEmpty() const noexcept
    {
        return size() == 0;
    }

    /** Returns a pointer to the object at this index in the array.

        If the index is out-of-range, this will return a null pointer, (and
        it could be null anyway, because it's ok for the array to hold null
        pointers as well as objects).

        @see getUnchecked
    */
    inline ObjectClass* operator[] (const size_t index) const noexcept
    {
        if (index < numUsed)
        {
            CARLA_SAFE_ASSERT_RETURN(data.elements != nullptr, nullptr);
            return data.elements [index];
        }

        return nullptr;
    }

    /** Returns a pointer to the object at this index in the array, without checking whether the index is in-range.

        This is a faster and less safe version of operator[] which doesn't check the index passed in, so
        it can be used when you're sure the index is always going to be legal.
    */
    inline ObjectClass* getUnchecked (const int index) const noexcept
    {
        return data.elements [index];
    }

    /** Returns a pointer to the first object in the array.

        This will return a null pointer if the array's empty.
        @see getLast
    */
    inline ObjectClass* getFirst() const noexcept
    {
        if (numUsed > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(data.elements != nullptr, nullptr);
            return data.elements [0];
        }

        return nullptr;
    }

    /** Returns a pointer to the last object in the array.

        This will return a null pointer if the array's empty.
        @see getFirst
    */
    inline ObjectClass* getLast() const noexcept
    {
        if (numUsed > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(data.elements != nullptr, nullptr);
            return data.elements [numUsed - 1];
        }

        return nullptr;
    }

    /** Returns a pointer to the actual array data.
        This pointer will only be valid until the next time a non-const method
        is called on the array.
    */
    inline ObjectClass** getRawDataPointer() noexcept
    {
        return data.elements;
    }

    //==============================================================================
    /** Returns a pointer to the first element in the array.
        This method is provided for compatibility with standard C++ iteration mechanisms.
    */
    inline ObjectClass** begin() const noexcept
    {
        return data.elements;
    }

    /** Returns a pointer to the element which follows the last element in the array.
        This method is provided for compatibility with standard C++ iteration mechanisms.
    */
    inline ObjectClass** end() const noexcept
    {
       #ifdef DEBUG
        if (data.elements == nullptr || numUsed <= 0) // (to keep static analysers happy)
            return data.elements;
       #endif

        return data.elements + numUsed;
    }

    //==============================================================================
    /** Finds the index of an object which might be in the array.

        @param objectToLookFor    the object to look for
        @returns                  the index at which the object was found, or -1 if it's not found
    */
    int indexOf (const ObjectClass* objectToLookFor) const noexcept
    {
        ObjectClass* const* e = data.elements.getData();
        ObjectClass* const* const end_ = e + numUsed;

        for (; e != end_; ++e)
            if (objectToLookFor == *e)
                return static_cast<int> (e - data.elements.getData());

        return -1;
    }

    /** Returns true if the array contains a specified object.

        @param objectToLookFor      the object to look for
        @returns                    true if the object is in the array
    */
    bool contains (const ObjectClass* objectToLookFor) const noexcept
    {
        ObjectClass* const* e = data.elements.getData();
        ObjectClass* const* const end_ = e + numUsed;

        for (; e != end_; ++e)
            if (objectToLookFor == *e)
                return true;

        return false;
    }

    //==============================================================================
    /** Appends a new object to the end of the array.

        Note that the this object will be deleted by the OwnedArray when it
        is removed, so be careful not to delete it somewhere else.

        Also be careful not to add the same object to the array more than once,
        as this will obviously cause deletion of dangling pointers.

        @param newObject    the new object to add to the array
        @returns            the new object that was added
        @see set, insert, addIfNotAlreadyThere, addSorted
    */
    ObjectClass* add (ObjectClass* newObject) noexcept
    {
        if (! data.ensureAllocatedSize (numUsed + 1))
            return nullptr;

        data.elements [numUsed++] = newObject;
        return newObject;
    }

    /** Inserts a new object into the array at the given index.

        Note that the this object will be deleted by the OwnedArray when it
        is removed, so be careful not to delete it somewhere else.

        If the index is less than 0 or greater than the size of the array, the
        element will be added to the end of the array.
        Otherwise, it will be inserted into the array, moving all the later elements
        along to make room.

        Be careful not to add the same object to the array more than once,
        as this will obviously cause deletion of dangling pointers.

        @param indexToInsertAt      the index at which the new element should be inserted
        @param newObject            the new object to add to the array
        @returns                    the new object that was added
        @see add, addSorted, addIfNotAlreadyThere, set
    */
    ObjectClass* insert (int indexToInsertAt, ObjectClass* newObject) noexcept
    {
        if (indexToInsertAt < 0)
            return add (newObject);

        size_t uindexToInsertAt = static_cast<size_t>(indexToInsertAt);

        if (uindexToInsertAt > numUsed)
            uindexToInsertAt = numUsed;

        if (! data.ensureAllocatedSize (numUsed + 1))
            return nullptr;

        ObjectClass** const e = data.elements + uindexToInsertAt;
        const int numToMove = numUsed - uindexToInsertAt;

        if (numToMove > 0)
            std::memmove (e + 1, e, sizeof (ObjectClass*) * (size_t) numToMove);

        *e = newObject;
        ++numUsed;
        return newObject;
    }

    /** Inserts an array of values into this array at a given position.

        If the index is less than 0 or greater than the size of the array, the
        new elements will be added to the end of the array.
        Otherwise, they will be inserted into the array, moving all the later elements
        along to make room.

        @param indexToInsertAt      the index at which the first new element should be inserted
        @param newObjects           the new values to add to the array
        @param numberOfElements     how many items are in the array
        @see insert, add, addSorted, set
    */
    void insertArray (const size_t indexToInsertAt,
                      ObjectClass* const* newObjects,
                      const size_t numberOfElements)
    {
        if (numberOfElements > 0)
        {
            data.ensureAllocatedSize (numUsed + numberOfElements);
            ObjectClass** insertPos = data.elements;

            if (indexToInsertAt < numUsed)
            {
                insertPos += indexToInsertAt;
                const size_t numberToMove = numUsed - indexToInsertAt;
                std::memmove (insertPos + numberOfElements, insertPos, numberToMove * sizeof (ObjectClass*));
            }
            else
            {
                insertPos += numUsed;
            }

            numUsed += numberOfElements;

            for (size_t i=0; i < numberOfElements; ++i)
                *insertPos++ = *newObjects++;
        }
    }

    /** Appends a new object at the end of the array as long as the array doesn't
        already contain it.

        If the array already contains a matching object, nothing will be done.

        @param newObject   the new object to add to the array
        @returns           true if the new object was added, false otherwise
    */
    bool addIfNotAlreadyThere (ObjectClass* newObject) noexcept
    {
        if (contains (newObject))
            return false;

        return add (newObject) != nullptr;
    }

    /** Replaces an object in the array with a different one.

        If the index is less than zero, this method does nothing.
        If the index is beyond the end of the array, the new object is added to the end of the array.

        Be careful not to add the same object to the array more than once,
        as this will obviously cause deletion of dangling pointers.

        @param indexToChange        the index whose value you want to change
        @param newObject            the new value to set for this index.
        @param deleteOldElement     whether to delete the object that's being replaced with the new one
        @see add, insert, remove
    */
    ObjectClass* set (int indexToChange, ObjectClass* newObject, bool deleteOldElement = true)
    {
        if (indexToChange >= 0)
        {
            std::unique_ptr<ObjectClass> toDelete;

            {
                if (indexToChange < numUsed)
                {
                    if (deleteOldElement)
                    {
                        toDelete = data.elements [indexToChange];

                        if (toDelete == newObject)
                            toDelete.release();
                    }

                    data.elements [indexToChange] = newObject;
                }
                else
                {
                    data.ensureAllocatedSize (numUsed + 1);
                    data.elements [numUsed++] = newObject;
                }
            }
        }
        else
        {
            wassertfalse; // you're trying to set an object at a negative index, which doesn't have
                          // any effect - but since the object is not being added, it may be leaking..
        }

        return newObject;
    }

    /** Adds elements from another array to the end of this array.

        @param arrayToAddFrom       the array from which to copy the elements
        @param startIndex           the first element of the other array to start copying from
        @param numElementsToAdd     how many elements to add from the other array. If this
                                    value is negative or greater than the number of available elements,
                                    all available elements will be copied.
        @see add
    */
    template <class OtherArrayType>
    void addArray (const OtherArrayType& arrayToAddFrom,
                   int startIndex = 0,
                   int numElementsToAdd = -1)
    {
        if (startIndex < 0)
        {
            wassertfalse;
            startIndex = 0;
        }

        if (numElementsToAdd < 0 || startIndex + numElementsToAdd > arrayToAddFrom.size())
            numElementsToAdd = arrayToAddFrom.size() - startIndex;

        data.ensureAllocatedSize (numUsed + numElementsToAdd);
        wassert (numElementsToAdd <= 0 || data.elements != nullptr);

        while (--numElementsToAdd >= 0)
        {
            data.elements [numUsed] = arrayToAddFrom.getUnchecked (startIndex++);
            ++numUsed;
        }
    }

    /** Adds copies of the elements in another array to the end of this array.

        The other array must be either an OwnedArray of a compatible type of object, or an Array
        containing pointers to the same kind of object. The objects involved must provide
        a copy constructor, and this will be used to create new copies of each element, and
        add them to this array.

        @param arrayToAddFrom       the array from which to copy the elements
        @param startIndex           the first element of the other array to start copying from
        @param numElementsToAdd     how many elements to add from the other array. If this
                                    value is negative or greater than the number of available elements,
                                    all available elements will be copied.
        @see add
    */
    template <class OtherArrayType>
    void addCopiesOf (const OtherArrayType& arrayToAddFrom,
                      size_t startIndex = 0,
                      int numElementsToAdd = -1)
    {
        if (numElementsToAdd < 0 || startIndex + numElementsToAdd > arrayToAddFrom.size())
            numElementsToAdd = arrayToAddFrom.size() - startIndex;

        data.ensureAllocatedSize (numUsed + numElementsToAdd);
        wassert (numElementsToAdd <= 0 || data.elements != nullptr);

        while (--numElementsToAdd >= 0)
            data.elements [numUsed++] = createCopyIfNotNull (arrayToAddFrom.getUnchecked (startIndex++));
    }

    /** Inserts a new object into the array assuming that the array is sorted.

        This will use a comparator to find the position at which the new object
        should go. If the array isn't sorted, the behaviour of this
        method will be unpredictable.

        @param comparator   the comparator to use to compare the elements - see the sort method
                            for details about this object's structure
        @param newObject    the new object to insert to the array
        @returns the index at which the new object was added
        @see add, sort, indexOfSorted
    */
    template <class ElementComparator>
    int addSorted (ElementComparator& comparator, ObjectClass* const newObject) noexcept
    {
        ignoreUnused (comparator); // if you pass in an object with a static compareElements() method, this
                                   // avoids getting warning messages about the parameter being unused
        const int index = findInsertIndexInSortedArray (comparator, data.elements.getData(), newObject, 0, numUsed);
        insert (index, newObject);
        return index;
    }

    /** Finds the index of an object in the array, assuming that the array is sorted.

        This will use a comparator to do a binary-chop to find the index of the given
        element, if it exists. If the array isn't sorted, the behaviour of this
        method will be unpredictable.

        @param comparator           the comparator to use to compare the elements - see the sort()
                                    method for details about the form this object should take
        @param objectToLookFor      the object to search for
        @returns                    the index of the element, or -1 if it's not found
        @see addSorted, sort
    */
    template <typename ElementComparator>
    int indexOfSorted (ElementComparator& comparator, const ObjectClass* const objectToLookFor) const noexcept
    {
        ignoreUnused (comparator);
        int s = 0, e = numUsed;

        while (s < e)
        {
            if (comparator.compareElements (objectToLookFor, data.elements [s]) == 0)
                return s;

            const int halfway = (s + e) / 2;
            if (halfway == s)
                break;

            if (comparator.compareElements (objectToLookFor, data.elements [halfway]) >= 0)
                s = halfway;
            else
                e = halfway;
        }

        return -1;
    }

    //==============================================================================
    /** Removes an object from the array.

        This will remove the object at a given index (optionally also
        deleting it) and move back all the subsequent objects to close the gap.
        If the index passed in is out-of-range, nothing will happen.

        @param indexToRemove    the index of the element to remove
        @param deleteObject     whether to delete the object that is removed
        @see removeObject, removeRange
    */
    void remove (const size_t indexToRemove, bool deleteObject = true)
    {
        std::unique_ptr<ObjectClass> toDelete;

        if (indexToRemove < numUsed)
        {
            ObjectClass** const e = data.elements + indexToRemove;

            if (deleteObject)
                toDelete.reset(*e);

            --numUsed;
            const size_t numToShift = numUsed - indexToRemove;

            if (numToShift > 0)
                std::memmove (e, e + 1, sizeof (ObjectClass*) * numToShift);
        }

        if ((numUsed << 1) < data.numAllocated)
            minimiseStorageOverheads();
    }

    /** Removes and returns an object from the array without deleting it.

        This will remove the object at a given index and return it, moving back all
        the subsequent objects to close the gap. If the index passed in is out-of-range,
        nothing will happen.

        @param indexToRemove    the index of the element to remove
        @see remove, removeObject, removeRange
    */
    ObjectClass* removeAndReturn (const size_t indexToRemove)
    {
        ObjectClass* removedItem = nullptr;
        if (indexToRemove < numUsed)
        {
            ObjectClass** const e = data.elements + indexToRemove;
            removedItem = *e;

            --numUsed;
            const size_t numToShift = numUsed - indexToRemove;

            if (numToShift > 0)
                std::memmove (e, e + 1, sizeof (ObjectClass*) * (size_t) numToShift);

            if ((numUsed << 1) < data.numAllocated)
                minimiseStorageOverheads();
        }

        return removedItem;
    }

    /** Removes a specified object from the array.

        If the item isn't found, no action is taken.

        @param objectToRemove   the object to try to remove
        @param deleteObject     whether to delete the object (if it's found)
        @see remove, removeRange
    */
    void removeObject (const ObjectClass* objectToRemove, bool deleteObject = true)
    {
        ObjectClass** const e = data.elements.getData();

        for (int i = 0; i < numUsed; ++i)
        {
            if (objectToRemove == e[i])
            {
                remove (i, deleteObject);
                break;
            }
        }
    }

    /** Removes a range of objects from the array.

        This will remove a set of objects, starting from the given index,
        and move any subsequent elements down to close the gap.

        If the range extends beyond the bounds of the array, it will
        be safely clipped to the size of the array.

        @param startIndex       the index of the first object to remove
        @param numberToRemove   how many objects should be removed
        @param deleteObjects    whether to delete the objects that get removed
        @see remove, removeObject
    */
    void removeRange (size_t startIndex, const size_t numberToRemove, bool deleteObjects = true)
    {
        const size_t endIndex = jlimit ((size_t)0U, numUsed, startIndex + numberToRemove);
        startIndex = jlimit ((size_t)0U, numUsed, startIndex);

        if (endIndex > startIndex)
        {
            if (deleteObjects)
            {
                for (int i = startIndex; i < endIndex; ++i)
                {
                    delete data.elements [i];
                    data.elements [i] = nullptr; // (in case one of the destructors accesses this array and hits a dangling pointer)
                }
            }

            const size_t rangeSize = endIndex - startIndex;
            ObjectClass** e = data.elements + startIndex;
            size_t numToShift = numUsed - endIndex;
            numUsed -= rangeSize;

            for (size_t i=0; i < numToShift; ++i)
            {
                *e = e [rangeSize];
                ++e;
            }

            if ((numUsed << 1) < data.numAllocated)
                minimiseStorageOverheads();
        }
    }

    /** Removes the last n objects from the array.

        @param howManyToRemove   how many objects to remove from the end of the array
        @param deleteObjects     whether to also delete the objects that are removed
        @see remove, removeObject, removeRange
    */
    void removeLast (int howManyToRemove = 1,
                     bool deleteObjects = true)
    {
        if (howManyToRemove >= numUsed)
            clear (deleteObjects);
        else
            removeRange (numUsed - howManyToRemove, howManyToRemove, deleteObjects);
    }

    /** Swaps a pair of objects in the array.

        If either of the indexes passed in is out-of-range, nothing will happen,
        otherwise the two objects at these positions will be exchanged.
    */
    void swap (const size_t index1,
               const size_t index2) noexcept
    {
        if (index1 < numUsed && index2 < numUsed)
        {
            std::swap (data.elements [index1],
                       data.elements [index2]);
        }
    }

    /** Moves one of the objects to a different position.

        This will move the object to a specified index, shuffling along
        any intervening elements as required.

        So for example, if you have the array { 0, 1, 2, 3, 4, 5 } then calling
        move (2, 4) would result in { 0, 1, 3, 4, 2, 5 }.

        @param currentIndex     the index of the object to be moved. If this isn't a
                                valid index, then nothing will be done
        @param newIndex         the index at which you'd like this object to end up. If this
                                is less than zero, it will be moved to the end of the array
    */
    void move (const size_t currentIndex, size_t newIndex) noexcept
    {
        if (currentIndex != newIndex)
        {
            if (currentIndex < numUsed)
            {
                if (newIndex >=  numUsed)
                    newIndex = numUsed - 1;

                ObjectClass* const value = data.elements [currentIndex];

                if (newIndex > currentIndex)
                {
                    std::memmove (data.elements + currentIndex,
                                  data.elements + currentIndex + 1,
                                  sizeof (ObjectClass*) * (size_t) (newIndex - currentIndex));
                }
                else
                {
                    std::memmove (data.elements + newIndex + 1,
                                  data.elements + newIndex,
                                  sizeof (ObjectClass*) * (size_t) (currentIndex - newIndex));
                }

                data.elements [newIndex] = value;
            }
        }
    }

    /** This swaps the contents of this array with those of another array.

        If you need to exchange two arrays, this is vastly quicker than using copy-by-value
        because it just swaps their internal pointers.
    */
    template <class OtherArrayType>
    void swapWith (OtherArrayType& otherArray) noexcept
    {
        data.swapWith (otherArray.data);
        std::swap (numUsed, otherArray.numUsed);
    }

    //==============================================================================
    /** Reduces the amount of storage being used by the array.

        Arrays typically allocate slightly more storage than they need, and after
        removing elements, they may have quite a lot of unused space allocated.
        This method will reduce the amount of allocated storage to a minimum.
    */
    bool minimiseStorageOverheads() noexcept
    {
        return data.shrinkToNoMoreThan (numUsed);
    }

    /** Increases the array's internal storage to hold a minimum number of elements.

        Calling this before adding a large known number of elements means that
        the array won't have to keep dynamically resizing itself as the elements
        are added, and it'll therefore be more efficient.
    */
    bool ensureStorageAllocated (const int minNumElements) noexcept
    {
        return data.ensureAllocatedSize (minNumElements);
    }

    //==============================================================================
    /** Sorts the elements in the array.

        This will use a comparator object to sort the elements into order. The object
        passed must have a method of the form:
        @code
        int compareElements (ElementType* first, ElementType* second);
        @endcode

        ..and this method must return:
          - a value of < 0 if the first comes before the second
          - a value of 0 if the two objects are equivalent
          - a value of > 0 if the second comes before the first

        To improve performance, the compareElements() method can be declared as static or const.

        @param comparator   the comparator to use for comparing elements.
        @param retainOrderOfEquivalentItems     if this is true, then items
                            which the comparator says are equivalent will be
                            kept in the order in which they currently appear
                            in the array. This is slower to perform, but may
                            be important in some cases. If it's false, a faster
                            algorithm is used, but equivalent elements may be
                            rearranged.
        @see sortArray, indexOfSorted
    */
    template <class ElementComparator>
    void sort (ElementComparator& comparator,
               bool retainOrderOfEquivalentItems = false) const noexcept
    {
        ignoreUnused (comparator); // if you pass in an object with a static compareElements() method, this
                                   // avoids getting warning messages about the parameter being unused

        sortArray (comparator, data.elements.getData(), 0, size() - 1, retainOrderOfEquivalentItems);
    }

private:
    //==============================================================================
    ArrayAllocationBase <ObjectClass*> data;
    size_t numUsed;

    void deleteAllObjects()
    {
        while (numUsed > 0)
            delete data.elements [--numUsed];
    }

    CARLA_DECLARE_NON_COPYABLE (OwnedArray)
};

}

#endif // WATER_OWNEDARRAY_H_INCLUDED
