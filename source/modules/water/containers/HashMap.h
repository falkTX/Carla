/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>

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

#ifndef WATER_HASHMAP_H_INCLUDED
#define WATER_HASHMAP_H_INCLUDED

#include "Array.h"
#include "../containers/Variant.h"
#include "../text/String.h"

#include "CarlaJuceUtils.hpp"

namespace water {

//==============================================================================
/**
    A simple class to generate hash functions for some primitive types, intended for
    use with the HashMap class.
    @see HashMap
*/
struct DefaultHashFunctions
{
    /** Generates a simple hash from an integer. */
    int generateHash (const int key, const int upperLimit) const noexcept        { return std::abs (key) % upperLimit; }
    /** Generates a simple hash from an int64. */
    int generateHash (const int64 key, const int upperLimit) const noexcept      { return std::abs ((int) key) % upperLimit; }
    /** Generates a simple hash from a string. */
    int generateHash (const String& key, const int upperLimit) const noexcept    { return (int) (((uint32) key.hashCode()) % (uint32) upperLimit); }
    /** Generates a simple hash from a variant. */
    int generateHash (const var& key, const int upperLimit) const noexcept       { return generateHash (key.toString(), upperLimit); }
    /** Generates a simple hash from a void ptr. */
    int generateHash (const void* key, const int upperLimit) const noexcept      { return (int)(((pointer_sized_uint) key) % ((pointer_sized_uint) upperLimit)); }
};


//==============================================================================
/**
    Holds a set of mappings between some key/value pairs.

    The types of the key and value objects are set as template parameters.
    You can also specify a class to supply a hash function that converts a key value
    into an hashed integer. This class must have the form:

    @code
    struct MyHashGenerator
    {
        int generateHash (MyKeyType key, int upperLimit) const
        {
            // The function must return a value 0 <= x < upperLimit
            return someFunctionOfMyKeyType (key) % upperLimit;
        }
    };
    @endcode

    Like the Array class, the key and value types are expected to be copy-by-value
    types, so if you define them to be pointer types, this class won't delete the
    objects that they point to.

    If you don't supply a class for the HashFunctionType template parameter, the
    default one provides some simple mappings for strings and ints.

    @code
    HashMap<int, String> hash;
    hash.set (1, "item1");
    hash.set (2, "item2");

    DBG (hash [1]); // prints "item1"
    DBG (hash [2]); // prints "item2"

    // This iterates the map, printing all of its key -> value pairs..
    for (HashMap<int, String>::Iterator i (hash); i.next();)
        DBG (i.getKey() << " -> " << i.getValue());
    @endcode

    @tparam HashFunctionType The class of hash function, which must be copy-constructible.
    @see CriticalSection, DefaultHashFunctions, NamedValueSet, SortedSet
*/
template <typename KeyType,
          typename ValueType,
          class HashFunctionType = DefaultHashFunctions>
class HashMap
{
private:
    typedef PARAMETER_TYPE (KeyType)   KeyTypeParameter;
    typedef PARAMETER_TYPE (ValueType) ValueTypeParameter;

public:
    //==============================================================================
    /** Creates an empty hash-map.

        @param numberOfSlots Specifies the number of hash entries the map will use. This will be
                            the "upperLimit" parameter that is passed to your generateHash()
                            function. The number of hash slots will grow automatically if necessary,
                            or it can be remapped manually using remapTable().
        @param hashFunction An instance of HashFunctionType, which will be copied and
                            stored to use with the HashMap. This parameter can be omitted
                            if HashFunctionType has a default constructor.
    */
    explicit HashMap (int numberOfSlots = defaultHashTableSize,
                      HashFunctionType hashFunction = HashFunctionType())
       : hashFunctionToUse (hashFunction), totalNumItems (0)
    {
        hashSlots.insertMultiple (0, nullptr, numberOfSlots);
    }

    /** Destructor. */
    ~HashMap()
    {
        clear();
    }

    //==============================================================================
    /** Removes all values from the map.
        Note that this will clear the content, but won't affect the number of slots (see
        remapTable and getNumSlots).
    */
    void clear()
    {
        for (int i = hashSlots.size(); --i >= 0;)
        {
            HashEntry* h = hashSlots.getUnchecked(i);

            while (h != nullptr)
            {
                const ScopedPointer<HashEntry> deleter (h);
                h = h->nextEntry;
            }

            hashSlots.set (i, nullptr);
        }

        totalNumItems = 0;
    }

