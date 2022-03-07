#include "rng.hpp"

#include <chrono>
#include <random>

namespace Misc::Rng
{
    static Generator sGenerator;

    Generator& getGenerator()
    {
        return sGenerator;
    }

    void init(unsigned int seed)
    {
        sGenerator.seed(seed);
    }

    float rollProbability(Generator& prng)
    {
        return std::uniform_real_distribution<float>(0, 1 - std::numeric_limits<float>::epsilon())(prng);
    }

    float rollClosedProbability(Generator& prng)
    {
        return std::uniform_real_distribution<float>(0, 1)(prng);
    }

    int rollDice(int max, Generator& prng)
    {
        return max > 0 ? std::uniform_int_distribution<int>(0, max - 1)(prng) : 0;
    }

    unsigned int generateDefaultSeed()
    {
        return static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    float deviate(float mean, float deviation, Generator& prng)
    {
        return std::uniform_real_distribution<float>(mean - deviation, mean + deviation)(prng);
    }
}
