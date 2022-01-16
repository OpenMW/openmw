#ifndef OPENMW_COMPONENTS_SERIALIZATION_BINARYREADER_H
#define OPENMW_COMPONENTS_SERIALIZATION_BINARYREADER_H

#include <components/misc/endianness.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>

namespace Serialization
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
            if constexpr (std::is_enum_v<T>)
                (*this)(std::forward<Format>(format), static_cast<std::underlying_type_t<T>&>(value));
            else if constexpr (std::is_arithmetic_v<T>)
            {
                if (mEnd - mPos < static_cast<std::ptrdiff_t>(sizeof(T)))
                    throw std::runtime_error("Not enough data");
                std::memcpy(&value, mPos, sizeof(T));
                mPos += sizeof(T);
                value = Misc::toLittleEndian(value);
            }
            else
            {
                format(*this, value);
            }
        }

        template <class Format, class T>
        auto operator()(Format&& format, T* data, std::size_t count)
        {
            if constexpr (std::is_enum_v<T>)
                (*this)(std::forward<Format>(format), reinterpret_cast<std::underlying_type_t<T>*>(data), count);
            else if constexpr (std::is_arithmetic_v<T>)
            {
                const std::size_t size = sizeof(T) * count;
                if (mEnd - mPos < static_cast<std::ptrdiff_t>(size))
                    throw std::runtime_error("Not enough data");
                std::memcpy(data, mPos, size);
                mPos += size;
                if constexpr (!Misc::IS_LITTLE_ENDIAN)
                    std::for_each(data, data + count, [&] (T& v) { v = Misc::fromLittleEndian(v); });
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
