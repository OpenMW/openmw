#ifndef OPENMW_MWMECHANICS_RUMBLE_HPP
#define OPENMW_MWMECHANICS_RUMBLE_HPP

#include <algorithm>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "actorutil.hpp"
#include "creaturestats.hpp"

namespace MWMechanics::Rumble
{
    inline MWBase::InputManager* getInputManager()
    {
        return MWBase::Environment::get().getInputManager();
    }

    inline bool prepare(MWBase::InputManager*& inputManager)
    {
        inputManager = getInputManager();
        return inputManager != nullptr && inputManager->controllerHasRumble();
    }

    inline void onPlayerDamaged(const MWWorld::Ptr& player, float healthDamage)
    {
        if (player != MWMechanics::getPlayer() || healthDamage <= 0.f)
            return;

        MWBase::InputManager* inputManager = nullptr;
        if (!prepare(inputManager))
            return;

        const MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        const float maxHealth = stats.getHealth().getModified();
        if (maxHealth <= 0.f)
            return;

        const float normalized = std::clamp(healthDamage / maxHealth * 2.f, 0.f, 1.f);
        const float low = 0.3f + 0.5f * normalized;
        const float high = 0.6f + 0.4f * normalized;
        const float duration = 0.25f + 0.25f * normalized;

        inputManager->playControllerRumble(low, high, duration);
    }

    inline void onPlayerDealtDamage(float healthDamage)
    {
        if (healthDamage <= 0.f)
            return;

        MWBase::InputManager* inputManager = nullptr;
        if (!prepare(inputManager))
            return;

        const float normalized = std::clamp(healthDamage / 35.f, 0.f, 1.f);
        const float low = 0.25f + 0.45f * normalized;
        const float high = 0.35f + 0.55f * normalized;
        const float duration = 0.16f + 0.12f * normalized;

        inputManager->playControllerRumble(low, high, duration);
    }

    inline void onPlayerBlock(float attackStrength, float damageBlocked)
    {
        if (damageBlocked < 0.f)
            damageBlocked = 0.f;

        MWBase::InputManager* inputManager = nullptr;
        if (!prepare(inputManager))
            return;

        const float normalizedStrength = std::clamp(attackStrength, 0.f, 1.f);
        const float impact = std::clamp(damageBlocked / 25.f, 0.f, 1.f);
        const float low = 0.2f + 0.6f * impact;
        const float high = 0.15f + 0.55f * normalizedStrength;
        const float duration = 0.12f + 0.10f * std::max(impact, normalizedStrength);

        inputManager->playControllerRumble(low, high, duration);
    }

    inline void onPlayerAttackWasBlocked(float damageBlocked)
    {
        if (damageBlocked <= 0.f)
            return;

        MWBase::InputManager* inputManager = nullptr;
        if (!prepare(inputManager))
            return;

        const float normalized = std::clamp(damageBlocked / 30.f, 0.f, 1.f);
        const float low = 0.25f + 0.4f * normalized;
        const float high = 0.3f + 0.4f * normalized;
        const float duration = 0.1f + 0.08f * normalized;

        inputManager->playControllerRumble(low, high, duration);
    }

    inline void onPlayerCastSpell(float baseCost, bool hasProjectileComponent)
    {
        if (baseCost < 0.f)
            baseCost = 0.f;

        MWBase::InputManager* inputManager = nullptr;
        if (!prepare(inputManager))
            return;

        const float normalizedCost = std::clamp(baseCost / 100.f, 0.f, 1.f);
        float low = 0.25f + 0.45f * normalizedCost;
        float high = 0.35f + 0.55f * normalizedCost;
        float duration = 0.18f + 0.14f * normalizedCost;

        if (hasProjectileComponent)
        {
            high = std::min(1.f, high + 0.1f);
            duration += 0.05f;
        }

        inputManager->playControllerRumble(low, high, duration);
    }
}

#endif
