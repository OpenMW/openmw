#ifndef OPENMW_TEST_SUITE_SERIALIZATION_FORMAT_H
#define OPENMW_TEST_SUITE_SERIALIZATION_FORMAT_H

#include <components/serialization/format.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>

namespace SerializationTesting
{
    struct Pod
    {
        int mInt = 42;
        double mDouble = 3.14;

        friend bool operator==(const Pod& l, const Pod& r)
        {
            const auto tuple = [](const Pod& v) { return std::tuple(v.mInt, v.mDouble); };
            return tuple(l) == tuple(r);
        }
    };

    enum Enum : std::int32_t
    {
        A,
        B,
        C,
    };

    struct Composite
    {
        short mFloatArray[3] = { 0 };
        std::vector<std::int32_t> mIntVector;
        std::vector<Enum> mEnumVector;
        std::vector<Pod> mPodVector;
        std::size_t mPodDataSize = 0;
        std::vector<Pod> mPodBuffer;
        std::size_t mCharDataSize = 0;
        std::vector<char> mCharBuffer;
    };

    template <Serialization::Mode mode>
    struct TestFormat : Serialization::Format<mode, TestFormat<mode>>
    {
        using Serialization::Format<mode, TestFormat<mode>>::operator();

        template <class Visitor, class T>
        auto operator()(Visitor&& visitor, T& value) const -> std::enable_if_t<std::is_same_v<std::decay_t<T>, Pod>>
        {
            visitor(*this, value.mInt);
            visitor(*this, value.mDouble);
        }

        template <class Visitor, class T>
        auto operator()(Visitor&& visitor, T& value) const
            -> std::enable_if_t<std::is_same_v<std::decay_t<T>, Composite>>
        {
            visitor(*this, value.mFloatArray);
            visitor(*this, value.mIntVector);
            visitor(*this, value.mEnumVector);
            visitor(*this, value.mPodVector);
            visitor(*this, value.mPodDataSize);
            if constexpr (mode == Serialization::Mode::Read)
                value.mPodBuffer.resize(value.mPodDataSize);
            visitor(*this, value.mPodBuffer.data(), value.mPodDataSize);
            visitor(*this, value.mCharDataSize);
            if constexpr (mode == Serialization::Mode::Read)
                value.mCharBuffer.resize(value.mCharDataSize);
            visitor(*this, value.mCharBuffer.data(), value.mCharDataSize);
        }
    };
}

#endif
