#pragma once
#include <cstdlib>
#include <utility>
#include <new>

//! Allocator Base class
//! subclasses must specify allocation and deallocation
class Allocator
{
    public:
        Allocator(void);
        Allocator(const Allocator&) = delete;
        virtual ~Allocator(void);

        virtual void *alloc_mem(size_t mem_size) = 0;
        virtual void dealloc_mem(void *memory) = 0;

        /**
         * High level allocator method, which return a pointer to a class
         * or struct
         * allocated with the specialized subclass strategy
         * @param ts argument(s) for the constructor of the type T
         * @return a non null pointer to a new object of type T
         * @throw std::bad_alloc is no memory could be allocated
         */
        template <typename T, typename... Ts>
        T *alloc(Ts&&... ts)
        {
            void *data = alloc_mem(sizeof(T));
            if(!data) {
                rollbackTransaction();
                throw std::bad_alloc();
            }
            append_alloc_to_memory_transaction(data);
            return new (data) T(std::forward<Ts>(ts)...);
        }

        /**
         * High level allocator method, which return a pointer to an array of
         * class or struct
         * allocated with the specialized subclass strategy
         * @param len the array length
         * @param ts argument(s) for the constructor of the type T
         * @return a non null pointer to an array of new object(s) of type T
         * @throw std::bad_alloc is no memory could be allocated
         */
        template <typename T, typename... Ts>
        T *valloc(size_t len, Ts&&... ts)
        {
            T *data = (T*)alloc_mem(len*sizeof(T));
            if(!data) {
                rollbackTransaction();
                throw std::bad_alloc();
            }
            append_alloc_to_memory_transaction(data);
            for(unsigned i=0; i<len; ++i)
                new ((void*)&data[i]) T(std::forward<Ts>(ts)...);

            return data;
        }

        template <typename T>
        void dealloc(T*&t)
        {
            if(t) {
                t->~T();
                dealloc_mem((void*)t);
                t = nullptr;
            }
        }

        //Destructor Free Version
        template <typename T>
        void devalloc(T*&t)
        {
            if(t) {
                dealloc_mem(t);
                t = nullptr;
            }
        }

        template <typename T>
        void devalloc(size_t elms, T*&t)
        {
            if(t) {
                for(size_t i=0; i<elms; ++i)
                    (t+i)->~T();

                dealloc_mem(t);
                t = nullptr;
            }
        }

    void beginTransaction();
    void endTransaction();

    virtual void addMemory(void *, size_t mem_size) = 0;

    //Return true if the current pool cannot allocate n chunks of chunk_size
    virtual bool lowMemory(unsigned n, size_t chunk_size) const = 0;
    bool memFree(void *pool) const;

    //returns number of pools
    int memPools() const;

    int freePools() const;

    unsigned long long totalAlloced() const;

    struct AllocatorImpl *impl;

private:
    const static size_t max_transaction_length = 256;

    void* transaction_alloc_content[max_transaction_length];
    size_t transaction_alloc_index;
    bool transaction_active;

    void rollbackTransaction();

    /**
     * Append memory block to the list of memory blocks allocated during this
     * transaction
     * @param new_memory pointer to the memory pointer to freshly allocated
     */
    void append_alloc_to_memory_transaction(void *new_memory) {
        if (transaction_active) {
            if (transaction_alloc_index < max_transaction_length) {
                transaction_alloc_content[transaction_alloc_index++] = new_memory;
            }
            // TODO add log about transaction too long and memory transaction
            // safety net being disabled
        }
    }

};

//! the allocator for normal use
class AllocatorClass : public Allocator
{
    public:
        void *alloc_mem(size_t mem_size);
        void dealloc_mem(void *memory);
        void addMemory(void *, size_t mem_size);
        bool lowMemory(unsigned n, size_t chunk_size) const;
        using Allocator::Allocator;
};
typedef AllocatorClass Alloc;

//! the dummy allocator, which does not allow any allocation
class DummyAllocator : public Allocator
{
    void not_allowed() const {
        throw "(de)allocation forbidden"; // TODO: std exception
    }
public:
    void *alloc_mem(size_t ) { return not_allowed(), nullptr; }
    void dealloc_mem(void* ) { not_allowed(); } // TODO: more functions?
    void addMemory(void *, size_t ) { not_allowed(); }
    bool lowMemory(unsigned , size_t ) const { return not_allowed(), true; }
    using Allocator::Allocator;
};

extern DummyAllocator DummyAlloc;

/**
 * General notes on Memory Allocation Within ZynAddSubFX
 * -----------------------------------------------------
 *
 *  - Parameter Objects Are never allocated within the realtime thread
 *  - Effects, notes and note subcomponents must be allocated with an allocator
 *  - 5M Chunks are used to give the allocator the memory it wants
 *  - If there are 3 chunks that are unused then 1 will be deallocated
 *  - The system will request more allocated space if 5x 1MB chunks cannot be
 *    allocated at any given time (this is likely huge overkill, but if this is
 *    satisfied, then a lot of note spamming would be needed to run out of
 *    space)
 *
 *   - Things will get a bit weird around the effects due to how pointer swaps
 *     occur
 *     * When a new Part instance is provided it may or may not come with some
 *       instrument effects
 *     * Merging blocks is an option, but one that is not going to likely be
 *       implmented too soon, thus all effects need to be reallocated when the
 *       pointer swap occurs
 *     * The old effect is extracted from the manager
 *     * A new one is constructed with a deep copy
 *     * The old one is returned to middleware for deallocation
 */
