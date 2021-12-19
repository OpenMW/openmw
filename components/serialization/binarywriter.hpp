#ifndef OPENMW_COMPONENTS_SERIALIZATION_BINARYWRITER_H
#define OPENMW_COMPONENTS_SERIALIZATION_BINARYWRITER_H

#include <components/misc/endianness.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>

namespace Serialization
{
    struct NotEnoughSpace : std::runtime_error
    {
        NotEnoughSpace() : std::runtime_error("Not enough space") {}
    };

    struct BinaryWriter
    {
    public:
        explicit BinaryWriter(std::byte* dest, const std::byte* end)
            : mDest(dest), mEnd(end)
        {
            assert(mDest <= mEnd);
        }

        BinaryWriter(const BinaryWriter&) = delete;

        template <class Format, class T>
        void operator()(Format&& format, const T& value)
        {
            if constexpr (std::is_enum_v<T>)
                (*this)(std::forward<Format>(format), static_cast<std::underlying_type_t<T>>(value));
            else if constexpr (std::is_arithmetic_v<T>)
            {
                if (mEnd - mDest < static_cast<std::ptrdiff_t>(sizeof(T)))
                    throw NotEnoughSpace();
                writeValue(value);
            }
            else
            {
                format(*this, value);
            }
        }

        template <class Format, class T>
        auto operator()(Format&& format, const T* data, std::size_t count)
        {
            if constexpr (std::is_enum_v<T>)
                (*this)(std::forward<Format>(format), reinterpret_cast<const std::underlying_type_t<T>*>(data), count);
            else if constexpr (std::is_arithmetic_v<T>)
            {
                const std::size_t size = sizeof(T) * count;
                if (mEnd - mDest < static_cast<std::ptrdiff_t>(size))
                    throw NotEnoughSpace();
                if constexpr (Misc::IS_LITTLE_ENDIAN)
                {
                    std::memcpy(mDest, data, size);
                    mDest += size;
                }
                else
                    std::for_each(data, data + count, [&] (const T& v) { writeValue(v); });
            }
            else
            {
                format(*this, data, count);
            }
        }

    private:
        std::byte* mDest;
        const std::byte* const mEnd;

        template <class T>
        void writeValue(const T& value) noexcept
        {
            T coverted = Misc::toLittleEndian(value);
            std::memcpy(mDest, &coverted, sizeof(T));
            mDest += sizeof(T);
        }
    };
}

#endif
