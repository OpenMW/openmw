#ifndef OPENMW_COMPONENTS_SERIALIZATION_FORMAT_H
#define OPENMW_COMPONENTS_SERIALIZATION_FORMAT_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace Serialization
{
    enum class Mode
    {
        Read,
        Write,
    };

    template <class>
    struct IsContiguousContainer : std::false_type {};

    template <class ... Args>
    struct IsContiguousContainer<std::vector<Args ...>> : std::true_type {};

    template <class T, std::size_t n>
    struct IsContiguousContainer<std::array<T, n>> : std::true_type {};

    template <class T>
    constexpr bool isContiguousContainer = IsContiguousContainer<std::decay_t<T>>::value;

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

        template <class Visitor, class T>
        auto operator()(Visitor&& visitor, T&& value) const
            -> std::enable_if_t<isContiguousContainer<T>>
        {
            if constexpr (mode == Mode::Write)
                visitor(self(), value.size());
            else
            {
                static_assert(mode == Mode::Read);
                std::size_t size = 0;
                visitor(self(), size);
                value.resize(size);
            }
            self()(std::forward<Visitor>(visitor), value.data(), value.size());
        }

        const Derived& self() const { return static_cast<const Derived&>(*this); }
    };
}

#endif
