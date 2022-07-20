#ifndef GAME_MWMECHANICS_AIAVOIDDOOR_H
#define GAME_MWMECHANICS_AIAVOIDDOOR_H

#include "typedaipackage.hpp"

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
            explicit AiAvoidDoor(const MWWorld::ConstPtr& doorPtr);

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) override;

            static constexpr AiPackageTypeId getTypeId() { return AiPackageTypeId::AvoidDoor; }

            static constexpr Options makeDefaultOptions()
            {
                AiPackage::Options options;
                options.mPriority = 2;
                options.mCanCancel = false;
                options.mShouldCancelPreviousAi = false;
                return options;
            }

        private:
            float mDuration;
            const MWWorld::ConstPtr mDoorPtr;
            osg::Vec3f mLastPos;
            int mDirection;

            bool isStuck(const osg::Vec3f& actorPos) const;

            void adjustDirection();

            float getAdjustedAngle() const;
    };
}
#endif

