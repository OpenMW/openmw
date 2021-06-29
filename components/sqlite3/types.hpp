#ifndef OPENMW_COMPONENTS_SQLITE3_TYPES_H
#define OPENMW_COMPONENTS_SQLITE3_TYPES_H

#include <cstddef>

namespace Sqlite3
{
    struct ConstBlob
    {
        const char* mData;
        int mSize;
    };
}

#endif
