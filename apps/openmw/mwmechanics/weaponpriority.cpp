#include "weaponpriority.hpp"

#include <components/esm/loadench.hpp>
#include <components/esm/loadmgef.hpp>

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

namespace MWMechanics
{
    float rateWeapon (const MWWorld::Ptr &item, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, int type,
                      float arrowRating, float boltRating)
    {
        if (item.getTypeName() != typeid(ESM::Weapon).name())
            return 0.f;

        const ESM::Weapon* weapon = item.get<ESM::Weapon>()->mBase;

        if (type != -1 && weapon->mData.mType != type)
            return 0.f;

        if (type == -1 && (weapon->mData.mType == ESM::Weapon::Arrow || weapon->mData.mType == ESM::Weapon::Bolt))
            return 0.f;

        float rating=0.f;
        float rangedMult=1.f;

        if (weapon->mData.mType >= ESM::Weapon::MarksmanBow && weapon->mData.mType <= ESM::Weapon::MarksmanThrown)
        {
            // Range weapon is useless under water
            if (MWBase::Environment::get().getWorld()->isUnderwater(MWWorld::ConstPtr(actor), 0.75f))
                return 0.f;

            if (enemy.isEmpty())
                return 0.f;

            if (MWBase::Environment::get().getWorld()->isUnderwater(MWWorld::ConstPtr(enemy), 0.75f))
                return 0.f;

            if (getDistanceMinusHalfExtents(actor, enemy) >= getMaxAttackDistance(enemy))
                rangedMult = 1.5f;
        }

        if (weapon->mData.mType >= ESM::Weapon::MarksmanBow)
        {
            float rangedDamage = weapon->mData.mChop[0] + weapon->mData.mChop[1];
            MWMechanics::adjustWeaponDamage(rangedDamage, item, actor);

            rating = rangedDamage / 2.f;

            if (weapon->mData.mType >= ESM::Weapon::MarksmanThrown)
                MWMechanics::resistNormalWeapon(enemy, actor, item, rating);
        }
        else
        {
            float meleeDamage = 0.f;

            for (int i=0; i<2; ++i)
            {
                meleeDamage += weapon->mData.mSlash[i];
                meleeDamage += weapon->mData.mThrust[i];
                meleeDamage += weapon->mData.mChop[i];
            }

            MWMechanics::adjustWeaponDamage(meleeDamage, item, actor);
            rating = meleeDamage / 6.f;

            MWMechanics::resistNormalWeapon(enemy, actor, item, rating);
        }

        if (item.getClass().hasItemHealth(item))
        {
            if (item.getClass().getItemHealth(item) == 0)
                return 0.f;
        }

        if (weapon->mData.mType == ESM::Weapon::MarksmanBow)
        {
            if (arrowRating <= 0.f)
                rating = 0.f;
            else
                rating += arrowRating;
        }
        else if (weapon->mData.mType == ESM::Weapon::MarksmanCrossbow)
        {
            if (boltRating <= 0.f)
                rating = 0.f;
            else
                rating += boltRating;
        }

        if (!weapon->mEnchant.empty())
        {
            const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(weapon->mEnchant);
            if (enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
            {
                int castCost = getEffectiveEnchantmentCastCost(static_cast<float>(enchantment->mData.mCost), actor);

                if (item.getCellRef().getEnchantmentCharge() == -1 || item.getCellRef().getEnchantmentCharge() >= castCost)
                    rating += rateEffects(enchantment->mEffects, actor, enemy);
            }
        }

        if (actor.getClass().isNpc())
        {
            int skill = item.getClass().getEquipmentSkill(item);
            if (skill != -1)
            {
               int value = actor.getClass().getSkill(actor, skill);
               rating *= MWMechanics::getHitChance(actor, enemy, value) / 100.f;
            }
        }
        else
        {
            MWWorld::LiveCellRef<ESM::Creature> *ref = actor.get<ESM::Creature>();
            rating *= MWMechanics::getHitChance(actor, enemy, ref->mBase->mData.mCombat) / 100.f;
        }

        return rating * rangedMult;
    }

    float rateAmmo(const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy, MWWorld::Ptr &bestAmmo, ESM::Weapon::Type ammoType)
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

    float rateAmmo(const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy, ESM::Weapon::Type ammoType)
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

        if (esmWeap->mData.mType >= ESM::Weapon::MarksmanBow)
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
