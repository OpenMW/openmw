#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTTEMPALLOCATOR_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTTEMPALLOCATOR_H

#include "recastallocutils.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace DetourNavigator
{
    class RecastTempAllocator
    {
    public:
        RecastTempAllocator(std::size_t capacity)
            : mStack(capacity), mTop(mStack.data()), mPrev(nullptr)
        {}

        void* alloc(std::size_t size)
        {
            std::size_t space = mStack.size() - getUsedSize();
            void* top = mTop;
            const auto itemSize = 2 * sizeof(std::size_t) + size;
            if (rcUnlikely(!std::align(sizeof(std::size_t), itemSize, top, space)))
                return nullptr;
            setTempPtrBufferType(top, BufferType_temp);
            setTempPtrPrev(top, mPrev);
            mTop = static_cast<char*>(top) + itemSize;
            mPrev = static_cast<char*>(top);
            return getTempPtrDataPtr(top);
        }

        void free(void* ptr)
        {
            if (rcUnlikely(!ptr))
                return;
            assert(BufferType_temp == getDataPtrBufferType(ptr));
            if (!mPrev || getTempDataPtrStackPtr(ptr) != mPrev)
            {
                setDataPtrBufferType(ptr, BufferType_unused);
                return;
            }
            mTop = getTempDataPtrStackPtr(ptr);
            mPrev = getTempPtrPrev(mTop);
            while (mPrev && BufferType_unused == getTempPtrBufferType(mPrev))
            {
                mTop = mPrev;
                mPrev = getTempPtrPrev(mTop);
            }
            return;
        }

    private:
        std::vector<char> mStack;
        void* mTop;
        void* mPrev;

        std::size_t getUsedSize() const
        {
            return static_cast<std::size_t>(static_cast<char*>(mTop) - mStack.data());
        }
    };
}

#endif
