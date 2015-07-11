#ifndef GAME_MWMECHANICS_AICOMBAT_H
#define GAME_MWMECHANICS_AICOMBAT_H

#include "aipackage.hpp"

#include "pathfinding.hpp"

#include "movement.hpp"
#include "obstacle.hpp"

#include "../mwworld/cellstore.hpp" // for Doors

#include "../mwbase/world.hpp"



namespace ESM
{
    namespace AiSequence
    {
        struct AiCombat;
    }
}

namespace MWMechanics
{
    class Action;

    /// \brief Causes the actor to fight another actor
    class AiCombat : public AiPackage
    {
        public:
            ///Constructor
            /** \param actor Actor to fight **/
            AiCombat(const MWWorld::Ptr& actor);

            AiCombat (const ESM::AiSequence::AiCombat* combat);

            void init();

            virtual AiCombat *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration);

            virtual int getTypeId() const;

            virtual unsigned int getPriority() const;

            ///Returns target ID
            MWWorld::Ptr getTarget() const;

            virtual void writeState(ESM::AiSequence::AiSequence &sequence) const;

        protected:
            virtual bool doesPathNeedRecalc(ESM::Pathgrid::Point dest, const ESM::Cell *cell);

        private:

            int mTargetActorId;


            void buildNewPath(const MWWorld::Ptr& actor, const MWWorld::Ptr& target);
    };
    
    
}

#endif
