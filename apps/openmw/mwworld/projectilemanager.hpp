#ifndef OPENMW_MWWORLD_PROJECTILEMANAGER_H
#define OPENMW_MWWORLD_PROJECTILEMANAGER_H

#include <string>

#include <osg/ref_ptr>

#include <components/esm/effectlist.hpp>

#include "../mwbase/soundmanager.hpp"

#include "ptr.hpp"

namespace MWPhysics
{
    class PhysicsSystem;
}

namespace Loading
{
    class Listener;
}

namespace osg
{
    class Group;
    class Quat;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWRender
{
    class EffectAnimationTime;
}

namespace MWWorld
{

    class ProjectileManager
    {
    public:
        ProjectileManager (osg::Group* parent, Resource::ResourceSystem* resourceSystem,
                MWPhysics::PhysicsSystem* physics);

        /// If caster is an actor, the actor's facing orientation is used. Otherwise fallbackDirection is used.
        void launchMagicBolt (const std::string& model, const std::string &sound, const std::string &spellId,
                                     float speed, bool stack, const ESM::EffectList& effects,
                                       const MWWorld::Ptr& caster, const std::string& sourceName, const osg::Vec3f& fallbackDirection);

        void launchProjectile (MWWorld::Ptr actor, MWWorld::Ptr projectile,
                                       const osg::Vec3f& pos, const osg::Quat& orient, MWWorld::Ptr bow, float speed, float attackStrength);

        void update(float dt);

        /// Removes all current projectiles. Should be called when switching to a new worldspace.
        void clear();

        void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;
        bool readRecord (ESM::ESMReader& reader, uint32_t type);
        int countSavedGameRecords() const;

    private:
        osg::ref_ptr<osg::Group> mParent;
        Resource::ResourceSystem* mResourceSystem;
        MWPhysics::PhysicsSystem* mPhysics;

        struct State
        {
            osg::ref_ptr<osg::PositionAttitudeTransform> mNode;
            boost::shared_ptr<MWRender::EffectAnimationTime> mEffectAnimationTime;

            int mActorId;

            // TODO: this will break when the game is saved and reloaded, since there is currently
            // no way to write identifiers for non-actors to a savegame.
            MWWorld::Ptr mCasterHandle;

            MWWorld::Ptr getCaster();

            // MW-id of this projectile
            std::string mId;
        };

        struct MagicBoltState : public State
        {
            std::string mSpellId;

            // Name of item to display as effect source in magic menu (in case we casted an enchantment)
            std::string mSourceName;

            ESM::EffectList mEffects;

            float mSpeed;

            bool mStack;

            MWBase::SoundPtr mSound;
            std::string mSoundId;
        };

        struct ProjectileState : public State
        {
            // RefID of the bow or crossbow the actor was using when this projectile was fired (may be empty)
            std::string mBowId;

            osg::Vec3f mVelocity;
            float mAttackStrength;
        };

        std::vector<MagicBoltState> mMagicBolts;
        std::vector<ProjectileState> mProjectiles;

        void moveProjectiles(float dt);
        void moveMagicBolts(float dt);

        void createModel (State& state, const std::string& model, const osg::Vec3f& pos, const osg::Quat& orient);
        void update (State& state, float duration);
    };

}

#endif
