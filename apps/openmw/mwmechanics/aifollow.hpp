#ifndef GAME_MWMECHANICS_AIFOLLOW_H
#define GAME_MWMECHANICS_AIFOLLOW_H

#include "aipackage.hpp"
#include <string>
#include "pathfinding.hpp"
#include <components/esm/defs.hpp>

namespace ESM
{
namespace AiSequence
{
    struct AiFollow;
}
}

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
            AiFollow(const std::string &ActorId, bool commanded=false);

            AiFollow(const ESM::AiSequence::AiFollow* follow);

            MWWorld::Ptr getTarget();

            virtual AiFollow *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor, AiState& state, float duration);

            virtual int getTypeId() const;

            /// Returns the actor being followed
            std::string getFollowedActor();

            virtual void writeState (ESM::AiSequence::AiSequence& sequence) const;

            bool isCommanded() const;

            int getFollowIndex() const;

        private:
            /// This will make the actor always follow.
            /** Thus ignoring mDuration and mX,mY,mZ (used for summoned creatures). **/
            bool mAlwaysFollow;
            bool mCommanded;
            float mRemainingDuration; // Seconds
            float mX;
            float mY;
            float mZ;
            std::string mActorRefId;
            int mActorId;
            std::string mCellId;
            bool mActive; // have we spotted the target?
            int mFollowIndex;

            static int mFollowIndexCounter;
    };
}
#endif
