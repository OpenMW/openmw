#include "rng.hpp"

#include <chrono>
#include <random>
#include <sstream>

namespace Misc::Rng
{
    static Generator sGenerator;

    Generator& getGenerator()
    {
        return sGenerator;
    }

    std::string serialize(const Generator& prng)
    {
        std::stringstream ss;
        ss << prng;

        return ss.str();
    }

    void deserialize(std::string_view data, Generator& prng)
    {
        std::stringstream ss;
        ss << data;

        ss.seekg(0);
        ss >> prng;
    }

    unsigned int generateDefaultSeed()
    {
        auto res = static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        return res;
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

    float deviate(float mean, float deviation, Generator& prng)
    {
        return std::uniform_real_distribution<float>(mean - deviation, mean + deviation)(prng);
    }

}
