#ifndef OPENMW_COMPONENTS_MISC_RNG_H
#define OPENMW_COMPONENTS_MISC_RNG_H

#include <cassert>
#include <random>

namespace Misc
{

/*
  Provides central implementation of the RNG logic
*/
class Rng
{
    std::mt19937 mGenerator;

    void seed(unsigned int seed);

public:
    Rng();

    Rng(const Rng&) = delete;

    Rng(unsigned int seed);

    float exclusiveRandom();

    float inclusiveRandom();

    int exclusiveRandom(int max);

    /// create a RNG
    static Rng sInstance;

    /// seed the RNG
    static void init(unsigned int seed = generateDefaultSeed()) { sInstance.seed(seed); }

    /// return value in range [0.0f, 1.0f)  <- note open upper range.
    static float rollProbability() { return sInstance.exclusiveRandom(); }
  
    /// return value in range [0.0f, 1.0f]  <- note closed upper range.
    static float rollClosedProbability() { return sInstance.inclusiveRandom(); }

    /// return value in range [0, max)  <- note open upper range.
    static int rollDice(int max) { return sInstance.exclusiveRandom(max); }

    /// return value in range [0, 99]
    static int roll0to99() { return sInstance.exclusiveRandom(100); }

    /// returns default seed for RNG
    static unsigned int generateDefaultSeed();
};

}

#endif
