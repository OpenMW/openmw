#ifndef GAME_MWMECHANICS_AIAVOIDDOOR_H
#define GAME_MWMECHANICS_AIAVOIDDOOR_H

#include "typedaipackage.hpp"

#include <string>

#include <components/esm/defs.hpp>

#include "../mwworld/class.hpp"

#include "pathfinding.hpp"

namespace MWMechanics
{
    /// \brief AiPackage to have an actor avoid an opening door
    /** The AI will retreat from the door until it has finished opening, walked far away from it, or one second has passed, in an attempt to avoid it
    **/
    class AiAvoidDoor final : public TypedAiPackage<AiAvoidDoor>
    {
        public:
            /// Avoid door until the door is fully open
            AiAvoidDoor(const MWWorld::ConstPtr& doorPtr);

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) final;

            static constexpr TypeId getTypeId() { return TypeIdAvoidDoor; }

            static constexpr unsigned int defaultPriority() { return 2; }

            static constexpr bool defaultCanCancel() { return false; }

            static constexpr bool defaultShouldCancelPreviousAi() { return false; }

        private:
            float mDuration;
            MWWorld::ConstPtr mDoorPtr;
            osg::Vec3f mLastPos;
            int mDirection;

            bool isStuck(const osg::Vec3f& actorPos) const;

            void adjustDirection();

            float getAdjustedAngle() const;
    };
}
#endif

