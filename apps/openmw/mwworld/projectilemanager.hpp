#ifndef OPENMW_MWWORLD_PROJECTILEMANAGER_H
#define OPENMW_MWWORLD_PROJECTILEMANAGER_H

#include <string>

#include <OgreVector3.h>

#include <components/esm/effectlist.hpp>
#include <components/nifogre/ogrenifloader.hpp>

#include "../mwbase/soundmanager.hpp"

#include "ptr.hpp"

namespace OEngine
{
namespace Physic
{
    class PhysicEngine;
}
}

namespace Ogre
{
    class SceneManager;
}

namespace MWWorld
{

    class ProjectileManager
    {
    public:
        ProjectileManager (Ogre::SceneManager* sceneMgr,
                OEngine::Physic::PhysicEngine& engine);

        void launchMagicBolt (const std::string& model, const std::string &sound, const std::string &spellId,
                                     float speed, bool stack, const ESM::EffectList& effects,
                                       const MWWorld::Ptr& actor, const std::string& sourceName);

        void launchProjectile (MWWorld::Ptr actor, MWWorld::Ptr projectile,
                                       const Ogre::Vector3& pos, const Ogre::Quaternion& orient, MWWorld::Ptr bow, float speed);

        void update(float dt);

        /// Removes all current projectiles. Should be called when switching to a new worldspace.
        void clear();

    private:
        OEngine::Physic::PhysicEngine& mPhysEngine;
        Ogre::SceneManager* mSceneMgr;

        struct State
        {
            NifOgre::ObjectScenePtr mObject;
            Ogre::SceneNode* mNode;
        };

        struct MagicBoltState : public State
        {
            // Id of spell or enchantment to apply when it hits
            std::string mId;

            // Actor who casted this projectile
            int mActorId;

            // Name of item to display as effect source in magic menu (in case we casted an enchantment)
            std::string mSourceName;

            ESM::EffectList mEffects;

            float mSpeed;

            bool mStack;

            MWBase::SoundPtr mSound;
        };

        struct ProjectileState : public State
        {
            // Actor who shot this projectile
            int mActorId;

            // RefID of the projectile
            std::string mProjectileId;

            // RefID of the bow or crossbow the actor was using when this projectile was fired (may be empty)
            std::string mBowId;

            Ogre::Vector3 mVelocity;
        };

        std::vector<MagicBoltState> mMagicBolts;
        std::vector<ProjectileState> mProjectiles;

        void moveProjectiles(float dt);
        void moveMagicBolts(float dt);

        void createModel (State& state, const std::string& model);
        void update (NifOgre::ObjectScenePtr object, float duration);
    };

}

#endif
