#include "rng.hpp"

#include <chrono>
#include <random>

namespace
{
    Misc::Rng::Seed sSeed;
}

namespace Misc
{
    Rng::Seed::Seed(unsigned int seed)
    {
        mGenerator.seed(seed);
    }

    Rng::Seed& Rng::getSeed()
    {
        return sSeed;
    }

    void Rng::init(unsigned int seed)
    {
        sSeed.mGenerator.seed(seed);
    }

    float Rng::rollProbability(Seed& seed)
    {
        return std::uniform_real_distribution<float>(0, 1 - std::numeric_limits<float>::epsilon())(seed.mGenerator);
    }

    float Rng::rollClosedProbability(Seed& seed)
    {
        return std::uniform_real_distribution<float>(0, 1)(seed.mGenerator);
    }

    int Rng::rollDice(int max, Seed& seed)
    {
        return max > 0 ? std::uniform_int_distribution<int>(0, max - 1)(seed.mGenerator) : 0;
    }

    unsigned int Rng::generateDefaultSeed()
    {
        return static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    float Rng::deviate(float mean, float deviation, Seed& seed)
    {
        return std::uniform_real_distribution<float>(mean - deviation, mean + deviation)(seed.mGenerator);
    }
}
