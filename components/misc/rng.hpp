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
    class Seed
    {
        std::mt19937 mGenerator;
    public:
        Seed() = default;
        Seed(const Seed&) = delete;
        Seed(unsigned int seed);
        friend class Rng;
    };

    static Seed& getSeed();

    /// seed the RNG
    static void init(unsigned int seed = generateDefaultSeed());

    /// return value in range [0.0f, 1.0f)  <- note open upper range.
    static float rollProbability(Seed& seed = getSeed());
  
    /// return value in range [0.0f, 1.0f]  <- note closed upper range.
    static float rollClosedProbability(Seed& seed = getSeed());

    /// return value in range [0, max)  <- note open upper range.
    static int rollDice(int max, Seed& seed = getSeed());

    /// return value in range [0, 99]
    static int roll0to99(Seed& seed = getSeed()) { return rollDice(100, seed); }

    /// returns default seed for RNG
    static unsigned int generateDefaultSeed();

    static float deviate(float mean, float deviation, Seed& seed = getSeed());
};

}

#endif
