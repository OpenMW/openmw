#include "rng.hpp"
#include <cstdlib>
#include <ctime>

namespace OEngine {
namespace Misc {

    void Rng::init()
    {
        std::srand(static_cast<unsigned int>(std::time(NULL)));
    }

    float Rng::rollProbability()
    {
        return static_cast<float>(std::rand() / (static_cast<double>(RAND_MAX)+1.0));
    }

    float Rng::rollClosedProbability()
    {
        return static_cast<float>(std::rand() / static_cast<double>(RAND_MAX));
    }

    int Rng::rollDice(int max)
    {
        return static_cast<int>((std::rand() / (static_cast<double>(RAND_MAX)+1.0)) * (max));
    }

}
}
