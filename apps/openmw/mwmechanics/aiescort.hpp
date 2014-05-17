#ifndef GAME_MWMECHANICS_AIESCORT_H
#define GAME_MWMECHANICS_AIESCORT_H

#include "aipackage.hpp"
#include <string>

#include "pathfinding.hpp"

namespace MWMechanics
{
    /// \brief AI Package to have an NPC lead the player to a specific point
    class AiEscort : public AiPackage
    {
        public:
            /// Implementation of AiEscort
            /** The Actor will escort the specified actor to the world position x, y, z until they reach their position, or they run out of time
                \implement AiEscort **/
            AiEscort(const std::string &actorId,int duration, float x, float y, float z);
            /// Implementation of AiEscortCell
            /** The Actor will escort the specified actor to the cell position x, y, z until they reach their position, or they run out of time
                \implement AiEscortCell **/
            AiEscort(const std::string &actorId,const std::string &cellId,int duration, float x, float y, float z);

            virtual AiEscort *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor,float duration);

            virtual int getTypeId() const;

        private:
            std::string mActorId;
            std::string mCellId;
            float mX;
            float mY;
            float mZ;
            float mMaxDist;
            unsigned int mStartingSecond;
            unsigned int mDuration;

            PathFinder mPathFinder;
            int mCellX;
            int mCellY;
    };
}
#endif
