#ifndef GAME_MWMECHANICS_AIESCORT_H
#define GAME_MWMECHANICS_AIESCORT_H

#include "aipackage.hpp"
#include "pathfinding.hpp"
#include <string>

/* From CS:
Escort

Makes the actor escort another actor to a location or for a specified period of time. During this time the actor will also protect the actor it is escorting. 

If you are not doing this package with the player as the target, you’ll want to also put a follow package on the target Actor, since escorting an actor makes the escorter wait for the other actor. If the Target does not know they are supposed to follow, the escorter will most likely just stand there.

Target: The ActorID to Escort. Remember that since all ActorIDs share the same AI packages, putting this on an Actor with multiple references will cause ALL references of that actor to attempt to escort the same actor. Thus, this type of AI should only be placed on specific or unique sets of Actors.

Duration: The duration the actor should escort for. Trumped by providing a location.

Escort to: Check this to use location data for the escort.

Cell: The Cell to escort to.

XYZ: Like Travel, specify the XYZ location to escort to.

View Location: A red X will appear in the render window that you can move around with the standard render window object controls. Place the X on the escort destination.


*/

namespace MWMechanics
{
    class AiEscort : public AiPackage
    {
        public:
            AiEscort(const std::string &actorId,int duration, float x, float y, float z);
            ///< \implement AiEscort
            AiEscort(const std::string &actorId,const std::string &cellId,int duration, float x, float y, float z);
            ///< \implement AiEscortCell

            virtual AiEscort *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor);
            ///< \return Package completed?

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
            int cellX;
            int cellY;
    };
}
#endif
