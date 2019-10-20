#ifndef OPENMW_MECHANICS_COMBAT_H
#define OPENMW_MECHANICS_COMBAT_H

namespace osg
{
    class Vec3f;
}

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{

bool applyOnStrikeEnchantment(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, const MWWorld::Ptr& object, const osg::Vec3f& hitPosition,
                              const bool fromProjectile=false);

/// @return can we block the attack?
bool blockMeleeAttack (const MWWorld::Ptr& attacker, const MWWorld::Ptr& blocker, const MWWorld::Ptr& weapon, float damage, float attackStrength);

/// @return does normal weapon resistance and weakness apply to the weapon?
bool isNormalWeapon (const MWWorld::Ptr& weapon);

void resistNormalWeapon (const MWWorld::Ptr& actor, const MWWorld::Ptr& attacker, const MWWorld::Ptr& weapon, float& damage);

void applyWerewolfDamageMult (const MWWorld::Ptr& actor, const MWWorld::Ptr& weapon, float &damage);

/// @note for a thrown weapon, \a weapon == \a projectile, for bows/crossbows, \a projectile is the arrow/bolt
/// @note \a victim may be empty (e.g. for a hit on terrain), a non-actor (environment objects) or an actor
void projectileHit (const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, MWWorld::Ptr weapon, const MWWorld::Ptr& projectile,
                    const osg::Vec3f& hitPosition, float attackStrength);

/// Get the chance (in percent) for \a attacker to successfully hit \a victim with a given weapon skill value
float getHitChance (const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, int skillValue);

/// Applies damage to attacker based on the victim's elemental shields.
void applyElementalShields(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim);

/// @param damage Unmitigated weapon damage of the attack
/// @param hit Was the attack successful?
/// @param weapon The weapon used.
/// @note if the weapon is unequipped as result of condition damage, a new Ptr will be assigned to \a weapon.
void reduceWeaponCondition (float damage, bool hit, MWWorld::Ptr& weapon, const MWWorld::Ptr& attacker);

/// Adjust weapon damage based on its condition. A used weapon will be less effective.
void adjustWeaponDamage (float& damage, const MWWorld::Ptr& weapon, const MWWorld::Ptr& attacker);

void getHandToHandDamage (const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, float& damage, bool& healthdmg, float attackStrength);

/// Apply the fatigue loss incurred by attacking with the given weapon (weapon may be empty = hand-to-hand)
void applyFatigueLoss(const MWWorld::Ptr& attacker, const MWWorld::Ptr& weapon, float attackStrength);

float getFightDistanceBias(const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2);

bool isTargetMagicallyHidden(const MWWorld::Ptr& target);

float getAggroDistance(const MWWorld::Ptr& actor, const osg::Vec3f& lhs, const osg::Vec3f& rhs);

}

#endif
