#ifndef GAME_MWMECHANICS_AICAST_H
#define GAME_MWMECHANICS_AICAST_H

#include "typedaipackage.hpp"
#include <components/esm/refid.hpp>

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    /// AiPackage which makes an actor to cast given spell.
    class AiCast final : public TypedAiPackage<AiCast>
    {
    public:
        AiCast(const ESM::RefId& targetId, const ESM::RefId& spellId, bool scriptedSpell = false);

        bool execute(const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state,
            float duration) override;

        static constexpr AiPackageTypeId getTypeId() { return AiPackageTypeId::Cast; }

        MWWorld::Ptr getTarget() const override;

        static constexpr Options makeDefaultOptions()
        {
            AiPackage::Options options;
            options.mPriority = 3;
            options.mCanCancel = false;
            options.mShouldCancelPreviousAi = false;
            return options;
        }

    private:
        const ESM::RefId mTargetId;
        const ESM::RefId mSpellId;
        bool mCasting;
        const bool mScripted;
        const float mDistance;
    };
}

#endif
