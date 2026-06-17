#ifndef OPENMW_COMPONENTS_MISC_RNG_H
#define OPENMW_COMPONENTS_MISC_RNG_H

#include <cassert>
#include <concepts>
#include <random>
#include <string_view>

/*
  Provides central implementation of the RNG logic
*/
namespace Misc::Rng
{
    /// The use of a rather minimalistic prng is preferred to avoid saving a lot of state in the save game.
    using Generator = std::minstd_rand;

    Generator& getGenerator();

    std::string serialize(const Generator& prng);
    void deserialize(std::string_view data, Generator& prng);

    /// returns default seed for RNG
    unsigned int generateDefaultSeed();

    /// seed the RNG
    void init(unsigned int seed = generateDefaultSeed());

    /// return value in range [0.0f, 1.0f)  <- note open upper range.
    float rollProbability(Generator& prng = getGenerator());

    /// return value in range [0.0f, 1.0f]  <- note closed upper range.
    float rollClosedProbability(Generator& prng = getGenerator());

    /// return value in range [0, max)  <- note open upper range.
    template <std::integral T>
    inline T rollDice(T max, Generator& prng = getGenerator())
    {
        return max > 0 ? std::uniform_int_distribution<T>(0, max - 1)(prng) : 0;
    }

    /// return value in range [0, 99]
    inline int roll0to99(Generator& prng = getGenerator())
    {
        return rollDice(100, prng);
    }

    float deviate(float mean, float deviation, Generator& prng = getGenerator());
}

#endif
