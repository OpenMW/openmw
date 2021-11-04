#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SERIALIZATION_SIZEACCUMULATOR_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SERIALIZATION_SIZEACCUMULATOR_H

#include <cstddef>
#include <type_traits>

namespace DetourNavigator::Serialization
{
    class SizeAccumulator
    {
    public:
        SizeAccumulator() = default;

        SizeAccumulator(const SizeAccumulator&) = delete;

        std::size_t value() const { return mValue; }

        template <class Format, class T>
        void operator()(Format&& format, const T& value)
        {
            if constexpr (std::is_arithmetic_v<T>)
                mValue += sizeof(T);
            else
                format(*this, value);
        }

        template <class Format, class T>
        auto operator()(Format&& format, const T* data, std::size_t count)
        {
            if constexpr (std::is_arithmetic_v<T>)
                mValue += count * sizeof(T);
            else
                format(*this, data, count);
        }

    private:
        std::size_t mValue = 0;
    };
}

#endif
