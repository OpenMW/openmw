#ifndef OPENMW_COMPONENTS_MISC_RNG_H
#define OPENMW_COMPONENTS_MISC_RNG_H

#include <cassert>
#include <random>

/*
  Provides central implementation of the RNG logic
*/
namespace Misc::Rng
{

    using Generator = std::mt19937;

    Generator& getGenerator();

    /// returns default seed for RNG
    unsigned int generateDefaultSeed();

    /// seed the RNG
    void init(unsigned int seed = generateDefaultSeed());

    /// return value in range [0.0f, 1.0f)  <- note open upper range.
    float rollProbability(Generator& prng = getGenerator());
  
    /// return value in range [0.0f, 1.0f]  <- note closed upper range.
    float rollClosedProbability(Generator& prng = getGenerator());

    /// return value in range [0, max)  <- note open upper range.
    int rollDice(int max, Generator& prng = getGenerator());

    /// return value in range [0, 99]
    inline int roll0to99(Generator& prng = getGenerator()) { return rollDice(100, prng); }

    float deviate(float mean, float deviation, Generator& prng = getGenerator());

}

#endif
