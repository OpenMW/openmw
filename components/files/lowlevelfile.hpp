#ifndef COMPONENTS_FILES_LOWLEVELFILE_HPP
#define COMPONENTS_FILES_LOWLEVELFILE_HPP

#include <cstdlib>
#include <memory>

#include <components/platform/file.hpp>

class LowLevelFile
{
public:
    ~LowLevelFile();

    void open(char const* filename);
    void close();

    size_t size();

    void seek(size_t Position);
    size_t tell();

    size_t read(void* data, size_t size);

private:
    Platform::File::Handle mHandle{ Platform::File::Handle::Invalid };
};

#endif
