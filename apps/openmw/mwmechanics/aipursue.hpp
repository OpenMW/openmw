#ifndef GAME_MWMECHANICS_AIPURSUE_H
#define GAME_MWMECHANICS_AIPURSUE_H

#include "typedaipackage.hpp"

namespace ESM
{
namespace AiSequence
{
    struct AiPursue;
}
}

namespace MWMechanics
{
    /// \brief Makes the actor very closely follow the actor
    /** Used for arresting players. Causes the actor to run to the pursued actor and activate them, to arrest them.
        Note that while very similar to AiActivate, it will ONLY activate when evry close to target (Not also when the
        path is completed). **/
    class AiPursue final : public TypedAiPackage<AiPursue>
    {
        public:
            ///Constructor
            /** \param actor Actor to pursue **/
            AiPursue(const MWWorld::Ptr& actor);

            AiPursue(const ESM::AiSequence::AiPursue* pursue);

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) final;

            static constexpr TypeId getTypeId() { return TypeIdPursue; }

            MWWorld::Ptr getTarget() const final;

            void writeState (ESM::AiSequence::AiSequence& sequence) const final;

            static constexpr bool defaultCanCancel() { return false; }

            static constexpr bool defaultShouldCancelPreviousAi() { return false; }
    };
}
#endif
