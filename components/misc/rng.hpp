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
public:

    /// create a RNG
    static std::mt19937 generator;

    /// seed the RNG
    static void init(unsigned int seed = generateDefaultSeed());

    /// return value in range [0.0f, 1.0f)  <- note open upper range.
    static float rollProbability();
  
    /// return value in range [0.0f, 1.0f]  <- note closed upper range.
    static float rollClosedProbability();

    /// return value in range [0, max)  <- note open upper range.
    static int rollDice(int max);

    /// return value in range [0, 99]
    static int roll0to99() { return rollDice(100); }

    /// returns default seed for RNG
    static unsigned int generateDefaultSeed();
};

}

#endif
