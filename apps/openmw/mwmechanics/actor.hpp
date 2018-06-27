#ifndef OPENMW_MECHANICS_ACTOR_H
#define OPENMW_MECHANICS_ACTOR_H

#include <memory>

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

    private:
        std::unique_ptr<CharacterController> mCharacterController;
    };

}

#endif
