#include "weaponpriority.hpp"

#include <components/esm/loadench.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "npcstats.hpp"
#include "combat.hpp"
#include "aicombataction.hpp"
#include "spellpriority.hpp"
#include "spellcasting.hpp"
#include "weapontype.hpp"

namespace MWMechanics
{
    float rateWeapon (const MWWorld::Ptr &item, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, int type,
                      float arrowRating, float boltRating)
    {
        if (enemy.isEmpty() || item.getTypeName() != typeid(ESM::Weapon).name())
            return 0.f;

        if (item.getClass().hasItemHealth(item) && item.getClass().getItemHealth(item) == 0)
            return 0.f;

        const ESM::Weapon* weapon = item.get<ESM::Weapon>()->mBase;

        if (type != -1 && weapon->mData.mType != type)
            return 0.f;

        const MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& gmst = world->getStore().get<ESM::GameSetting>();

        ESM::WeaponType::Class weapclass = MWMechanics::getWeaponType(weapon->mData.mType)->mWeaponClass;
        if (type == -1 && weapclass == ESM::WeaponType::Ammo)
            return 0.f;

        float rating=0.f;
        static const float fAIMeleeWeaponMult = gmst.find("fAIMeleeWeaponMult")->mValue.getFloat();
        float ratingMult = fAIMeleeWeaponMult;

        if (weapclass != ESM::WeaponType::Melee)
        {
            // Underwater ranged combat is impossible
            if (world->isUnderwater(MWWorld::ConstPtr(actor), 0.75f)
             || world->isUnderwater(MWWorld::ConstPtr(enemy), 0.75f))
                return 0.f;

            // Use a higher rating multiplier if the actor is out of enemy's reach, use the normal mult otherwise
            if (getDistanceMinusHalfExtents(actor, enemy) >= getMaxAttackDistance(enemy))
            {
                static const float fAIRangeMeleeWeaponMult = gmst.find("fAIRangeMeleeWeaponMult")->mValue.getFloat();
                ratingMult = fAIRangeMeleeWeaponMult;
            }
        }

        const float chop = (weapon->mData.mChop[0] + weapon->mData.mChop[1]) / 2.f;
        // We need to account for the fact that thrown weapons have 2x real damage applied to the target
        // as they're both the weapon and the ammo of the hit
        if (weapclass == ESM::WeaponType::Thrown)
        {
            rating = chop * 2;
        }
        else if (weapclass != ESM::WeaponType::Melee)
        {
            rating = chop;
        }
        else
        {
            const float slash = (weapon->mData.mSlash[0] + weapon->mData.mSlash[1]) / 2.f;
            const float thrust = (weapon->mData.mThrust[0] + weapon->mData.mThrust[1]) / 2.f;
            rating = (slash * slash + thrust * thrust + chop * chop) / (slash + thrust + chop);
        }

        adjustWeaponDamage(rating, item, actor);

        if (weapclass != ESM::WeaponType::Ranged)
        {
            resistNormalWeapon(enemy, actor, item, rating);
            applyWerewolfDamageMult(enemy, item, rating);
        }
        else
        {
            int ammotype = MWMechanics::getWeaponType(weapon->mData.mType)->mAmmoType;
            if (ammotype == ESM::Weapon::Arrow)
            {
                if (arrowRating <= 0.f)
                    rating = 0.f;
                else
                    rating += arrowRating;
            }
            else if (ammotype == ESM::Weapon::Bolt)
            {
                if (boltRating <= 0.f)
                    rating = 0.f;
                else
                    rating += boltRating;
            }
        }

        if (!weapon->mEnchant.empty())
        {
            const ESM::Enchantment* enchantment = world->getStore().get<ESM::Enchantment>().find(weapon->mEnchant);
            if (enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
            {
                int castCost = getEffectiveEnchantmentCastCost(static_cast<float>(enchantment->mData.mCost), actor);
                float charge = item.getCellRef().getEnchantmentCharge();

                if (charge == -1 || charge >= castCost || weapclass == ESM::WeaponType::Thrown || weapclass == ESM::WeaponType::Ammo)
                    rating += rateEffects(enchantment->mEffects, actor, enemy);
            }
        }

        int value = 50.f;
        if (actor.getClass().isNpc())
        {
            int skill = item.getClass().getEquipmentSkill(item);
            if (skill != -1)
               value = actor.getClass().getSkill(actor, skill);
        }
        else
        {
            MWWorld::LiveCellRef<ESM::Creature> *ref = actor.get<ESM::Creature>();
            value = ref->mBase->mData.mCombat;
        }

        // Take hit chance in account, but do not allow rating become negative.
        float chance = getHitChance(actor, enemy, value) / 100.f;
        rating *= std::min(1.f, std::max(0.01f, chance));

        if (weapclass != ESM::WeaponType::Ammo)
            rating *= weapon->mData.mSpeed;

        return rating * ratingMult;
    }

    float rateAmmo(const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy, MWWorld::Ptr &bestAmmo, int ammoType)
    {
        float bestAmmoRating = 0.f;
        if (!actor.getClass().hasInventoryStore(actor))
            return bestAmmoRating;

        MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);

        for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
        {
            float rating = rateWeapon(*it, actor, enemy, ammoType);
            if (rating > bestAmmoRating)
            {
                bestAmmoRating = rating;
                bestAmmo = *it;
            }
        }

        return bestAmmoRating;
    }

    float rateAmmo(const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy, int ammoType)
    {
        MWWorld::Ptr emptyPtr;
        return rateAmmo(actor, enemy, emptyPtr, ammoType);
    }

    float vanillaRateWeaponAndAmmo(const MWWorld::Ptr& weapon, const MWWorld::Ptr& ammo, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        static const float fAIMeleeWeaponMult = gmst.find("fAIMeleeWeaponMult")->mValue.getFloat();
        static const float fAIMeleeArmorMult = gmst.find("fAIMeleeArmorMult")->mValue.getFloat();
        static const float fAIRangeMeleeWeaponMult = gmst.find("fAIRangeMeleeWeaponMult")->mValue.getFloat();

        if (weapon.isEmpty())
            return 0.f;

        float skillMult = actor.getClass().getSkill(actor, weapon.getClass().getEquipmentSkill(weapon)) * 0.01f;
        float chopMult = fAIMeleeWeaponMult;
        float bonusDamage = 0.f;

        const ESM::Weapon* esmWeap = weapon.get<ESM::Weapon>()->mBase;
        int type = esmWeap->mData.mType;
        if (getWeaponType(type)->mWeaponClass != ESM::WeaponType::Melee)
        {
            if (!ammo.isEmpty() && !MWBase::Environment::get().getWorld()->isSwimming(enemy))
            {
                bonusDamage = ammo.get<ESM::Weapon>()->mBase->mData.mChop[1];
                chopMult = fAIRangeMeleeWeaponMult;
            }
            else
                chopMult = 0.f;
        }

        float chopRating = (esmWeap->mData.mChop[1] + bonusDamage) * skillMult * chopMult;
        float slashRating = esmWeap->mData.mSlash[1] * skillMult * fAIMeleeWeaponMult;
        float thrustRating = esmWeap->mData.mThrust[1] * skillMult * fAIMeleeWeaponMult;

        return actor.getClass().getArmorRating(actor) * fAIMeleeArmorMult
                    + std::max(std::max(chopRating, slashRating), thrustRating);
    }

}
