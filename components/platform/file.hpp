#ifndef OPENMW_COMPONENTS_PLATFORM_FILE_HPP
#define OPENMW_COMPONENTS_PLATFORM_FILE_HPP

#include <cstdlib>
#include <string_view>

namespace Platform::File {

    enum class Handle : intptr_t
    {
        Invalid = -1
    };

    enum class SeekType
    {
        Begin,
        Current,
        End
    };

    Handle open(const char* filename);

    void close(Handle handle);

    size_t size(Handle handle);

    void seek(Handle handle, size_t Position, SeekType type = SeekType::Begin);
    size_t tell(Handle handle);

    size_t read(Handle handle, void* data, size_t size);


}

#endif // OPENMW_COMPONENTS_PLATFORM_FILE_HPP
