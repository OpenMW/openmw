#ifndef OPENMW_MECHANICS_ACTOR_H
#define OPENMW_MECHANICS_ACTOR_H

#include <memory>

#include "aistate.hpp"

namespace MWRender
{
    class Animation;
}
namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    class CharacterController;

    /// @brief Holds temporary state for an actor that will be discarded when the actor leaves the scene.
    class Actor
    {
    public:
        Actor(const MWWorld::Ptr& ptr, MWRender::Animation* animation);

        /// Notify this actor of its new base object Ptr, use when the object changed cells
        void updatePtr(const MWWorld::Ptr& newPtr);

        CharacterController* getCharacterController();

        AiState& getAiState();

    private:
        std::auto_ptr<CharacterController> mCharacterController;

        AiState mAiState;
    };

}

#endif
