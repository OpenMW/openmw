#ifndef OPENMW_MECHANICS_COMBAT_H
#define OPENMW_MECHANICS_COMBAT_H

#include "../mwworld/ptr.hpp"
#include <OgreVector3.h>

namespace MWMechanics
{

/// @return can we block the attack?
bool blockMeleeAttack (const MWWorld::Ptr& attacker, const MWWorld::Ptr& blocker, const MWWorld::Ptr& weapon, float damage);

void resistNormalWeapon (const MWWorld::Ptr& actor, const MWWorld::Ptr& attacker, const MWWorld::Ptr& weapon, float& damage);

/// @note for a thrown weapon, \a weapon == \a projectile, for bows/crossbows, \a projectile is the arrow/bolt
void projectileHit (const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, MWWorld::Ptr weapon, const MWWorld::Ptr& projectile,
                    const Ogre::Vector3& hitPosition);

/// Get the chance (in percent) for \a attacker to successfully hit \a victim with a given weapon skill value
float getHitChance (const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, int skillValue);

/// Applies damage to attacker based on the victim's elemental shields.
void applyElementalShields(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim);

}

#endif
