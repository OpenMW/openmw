#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SERIALIZATION_BINARYWRITER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SERIALIZATION_BINARYWRITER_H

#include <cassert>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>

namespace DetourNavigator::Serialization
{
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
            if constexpr (std::is_arithmetic_v<T>)
            {
                if (mEnd - mDest < static_cast<std::ptrdiff_t>(sizeof(value)))
                    throw std::runtime_error("Not enough space");
                std::memcpy(mDest, &value, sizeof(value));
                mDest += sizeof(value);
            }
            else
            {
                format(*this, value);
            }
        }

        template <class Format, class T>
        auto operator()(Format&& format, const T* data, std::size_t count)
        {
            if constexpr (std::is_arithmetic_v<T>)
            {
                const std::size_t size = sizeof(T) * count;
                if (mEnd - mDest < static_cast<std::ptrdiff_t>(size))
                    throw std::runtime_error("Not enough space");
                std::memcpy(mDest, data, size);
                mDest += size;
            }
            else
            {
                format(*this, data, count);
            }
        }

    private:
        std::byte* mDest;
        const std::byte* const mEnd;
    };
}

#endif
