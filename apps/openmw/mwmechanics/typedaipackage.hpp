#ifndef GAME_MWMECHANICS_TYPEDAIPACKAGE_H
#define GAME_MWMECHANICS_TYPEDAIPACKAGE_H

#include "aipackage.hpp"

namespace MWMechanics
{
    template <class T>
    struct TypedAiPackage : public AiPackage
    {
        virtual std::unique_ptr<AiPackage> clone() const override
        {
            return std::make_unique<T>(*static_cast<const T*>(this));
        }
    };
}

#endif
