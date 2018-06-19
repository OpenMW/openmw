#ifndef GAME_MWMECHANICS_AICOMBAT_H
#define GAME_MWMECHANICS_AICOMBAT_H

#include "aipackage.hpp"

#include "../mwworld/cellstore.hpp" // for Doors

#include "../mwbase/world.hpp"

#include "pathfinding.hpp"
#include "movement.hpp"
#include "obstacle.hpp"

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

    struct AiCombatStorage;

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

            virtual bool canCancel() const { return false; }
            virtual bool shouldCancelPreviousAi() const { return false; }

        private:
            /// Returns true if combat should end
            bool attack(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, AiCombatStorage& storage, CharacterController& characterController);

            void updateLOS(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, float duration, AiCombatStorage& storage);

            void updateFleeing(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, float duration, AiCombatStorage& storage);

            /// Transfer desired movement (from AiCombatStorage) to Actor
            void updateActorsMovement(const MWWorld::Ptr& actor, float duration, AiCombatStorage& storage);
            void rotateActorOnAxis(const MWWorld::Ptr& actor, int axis, 
                MWMechanics::Movement& actorMovementSettings, MWMechanics::Movement& desiredMovement);
    };
    
    
}

#endif
