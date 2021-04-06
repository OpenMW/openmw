#ifndef OPENMW_COMPONENTS_MISC_ALGORITHM_H
#define OPENMW_COMPONENTS_MISC_ALGORITHM_H

#include <iterator>
#include <type_traits>

namespace Misc
{
    template <typename Iterator, typename BinaryPredicate, typename Function>
    inline Iterator forEachUnique(Iterator begin, Iterator end, BinaryPredicate predicate, Function function)
    {
        static_assert(
            std::is_base_of_v<
                std::forward_iterator_tag,
                typename std::iterator_traits<Iterator>::iterator_category
            >
        );
        if (begin == end)
            return begin;
        function(*begin);
        auto last = begin;
        ++begin;
        while (begin != end)
        {
            if (!predicate(*begin, *last))
            {
                function(*begin);
                last = begin;
            }
            ++begin;
        }
        return begin;
    }
}

#endif
