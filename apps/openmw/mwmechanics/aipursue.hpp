#ifndef GAME_MWMECHANICS_AIPURSUE_H
#define GAME_MWMECHANICS_AIPURSUE_H

#include "aipackage.hpp"
#include <string>

#include "pathfinding.hpp"

namespace MWMechanics
{

    class AiPursue : public AiPackage
    {
        public:
            AiPursue(const std::string &objectId);
            virtual AiPursue *clone() const;
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
