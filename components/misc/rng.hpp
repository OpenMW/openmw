#ifndef OPENMW_COMPONENTS_MISC_RNG_H
#define OPENMW_COMPONENTS_MISC_RNG_H

#include <cassert>
#include <random>

/*
  Provides central implementation of the RNG logic
*/
namespace Misc::Rng
{
    class Generator
    {
        uint32_t mState{};

    public:
        using result_type = uint32_t;

        constexpr Generator() = default;
        constexpr Generator(result_type seed) : mState{ seed } {}
        constexpr result_type operator()() noexcept
        {
            mState = (214013 * mState + 2531011);
            return (mState >> 16) & max();
        }

        static constexpr result_type min() noexcept
        {
            return 0u;
        }

        static constexpr result_type max() noexcept
        {
            return 0x7FFFu;
        }        
        
        void seed(result_type val) noexcept
        {
            mState = val;
        }

        uint32_t getSeed() const noexcept
        {
            return mState;
        }
    };

    Generator& getGenerator();

    /// returns default seed for RNG
    unsigned int generateDefaultSeed();

    /// seed the RNG
    void init(unsigned int seed = generateDefaultSeed());

    /// return value in range [0.0f, 1.0f)  <- note open upper range.
    float rollProbability();
    float rollProbability(Generator& prng);
  
    /// return value in range [0.0f, 1.0f]  <- note closed upper range.
    float rollClosedProbability();
    float rollClosedProbability(Generator& prng);

    /// return value in range [0, max)  <- note open upper range.
    int rollDice(int max);
    int rollDice(int max, Generator& prng);

    /// return value in range [0, 99]
    inline int roll0to99(Generator& prng) { return rollDice(100, prng); }
    inline int roll0to99() { return rollDice(100); }

    float deviate(float mean, float deviation);
    float deviate(float mean, float deviation, Generator& prng);
}

#endif
