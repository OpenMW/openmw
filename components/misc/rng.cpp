#include "rng.hpp"

#include <chrono>
#include <random>

namespace Misc
{
    Rng Rng::sInstance{};

    Rng::Rng() {}

    Rng::Rng(unsigned int seed)
    {
        mGenerator.seed(seed);
    }

    void Rng::seed(unsigned int seed)
    {
        mGenerator.seed(seed);
    }

    float Rng::exclusiveRandom()
    {
        return std::uniform_real_distribution<float>(0, 1 - std::numeric_limits<float>::epsilon())(mGenerator);
    }

    float Rng::inclusiveRandom()
    {
        return std::uniform_real_distribution<float>(0, 1)(mGenerator);
    }

    int Rng::exclusiveRandom(int max)
    {
        return max > 0 ? std::uniform_int_distribution<int>(0, max - 1)(mGenerator) : 0;
    }

    unsigned int Rng::generateDefaultSeed()
    {
        return static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }
}
