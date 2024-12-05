#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTGLOBALALLOCATOR_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTGLOBALALLOCATOR_H

#include "recasttempallocator.hpp"

#include <DetourAlloc.h>
#include <RecastAlloc.h>

#include <cstdlib>

namespace DetourNavigator
{
    class RecastGlobalAllocator
    {
    public:
        static void init() { instance(); }

        static void* recastAlloc(size_t size, rcAllocHint hint) { return alloc(size, hint == RC_ALLOC_TEMP); }

        static void* detourAlloc(size_t size, dtAllocHint hint) { return alloc(size, hint == DT_ALLOC_TEMP); }

        static void* alloc(size_t size, bool temp)
        {
            void* result = nullptr;
            if (rcLikely(temp))
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
                std::free(getPermDataPtrHeapPtr(ptr));
            }
        }

    private:
        RecastGlobalAllocator()
        {
            rcAllocSetCustom(&RecastGlobalAllocator::recastAlloc, &RecastGlobalAllocator::free);
            dtAllocSetCustom(&RecastGlobalAllocator::detourAlloc, &RecastGlobalAllocator::free);
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
            const auto ptr = std::malloc(size + sizeof(std::size_t));
            if (rcUnlikely(!ptr))
                return ptr;
            setPermPtrBufferType(ptr, BufferType_perm);
            return getPermPtrDataPtr(ptr);
        }
    };
}

#endif