    //==============================================================================
    /** Returns the current number of items in the map. */
    inline int size() const noexcept
    {
        return totalNumItems;
    }

    /** Returns the value corresponding to a given key.
        If the map doesn't contain the key, a default instance of the value type is returned.
        @param keyToLookFor    the key of the item being requested
    */
    inline ValueType operator[] (KeyTypeParameter keyToLookFor) const
    {
        for (const HashEntry* entry = hashSlots.getUnchecked (generateHashFor (keyToLookFor)); entry != nullptr; entry = entry->nextEntry)
            if (entry->key == keyToLookFor)
                return entry->value;

        return ValueType();
    }

    //==============================================================================
    /** Returns true if the map contains an item with the specied key. */
    bool contains (KeyTypeParameter keyToLookFor) const
    {
        for (const HashEntry* entry = hashSlots.getUnchecked (generateHashFor (keyToLookFor)); entry != nullptr; entry = entry->nextEntry)
            if (entry->key == keyToLookFor)
                return true;

        return false;
    }

    /** Returns true if the hash contains at least one occurrence of a given value. */
    bool containsValue (ValueTypeParameter valueToLookFor) const
    {
        for (int i = getNumSlots(); --i >= 0;)
            for (const HashEntry* entry = hashSlots.getUnchecked(i); entry != nullptr; entry = entry->nextEntry)
                if (entry->value == valueToLookFor)
                    return true;

        return false;
    }

    //==============================================================================
    /** Adds or replaces an element in the hash-map.
        If there's already an item with the given key, this will replace its value. Otherwise, a new item
        will be added to the map.
    */
    void set (KeyTypeParameter newKey, ValueTypeParameter newValue)
    {
        const int hashIndex = generateHashFor (newKey);

        HashEntry* const firstEntry = hashSlots.getUnchecked (hashIndex);

        for (HashEntry* entry = firstEntry; entry != nullptr; entry = entry->nextEntry)
        {
            if (entry->key == newKey)
            {
                entry->value = newValue;
                return;
            }
        }

        hashSlots.set (hashIndex, new HashEntry (newKey, newValue, firstEntry));
        ++totalNumItems;

        if (totalNumItems > (getNumSlots() * 3) / 2)
            remapTable (getNumSlots() * 2);
    }

    /** Removes an item with the given key. */
    void remove (KeyTypeParameter keyToRemove)
    {
        const int hashIndex = generateHashFor (keyToRemove);
        HashEntry* entry = hashSlots.getUnchecked (hashIndex);
        HashEntry* previous = nullptr;

        while (entry != nullptr)
        {
            if (entry->key == keyToRemove)
            {
                const ScopedPointer<HashEntry> deleter (entry);

                entry = entry->nextEntry;

                if (previous != nullptr)
                    previous->nextEntry = entry;
                else
                    hashSlots.set (hashIndex, entry);

                --totalNumItems;
            }
            else
            {
                previous = entry;
                entry = entry->nextEntry;
            }
        }
    }

    /** Removes all items with the given value. */
    void removeValue (ValueTypeParameter valueToRemove)
    {
        for (int i = getNumSlots(); --i >= 0;)
        {
            HashEntry* entry = hashSlots.getUnchecked(i);
            HashEntry* previous = nullptr;

            while (entry != nullptr)
            {
                if (entry->value == valueToRemove)
                {
                    const ScopedPointer<HashEntry> deleter (entry);

                    entry = entry->nextEntry;

                    if (previous != nullptr)
                        previous->nextEntry = entry;
                    else
                        hashSlots.set (i, entry);

                    --totalNumItems;
                }
                else
                {
                    previous = entry;
                    entry = entry->nextEntry;
                }
            }
        }
    }

    /** Remaps the hash-map to use a different number of slots for its hash function.
        Each slot corresponds to a single hash-code, and each one can contain multiple items.
        @see getNumSlots()
    */
    void remapTable (int newNumberOfSlots)
    {
        HashMap newTable (newNumberOfSlots);

        for (int i = getNumSlots(); --i >= 0;)
            for (const HashEntry* entry = hashSlots.getUnchecked(i); entry != nullptr; entry = entry->nextEntry)
                newTable.set (entry->key, entry->value);

        swapWith (newTable);
    }

