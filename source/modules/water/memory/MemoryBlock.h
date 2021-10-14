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

#ifndef WATER_MEMORYBLOCK_H_INCLUDED
#define WATER_MEMORYBLOCK_H_INCLUDED

#include "HeapBlock.h"

namespace water {

//==============================================================================
/**
    A class to hold a resizable block of raw data.

*/
class MemoryBlock
{
public:
    //==============================================================================
    /** Create an uninitialised block with 0 size. */
    MemoryBlock() noexcept;

    /** Creates a memory block with a given initial size.

        @param initialSize          the size of block to create
        @param initialiseToZero     whether to clear the memory or just leave it uninitialised
    */
    MemoryBlock (const size_t initialSize,
                 bool initialiseToZero = false);

    /** Creates a copy of another memory block. */
    MemoryBlock (const MemoryBlock&);

    /** Creates a memory block using a copy of a block of data.

        @param dataToInitialiseFrom     some data to copy into this block
        @param sizeInBytes              how much space to use
    */
    MemoryBlock (const void* dataToInitialiseFrom, size_t sizeInBytes);

    /** Destructor. */
    ~MemoryBlock() noexcept;

    /** Copies another memory block onto this one.
        This block will be resized and copied to exactly match the other one.
    */
    MemoryBlock& operator= (const MemoryBlock&);

   #if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
    MemoryBlock (MemoryBlock&&) noexcept;
    MemoryBlock& operator= (MemoryBlock&&) noexcept;
   #endif

    //==============================================================================
    /** Compares two memory blocks.
        @returns true only if the two blocks are the same size and have identical contents.
    */
    bool operator== (const MemoryBlock& other) const noexcept;

    /** Compares two memory blocks.
        @returns true if the two blocks are different sizes or have different contents.
    */
    bool operator!= (const MemoryBlock& other) const noexcept;

    /** Returns true if the data in this MemoryBlock matches the raw bytes passed-in. */
    bool matches (const void* data, size_t dataSize) const noexcept;

    //==============================================================================
    /** Returns a void pointer to the data.

        Note that the pointer returned will probably become invalid when the
        block is resized.
    */
    void* getData() const noexcept                                  { return data; }

    /** Returns a byte from the memory block.
        This returns a reference, so you can also use it to set a byte.
    */
    template <typename Type>
    char& operator[] (const Type offset) const noexcept             { return data [offset]; }


    //==============================================================================
    /** Returns the block's current allocated size, in bytes. */
    size_t getSize() const noexcept                                 { return size; }

    /** Resizes the memory block.

        Any data that is present in both the old and new sizes will be retained.
        When enlarging the block, the new space that is allocated at the end can either be
        cleared, or left uninitialised.

        @param newSize                      the new desired size for the block
        @param initialiseNewSpaceToZero     if the block gets enlarged, this determines
                                            whether to clear the new section or just leave it
                                            uninitialised
        @see ensureSize
    */
    void setSize (const size_t newSize,
                  bool initialiseNewSpaceToZero = false);

    /** Increases the block's size only if it's smaller than a given size.

        @param minimumSize                  if the block is already bigger than this size, no action
                                            will be taken; otherwise it will be increased to this size
        @param initialiseNewSpaceToZero     if the block gets enlarged, this determines
                                            whether to clear the new section or just leave it
                                            uninitialised
        @see setSize
    */
    void ensureSize (const size_t minimumSize,
                     bool initialiseNewSpaceToZero = false);

    /** Frees all the blocks data, setting its size to 0. */
    void reset();

    //==============================================================================
    /** Fills the entire memory block with a repeated byte value.
        This is handy for clearing a block of memory to zero.
    */
    void fillWith (uint8 valueToUse) noexcept;

    /** Adds another block of data to the end of this one.
        The data pointer must not be null. This block's size will be increased accordingly.
    */
    void append (const void* data, size_t numBytes);

    /** Resizes this block to the given size and fills its contents from the supplied buffer.
        The data pointer must not be null.
    */
    void replaceWith (const void* data, size_t numBytes);

    /** Inserts some data into the block.
        The dataToInsert pointer must not be null. This block's size will be increased accordingly.
        If the insert position lies outside the valid range of the block, it will be clipped to
        within the range before being used.
    */
    void insert (const void* dataToInsert, size_t numBytesToInsert, size_t insertPosition);

    /** Chops out a section  of the block.

        This will remove a section of the memory block and close the gap around it,
        shifting any subsequent data downwards and reducing the size of the block.

        If the range specified goes beyond the size of the block, it will be clipped.
    */
    void removeSection (size_t startByte, size_t numBytesToRemove);

    //==============================================================================
    /** Copies data into this MemoryBlock from a memory address.

        @param srcData              the memory location of the data to copy into this block
        @param destinationOffset    the offset in this block at which the data being copied should begin
        @param numBytes             how much to copy in (if this goes beyond the size of the memory block,
                                    it will be clipped so not to do anything nasty)
    */
    void copyFrom (const void* srcData,
                   int destinationOffset,
                   size_t numBytes) noexcept;

    /** Copies data from this MemoryBlock to a memory address.

        @param destData         the memory location to write to
        @param sourceOffset     the offset within this block from which the copied data will be read
        @param numBytes         how much to copy (if this extends beyond the limits of the memory block,
                                zeros will be used for that portion of the data)
    */
    void copyTo (void* destData,
                 int sourceOffset,
                 size_t numBytes) const noexcept;

    //==============================================================================
    /** Exchanges the contents of this and another memory block.
        No actual copying is required for this, so it's very fast.
    */
    void swapWith (MemoryBlock& other) noexcept;

    //==============================================================================
    /** Release this object's data ownership, returning the data pointer. */
    void* release () noexcept;

    //==============================================================================
    /** Attempts to parse the contents of the block as a zero-terminated UTF8 string. */
    String toString() const;

    //==============================================================================
    /** Parses a string of hexadecimal numbers and writes this data into the memory block.

        The block will be resized to the number of valid bytes read from the string.
        Non-hex characters in the string will be ignored.

        @see String::toHexString()
    */
    void loadFromHexString (StringRef sourceHexString);

    //==============================================================================
    /** Sets a number of bits in the memory block, treating it as a long binary sequence. */
    void setBitRange (size_t bitRangeStart,
                      size_t numBits,
                      int binaryNumberToApply) noexcept;

    /** Reads a number of bits from the memory block, treating it as one long binary sequence */
    int getBitRange (size_t bitRangeStart,
                     size_t numBitsToRead) const noexcept;

private:
    //==============================================================================
    HeapBlock<char> data;
    size_t size;
};

}

#endif // WATER_MEMORYBLOCK_H_INCLUDED
