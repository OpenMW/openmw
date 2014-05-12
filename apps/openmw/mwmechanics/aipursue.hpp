#ifndef GAME_MWMECHANICS_AIPURSUE_H
#define GAME_MWMECHANICS_AIPURSUE_H

#include "aipackage.hpp"

#include "../mwbase/world.hpp"

#include "pathfinding.hpp"

namespace MWMechanics
{

    class AiPursue : public AiPackage
    {
        public:
            AiPursue(const MWWorld::Ptr target);
            virtual AiPursue *clone() const;
            virtual bool execute (const MWWorld::Ptr& actor,float duration);
                    ///< \return Package completed?
            virtual int getTypeId() const;

            virtual MWWorld::Ptr getTarget() const;

        private:

            MWWorld::Ptr mTarget;

            PathFinder mPathFinder;
            int mCellX;
            int mCellY;
    };
}
#endif
