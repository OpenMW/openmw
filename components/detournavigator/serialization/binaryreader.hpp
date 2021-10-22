#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SERIALIZATION_BINARYREADER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SERIALIZATION_BINARYREADER_H

#include <cassert>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>

namespace DetourNavigator::Serialization
{
    class BinaryReader
    {
    public:
        explicit BinaryReader(const std::byte* pos, const std::byte* end)
            : mPos(pos), mEnd(end)
        {
            assert(mPos <= mEnd);
        }

        BinaryReader(const BinaryReader&) = delete;

        template <class Format, class T>
        void operator()(Format&& format, T& value)
        {
            if constexpr (std::is_arithmetic_v<T>)
            {
                if (mEnd - mPos < static_cast<std::ptrdiff_t>(sizeof(value)))
                    throw std::runtime_error("Not enough data");
                std::memcpy(&value, mPos, sizeof(value));
                mPos += sizeof(value);
            }
            else
            {
                format(*this, value);
            }
        }

        template <class Format, class T>
        auto operator()(Format&& format, T* data, std::size_t count)
        {
            if constexpr (std::is_arithmetic_v<T>)
            {
                if (mEnd - mPos < static_cast<std::ptrdiff_t>(count * sizeof(T)))
                    throw std::runtime_error("Not enough data");
                const std::size_t size = sizeof(T) * count;
                std::memcpy(data, mPos, size);
                mPos += size;
            }
            else
            {
                format(*this, data, count);
            }
        }

    private:
        const std::byte* mPos;
        const std::byte* const mEnd;
    };
}

#endif
