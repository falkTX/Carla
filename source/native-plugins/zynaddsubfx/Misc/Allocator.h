#pragma once
#include <cstdlib>
#include <utility>

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

        template <typename T, typename... Ts>
        T *alloc(Ts&&... ts)
        {
            void *data = alloc_mem(sizeof(T));
            if(!data)
                return nullptr;
            return new (data) T(std::forward<Ts>(ts)...);
        }

        template <typename T, typename... Ts>
        T *valloc(size_t len, Ts&&... ts)
        {
            T *data = (T*)alloc_mem(len*sizeof(T));
            if(!data)
                return nullptr;
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

    virtual void addMemory(void *, size_t mem_size) = 0;

    //Return true if the current pool cannot allocate n chunks of chunk_size
    virtual bool lowMemory(unsigned n, size_t chunk_size) const = 0;
    bool memFree(void *pool) const;

    //returns number of pools
    int memPools() const;

    int freePools() const;

    unsigned long long totalAlloced() const;

    struct AllocatorImpl *impl;
};

//! the allocator for normal use
class AllocatorClass : public Allocator
{
    void *alloc_mem(size_t mem_size);
    void dealloc_mem(void *memory);
    void addMemory(void *, size_t mem_size);
    bool lowMemory(unsigned n, size_t chunk_size) const;
    using Allocator::Allocator;
};

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
