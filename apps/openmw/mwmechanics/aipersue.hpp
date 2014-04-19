#ifndef GAME_MWMECHANICS_AIPERSUE_H
#define GAME_MWMECHANICS_AIPERSUE_H

#include "aipackage.hpp"
#include <string>

#include "pathfinding.hpp"

namespace MWMechanics
{

    class AiPersue : public AiPackage
    {
        public:
            AiPersue(const std::string &objectId);
            virtual AiPersue *clone() const;
            virtual bool execute (const MWWorld::Ptr& actor,float duration);
                    ///< \return Package completed?
            virtual int getTypeId() const;

        private:
            std::string mObjectId;

            PathFinder mPathFinder;
            int mCellX;
            int mCellY;
    };
}
#endif
