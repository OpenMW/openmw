#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTGLOBALALLOCATOR_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTGLOBALALLOCATOR_H

#include "recasttempallocator.hpp"

namespace DetourNavigator
{
    class RecastGlobalAllocator
    {
    public:
        static void init()
        {
            instance();
        }

        static void* alloc(size_t size, rcAllocHint hint)
        {
            void* result = nullptr;
            if (rcLikely(hint == RC_ALLOC_TEMP))
                result = tempAllocator().alloc(size);
            if (rcUnlikely(!result))
                result = allocPerm(size);
            return result;
        }

        static void free(void* ptr)
        {
            if (rcUnlikely(!ptr))
                return;
            if (rcLikely(BufferType_temp == getDataPtrBufferType(ptr)))
                tempAllocator().free(ptr);
            else
            {
                assert(BufferType_perm == getDataPtrBufferType(ptr));
                ::free(getPermDataPtrHeapPtr(ptr));
            }
        }

    private:
        RecastGlobalAllocator()
        {
            rcAllocSetCustom(&RecastGlobalAllocator::alloc, &RecastGlobalAllocator::free);
        }

        static RecastGlobalAllocator& instance()
        {
            static RecastGlobalAllocator value;
            return value;
        }

        static RecastTempAllocator& tempAllocator()
        {
            static thread_local RecastTempAllocator value(1024ul * 1024ul);
            return value;
        }

        static void* allocPerm(size_t size)
        {
            const auto ptr = ::malloc(size + sizeof(std::size_t));
            if (rcUnlikely(!ptr))
                return ptr;
            setPermPtrBufferType(ptr, BufferType_perm);
            return getPermPtrDataPtr(ptr);
        }
    };
}

#endif
