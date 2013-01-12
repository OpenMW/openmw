#ifndef GAME_MWMECHANICS_CHARACTER_HPP
#define GAME_MWMECHANICS_CHARACTER_HPP

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{

class CharacterController
{
    MWWorld::Ptr mPtr;

public:
    CharacterController(const MWWorld::Ptr &ptr)
      : mPtr(ptr)
    { }
};

}

#endif /* GAME_MWMECHANICS_CHARACTER_HPP */
