#ifndef GAME_MWMECHANICS_AICOMBAT_H
#define GAME_MWMECHANICS_AICOMBAT_H

#include "aipackage.hpp"

#include "pathfinding.hpp"


#include "..\mwworld\class.hpp"
#include "creaturestats.hpp"
#include "npcstats.hpp"
#include "..\mwbase\environment.hpp"
#include "..\mwbase\world.hpp"
#include "..\mwworld\player.hpp"

#include "movement.hpp"

namespace MWMechanics
{
    class AiCombat : public AiPackage
    {
        public:
            AiCombat(const std::string &targetId);

            virtual AiCombat *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor);
            ///< \return Package completed?

            virtual int getTypeId() const;

        private:
            std::string mTargetId;

            PathFinder mPathFinder;
            unsigned int mStartingSecond;
    };
}

#endif