#include "actor.hpp"

//#include "character.hpp"

namespace MWMechanics
{

    Actor::Actor(const MWWorld::Ptr &ptr, MWRender::Animation *animation)
    {
        //mCharacterController.reset(new CharacterController(ptr, animation));
    }

    void Actor::updatePtr(const MWWorld::Ptr &newPtr)
    {
        //mCharacterController->updatePtr(newPtr);
    }

    CharacterController* Actor::getCharacterController()
    {
        return 0;//mCharacterController.get();
    }

    AiState& Actor::getAiState()
    {
        return mAiState;
    }

}
