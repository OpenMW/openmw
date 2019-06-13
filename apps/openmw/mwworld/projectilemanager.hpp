#ifndef OPENMW_MWWORLD_PROJECTILEMANAGER_H
#define OPENMW_MWWORLD_PROJECTILEMANAGER_H

#include <string>

#include <osg/ref_ptr>
#include <osg/PositionAttitudeTransform>

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
    class RenderingManager;
}

namespace MWWorld
{

    class ProjectileManager
    {
    public:
        ProjectileManager (osg::Group* parent, Resource::ResourceSystem* resourceSystem,
                MWRender::RenderingManager* rendering, MWPhysics::PhysicsSystem* physics);

        /// If caster is an actor, the actor's facing orientation is used. Otherwise fallbackDirection is used.
        void launchMagicBolt (const std::string &spellId, const MWWorld::Ptr& caster, const osg::Vec3f& fallbackDirection);

        void launchProjectile (MWWorld::Ptr actor, MWWorld::ConstPtr projectile,
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
        MWRender::RenderingManager* mRendering;
        MWPhysics::PhysicsSystem* mPhysics;
        float mCleanupTimer;

        struct State
        {
            osg::ref_ptr<osg::PositionAttitudeTransform> mNode;
            std::shared_ptr<MWRender::EffectAnimationTime> mEffectAnimationTime;

            int mActorId;

            // TODO: this will break when the game is saved and reloaded, since there is currently
            // no way to write identifiers for non-actors to a savegame.
            MWWorld::Ptr mCasterHandle;

            MWWorld::Ptr getCaster();

            // MW-ids of a magic projectile
            std::vector<std::string> mIdMagic;

            // MW-id of an arrow projectile
            std::string mIdArrow;
        };

        struct MagicBoltState : public State
        {
            std::string mSpellId;

            // Name of item to display as effect source in magic menu (in case we casted an enchantment)
            std::string mSourceName;

            ESM::EffectList mEffects;

            float mSpeed;

            std::vector<MWBase::Sound*> mSounds;
            std::set<std::string> mSoundIds;
        };

        struct ProjectileState : public State
        {
            // RefID of the bow or crossbow the actor was using when this projectile was fired (may be empty)
            std::string mBowId;

            osg::Vec3f mVelocity;
            float mAttackStrength;
            bool mThrown;
        };

        std::vector<MagicBoltState> mMagicBolts;
        std::vector<ProjectileState> mProjectiles;

        void cleanupProjectile(ProjectileState& state);
        void cleanupMagicBolt(MagicBoltState& state);
        void periodicCleanup(float dt);

        void moveProjectiles(float dt);
        void moveMagicBolts(float dt);

        void createModel (State& state, const std::string& model, const osg::Vec3f& pos, const osg::Quat& orient,
                            bool rotate, bool createLight, osg::Vec4 lightDiffuseColor, std::string texture = "");
        void update (State& state, float duration);

        void operator=(const ProjectileManager&);
        ProjectileManager(const ProjectileManager&);
    };

}

#endif
