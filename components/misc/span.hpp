#ifndef OPENMW_COMPONENTS_MISC_SPAN_H
#define OPENMW_COMPONENTS_MISC_SPAN_H

#include <cstddef>

namespace Misc
{
    template <class T>
    class Span
    {
        public:
            constexpr Span() = default;

            constexpr Span(T* pointer, std::size_t size)
                : mPointer(pointer)
                , mSize(size)
            {}

            template <class Range>
            constexpr Span(Range& range)
                : Span(range.data(), range.size())
            {}

            constexpr T* begin() const { return mPointer; }

            constexpr T* end() const { return mPointer + mSize; }

            constexpr std::size_t size() const { return mSize; }

        private:
            T* mPointer = nullptr;
            std::size_t mSize = 0;
    };
}

#endif
