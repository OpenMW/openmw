#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTALLOCUTILS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTALLOCUTILS_H

#include <RecastAlloc.h>

#include <cstdint>

namespace DetourNavigator
{
    static_assert(sizeof(std::size_t) == sizeof(void*), "");

    enum BufferType : std::size_t
    {
        BufferType_perm,
        BufferType_temp,
        BufferType_unused,
    };

    inline BufferType* tempPtrBufferType(void* ptr)
    {
        return reinterpret_cast<BufferType*>(static_cast<std::size_t*>(ptr) + 1);
    }

    inline BufferType getTempPtrBufferType(void* ptr)
    {
        return *tempPtrBufferType(ptr);
    }

    inline void setTempPtrBufferType(void* ptr, BufferType value)
    {
        *tempPtrBufferType(ptr) = value;
    }

    inline void** tempPtrPrev(void* ptr)
    {
        return static_cast<void**>(ptr);
    }

    inline void* getTempPtrPrev(void* ptr)
    {
        return *tempPtrPrev(ptr);
    }

    inline void setTempPtrPrev(void* ptr, void* value)
    {
        *tempPtrPrev(ptr) = value;
    }

    inline void* getTempPtrDataPtr(void* ptr)
    {
        return reinterpret_cast<void*>(static_cast<std::size_t*>(ptr) + 2);
    }

    inline BufferType* dataPtrBufferType(void* dataPtr)
    {
        return reinterpret_cast<BufferType*>(static_cast<std::size_t*>(dataPtr) - 1);
    }

    inline BufferType getDataPtrBufferType(void* dataPtr)
    {
        return *dataPtrBufferType(dataPtr);
    }

    inline void setDataPtrBufferType(void* dataPtr, BufferType value)
    {
        *dataPtrBufferType(dataPtr) = value;
    }

    inline void* getTempDataPtrStackPtr(void* dataPtr)
    {
        return static_cast<std::size_t*>(dataPtr) - 2;
    }

    inline void* getPermDataPtrHeapPtr(void* dataPtr)
    {
        return static_cast<std::size_t*>(dataPtr) - 1;
    }

    inline void setPermPtrBufferType(void* ptr, BufferType value)
    {
        *static_cast<BufferType*>(ptr) = value;
    }

    inline void* getPermPtrDataPtr(void* ptr)
    {
        return static_cast<std::size_t*>(ptr) + 1;
    }

}

#endif
