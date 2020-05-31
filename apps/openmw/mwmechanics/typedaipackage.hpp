#ifndef GAME_MWMECHANICS_TYPEDAIPACKAGE_H
#define GAME_MWMECHANICS_TYPEDAIPACKAGE_H

#include "aipackage.hpp"

namespace MWMechanics
{
    template <class T>
    struct TypedAiPackage : public AiPackage
    {
        virtual TypedAiPackage<T> *clone() const override
        {
            return new T(*static_cast<const T*>(this));
        }
    };
}

#endif
