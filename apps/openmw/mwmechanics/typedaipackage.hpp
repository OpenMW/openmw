#ifndef GAME_MWMECHANICS_TYPEDAIPACKAGE_H
#define GAME_MWMECHANICS_TYPEDAIPACKAGE_H

#include "aipackage.hpp"

namespace MWMechanics
{
    template <class T>
    constexpr AiPackage::Options makeAiPackageOptions()
    {
        AiPackage::Options options;
        options.mPriority = T::defaultPriority();
        options.mUseVariableSpeed = T::defaultUseVariableSpeed();
        options.mSideWithTarget = T::defaultSideWithTarget();
        options.mFollowTargetThroughDoors = T::defaultFollowTargetThroughDoors();
        options.mCanCancel = T::defaultCanCancel();
        options.mShouldCancelPreviousAi = T::defaultShouldCancelPreviousAi();
        options.mRepeat = T::defaultRepeat();
        options.mAlwaysActive = T::defaultAlwaysActive();
        return options;
    };

    template <class T>
    struct TypedAiPackage : public AiPackage
    {
        TypedAiPackage() :
            AiPackage(T::getTypeId(), makeAiPackageOptions<T>()) {}

        template <class Derived>
        TypedAiPackage(Derived*) :
            AiPackage(Derived::getTypeId(), makeAiPackageOptions<Derived>()) {}

        virtual std::unique_ptr<AiPackage> clone() const override
        {
            return std::make_unique<T>(*static_cast<const T*>(this));
        }
    };
}

#endif
