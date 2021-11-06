#ifndef OPENMW_COMPONENTS_MISC_ALGORITHM_H
#define OPENMW_COMPONENTS_MISC_ALGORITHM_H

#include <iterator>
#include <type_traits>

#include "stringops.hpp"

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

    /// Performs a binary search on a sorted container for a string that 'key' starts with
    template<typename Iterator, typename T>
    static Iterator partialBinarySearch(Iterator begin, Iterator end, const T& key)
    {
        const Iterator notFound = end;

        while(begin < end)
        {
            const Iterator middle = begin + (std::distance(begin, end) / 2);

            int comp = Misc::StringUtils::ciCompareLen((*middle), key, (*middle).size());

            if(comp == 0)
                return middle;
            else if(comp > 0)
                end = middle;
            else
                begin = middle + 1;
        }

        return notFound;
    }

}

#endif
