#ifndef OPENMW_COMPONENTS_FILES_STREAMWITHBUFFER_H
#define OPENMW_COMPONENTS_FILES_STREAMWITHBUFFER_H

#include <istream>
#include <memory>

namespace Files
{
    template <class Buffer>
    class StreamWithBuffer final : public std::istream
    {
        public:
            explicit StreamWithBuffer(std::unique_ptr<Buffer>&& buffer)
                : std::istream(buffer.get())
                , mBuffer(std::move(buffer))
            {}

        private:
            std::unique_ptr<Buffer> mBuffer;
    };
}

#endif
