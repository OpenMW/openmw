#ifndef GAME_MWMECHANICS_AIFOLLOW_H
#define GAME_MWMECHANICS_AIFOLLOW_H

#include "aipackage.hpp"
#include <string>
#include "pathfinding.hpp"
#include <components/esm/defs.hpp>

namespace MWMechanics
{
    /// \brief AiPackage for an actor to follow another actor/the PC
    /** The AI will follow the target until a condition (time, or position) are set. Both can be disabled to cause the actor to follow the other indefinitely
    **/
    class AiFollow : public AiPackage
    {
        public:
            /// Follow Actor for duration or until you arrive at a world position
            AiFollow(const std::string &ActorId,float duration, float X, float Y, float Z);
            /// Follow Actor for duration or until you arrive at a position in a cell
            AiFollow(const std::string &ActorId,const std::string &CellId,float duration, float X, float Y, float Z);
            /// Follow Actor indefinitively
            AiFollow(const std::string &ActorId);

            virtual AiFollow *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor,float duration);

            virtual int getTypeId() const;

            /// Returns the actor being followed
            std::string getFollowedActor();

        private:
            /// This will make the actor always follow.
            /** Thus ignoring mDuration and mX,mY,mZ (used for summoned creatures). **/
            bool mAlwaysFollow;
            float mDuration;
            float mX;
            float mY;
            float mZ;
            std::string mActorId;
            std::string mCellId;
    };
}
#endif
