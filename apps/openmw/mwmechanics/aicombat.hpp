#ifndef GAME_MWMECHANICS_AICOMBAT_H
#define GAME_MWMECHANICS_AICOMBAT_H

#include "aipackage.hpp"

#include "pathfinding.hpp"

#include "movement.hpp"

namespace MWMechanics
{
    class AiCombat : public AiPackage
    {
        public:
            AiCombat(const std::string &targetId);

            virtual AiCombat *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor,float duration);
            ///< \return Package completed?

            virtual int getTypeId() const;

            virtual unsigned int getPriority() const;

            const std::string &getTargetId() const;

        private:
            std::string mTargetId;

            PathFinder mPathFinder;
            PathFinder mPathFinder2;
            float mTimer;
            float mTimer2;
    };
}

#endif