    /** Returns the number of slots which are available for hashing.
        Each slot corresponds to a single hash-code, and each one can contain multiple items.
        @see getNumSlots()
    */
    inline int getNumSlots() const noexcept
    {
        return hashSlots.size();
    }

    //==============================================================================
    /** Efficiently swaps the contents of two hash-maps. */
    template <class OtherHashMapType>
    void swapWith (OtherHashMapType& otherHashMap) noexcept
    {
        hashSlots.swapWith (otherHashMap.hashSlots);
        std::swap (totalNumItems, otherHashMap.totalNumItems);
    }

private:
    //==============================================================================
    class HashEntry
    {
    public:
        HashEntry (KeyTypeParameter k, ValueTypeParameter val, HashEntry* const next)
            : key (k), value (val), nextEntry (next)
        {}

        const KeyType key;
        ValueType value;
        HashEntry* nextEntry;

        CARLA_DECLARE_NON_COPY_CLASS (HashEntry)
    };

public:
    //==============================================================================
    /** Iterates over the items in a HashMap.

        To use it, repeatedly call next() until it returns false, e.g.
        @code
        HashMap <String, String> myMap;

        HashMap<String, String>::Iterator i (myMap);

        while (i.next())
        {
            DBG (i.getKey() << " -> " << i.getValue());
        }
        @endcode

        The order in which items are iterated bears no resemblence to the order in which
        they were originally added!

        Obviously as soon as you call any non-const methods on the original hash-map, any
        iterators that were created beforehand will cease to be valid, and should not be used.

        @see HashMap
    */
    struct Iterator
    {
        Iterator (const HashMap& hashMapToIterate) noexcept
            : hashMap (hashMapToIterate), entry (nullptr), index (0)
        {}

        Iterator (const Iterator& other) noexcept
            : hashMap (other.hashMap), entry (other.entry), index (other.index)
        {}

        /** Moves to the next item, if one is available.
            When this returns true, you can get the item's key and value using getKey() and
            getValue(). If it returns false, the iteration has finished and you should stop.
        */
        bool next() noexcept
        {
            if (entry != nullptr)
                entry = entry->nextEntry;

            while (entry == nullptr)
            {
                if (index >= hashMap.getNumSlots())
                    return false;

                entry = hashMap.hashSlots.getUnchecked (index++);
            }

            return true;
        }

        /** Returns the current item's key.
            This should only be called when a call to next() has just returned true.
        */
        KeyType getKey() const
        {
            return entry != nullptr ? entry->key : KeyType();
        }

        /** Returns the current item's value.
            This should only be called when a call to next() has just returned true.
        */
        ValueType getValue() const
        {
            return entry != nullptr ? entry->value : ValueType();
        }

        /** Resets the iterator to its starting position. */
        void reset() noexcept
        {
            entry = nullptr;
            index = 0;
        }

        Iterator& operator++() noexcept                         { next(); return *this; }
        ValueType operator*() const                             { return getValue(); }
        bool operator!= (const Iterator& other) const noexcept  { return entry != other.entry || index != other.index; }
        void resetToEnd() noexcept                              { index = hashMap.getNumSlots(); }

    private:
        //==============================================================================
        const HashMap& hashMap;
        HashEntry* entry;
        int index;

        CARLA_LEAK_DETECTOR (Iterator)
    };

    /** Returns a start iterator for the values in this tree. */
    Iterator begin() const noexcept             { Iterator i (*this); i.next(); return i; }

    /** Returns an end iterator for the values in this tree. */
    Iterator end() const noexcept               { Iterator i (*this); i.resetToEnd(); return i; }

private:
    //==============================================================================
    enum { defaultHashTableSize = 101 };
    friend struct Iterator;

    HashFunctionType hashFunctionToUse;
    Array<HashEntry*> hashSlots;
    int totalNumItems;

    int generateHashFor (KeyTypeParameter key) const
    {
        const int hash = hashFunctionToUse.generateHash (key, getNumSlots());
        wassert (isPositiveAndBelow (hash, getNumSlots())); // your hash function is generating out-of-range numbers!
        return hash;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HashMap)
};

}

#endif   // WATER_HASHMAP_H_INCLUDED
