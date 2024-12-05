#ifndef OPENMW_TEST_SUITE_DETOURNAVIGATOR_GENERATE_H
#define OPENMW_TEST_SUITE_DETOURNAVIGATOR_GENERATE_H

#include <algorithm>
#include <numeric>
#include <random>
#include <type_traits>

namespace DetourNavigator
{
    namespace Tests
    {
        template <class T, class Random>
        inline auto generateValue(T& value, Random& random) -> std::enable_if_t<sizeof(T) >= 2>
        {
            using Distribution = std::conditional_t<std::is_floating_point_v<T>, std::uniform_real_distribution<T>,
                std::uniform_int_distribution<T>>;
            Distribution distribution(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            value = distribution(random);
        }

        template <class T, class Random>
        inline auto generateValue(T& value, Random& random) -> std::enable_if_t<sizeof(T) == 1>
        {
            unsigned short v;
            generateValue(v, random);
            value = static_cast<T>(v % 256);
        }

        template <class Random>
        inline void generateValue(unsigned char& value, Random& random)
        {
            unsigned short v;
            generateValue(v, random);
            value = static_cast<unsigned char>(v % 256);
        }

        template <class I, class Random>
        inline void generateRange(I begin, I end, Random& random)
        {
            std::for_each(begin, end, [&](auto& v) { generateValue(v, random); });
        }
    }
}

#endif
