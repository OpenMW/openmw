#ifndef OPENMW_COMPONENTS_SERIALIZATION_FORMAT_H
#define OPENMW_COMPONENTS_SERIALIZATION_FORMAT_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace Serialization
{
    enum class Mode
    {
        Read,
        Write,
    };

    template <class T>
    concept ContiguousContainer = requires (T v)
    {
        std::data(v);
        std::size(v);
    };

    template <class T>
    concept Resizeable = requires (T v)
    {
        v.resize(std::size_t{});
    };

    template <class T, std::size_t size>
    void resize(std::size_t dataSize, T(&/*value*/)[size])
    {
        if (static_cast<std::size_t>(dataSize) > size)
            throw std::runtime_error("Not enough array size");
    }

    template <class T, std::size_t size>
    void resize(std::size_t dataSize, std::array<T, size>& /*value*/)
    {
        if (static_cast<std::size_t>(dataSize) > size)
            throw std::runtime_error("Not enough std::array size");
    }

    void resize(std::size_t size, Resizeable auto& value)
    {
        value.resize(size);
    }

    template <Mode mode, class Derived>
    struct Format
    {
        template <class Visitor, class T>
        void operator()(Visitor&& visitor, T* data, std::size_t size) const
        {
            if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                visitor(self(), data, size);
            else
                std::for_each(data, data + size, [&] (auto& v) { visitor(self(), v); });
        }

        template <class Visitor, class T, std::size_t size>
        void operator()(Visitor&& visitor, T(& data)[size]) const
        {
            self()(std::forward<Visitor>(visitor), data, size);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, ContiguousContainer auto&& value) const
        {
            if constexpr (mode == Mode::Write)
                visitor(self(), static_cast<std::uint64_t>(std::size(value)));
            else
            {
                static_assert(mode == Mode::Read);
                std::uint64_t size = 0;
                visitor(self(), size);
                resize(static_cast<std::size_t>(size), value);
            }
            self()(std::forward<Visitor>(visitor), std::data(value), std::size(value));
        }

        const Derived& self() const { return static_cast<const Derived&>(*this); }
    };
}

#endif
