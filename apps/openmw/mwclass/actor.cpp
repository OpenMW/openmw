#include "actor.hpp"

#include <components/esm/loadmgef.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/magiceffects.hpp"

#include "../mwphysics/physicssystem.hpp"

#include "../mwworld/inventorystore.hpp"

namespace MWClass
{
    Actor::Actor() {}

    Actor::~Actor() {}

    void Actor::adjustPosition(const MWWorld::Ptr& ptr, bool force) const
    {
        MWBase::Environment::get().getWorld()->adjustPosition(ptr, force);
    }

    void Actor::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        if (!model.empty())
        {
            physics.addActor(ptr, model);
            if (getCreatureStats(ptr).isDead() && getCreatureStats(ptr).isDeathAnimationFinished())
                MWBase::Environment::get().getWorld()->enableActorCollision(ptr, false);
        }
    }

    bool Actor::useAnim() const
    {
        return true;
    }

    void Actor::block(const MWWorld::Ptr &ptr) const
    {
        const MWWorld::InventoryStore& inv = getInventoryStore(ptr);
        MWWorld::ConstContainerStoreIterator shield = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if (shield == inv.end())
            return;

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        switch (shield->getClass().getEquipmentSkill(*shield))
        {
            case ESM::Skill::LightArmor:
                sndMgr->playSound3D(ptr, "Light Armor Hit", 1.0f, 1.0f);
                break;
            case ESM::Skill::MediumArmor:
                sndMgr->playSound3D(ptr, "Medium Armor Hit", 1.0f, 1.0f);
                break;
            case ESM::Skill::HeavyArmor:
                sndMgr->playSound3D(ptr, "Heavy Armor Hit", 1.0f, 1.0f);
                break;
            default:
                return;
        }
    }

    osg::Vec3f Actor::getRotationVector(const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        osg::Vec3f vec(movement.mRotation[0], movement.mRotation[1], movement.mRotation[2]);
        movement.mRotation[0] = 0.0f;
        movement.mRotation[1] = 0.0f;
        movement.mRotation[2] = 0.0f;
        return vec;
    }

    float Actor::getEncumbrance(const MWWorld::Ptr& ptr) const
    {
        float weight = getContainerStore(ptr).getWeight();
        const MWMechanics::MagicEffects& effects = getCreatureStats(ptr).getMagicEffects();
        weight -= effects.get(MWMechanics::EffectKey(ESM::MagicEffect::Feather)).getMagnitude();
        weight += effects.get(MWMechanics::EffectKey(ESM::MagicEffect::Burden)).getMagnitude();
        return (weight < 0) ? 0.0f : weight;
    }

    bool Actor::allowTelekinesis(const MWWorld::ConstPtr &ptr) const {
        return false;
    }

    bool Actor::isActor() const
    {
        return true;
    }
}
