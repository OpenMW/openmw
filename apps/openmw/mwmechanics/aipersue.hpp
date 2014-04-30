#ifndef GAME_MWMECHANICS_AIPERSUE_H
#define GAME_MWMECHANICS_AIPERSUE_H

#include "aipackage.hpp"
#include <string>

#include "pathfinding.hpp"

namespace MWMechanics
{
    /// \brief Makes the actor very closely follow the actor
    /** Used for getting closer to fight, or to arrest (I think?) **/
    class AiPersue : public AiPackage
    {
        public:
            ///Constructor
            /** \param objectId Actor to pursue **/
            AiPersue(const std::string &objectId);
            virtual AiPersue *clone() const;
            virtual bool execute (const MWWorld::Ptr& actor,float duration);
            virtual int getTypeId() const;

        private:
            std::string mObjectId;

            PathFinder mPathFinder;
            int mCellX;
            int mCellY;
    };
}
#endif
