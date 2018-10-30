#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHDATA_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHDATA_H

#include <DetourAlloc.h>

#include <algorithm>
#include <memory>

namespace DetourNavigator
{
    struct NavMeshDataValueDeleter
    {
        void operator ()(unsigned char* value) const
        {
            dtFree(value);
        }
    };

    using NavMeshDataValue = std::unique_ptr<unsigned char, NavMeshDataValueDeleter>;

    struct NavMeshData
    {
        NavMeshDataValue mValue;
        int mSize;

        NavMeshData() = default;

        NavMeshData(unsigned char* value, int size)
            : mValue(value)
            , mSize(size)
        {}
    };
}

#endif
