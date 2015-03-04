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

namespace Loading
{
    class Listener;
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

        /// If caster is an actor, the actor's facing orientation is used. Otherwise fallbackDirection is used.
        void launchMagicBolt (const std::string& model, const std::string &sound, const std::string &spellId,
                                     float speed, bool stack, const ESM::EffectList& effects,
                                       const MWWorld::Ptr& caster, const std::string& sourceName, const Ogre::Vector3& fallbackDirection);

        void launchProjectile (MWWorld::Ptr actor, MWWorld::Ptr projectile,
                                       const Ogre::Vector3& pos, const Ogre::Quaternion& orient, MWWorld::Ptr bow, float speed);

        void update(float dt);

        /// Removes all current projectiles. Should be called when switching to a new worldspace.
        void clear();

        void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;
        bool readRecord (ESM::ESMReader& reader, uint32_t type);
        int countSavedGameRecords() const;

    private:
        OEngine::Physic::PhysicEngine& mPhysEngine;
        Ogre::SceneManager* mSceneMgr;

        struct State
        {
            NifOgre::ObjectScenePtr mObject;
            Ogre::SceneNode* mNode;

            int mActorId;

            // actorId doesn't work for non-actors, so we also keep track of the Ogre-handle.
            // For non-actors, the caster ptr is mainly needed to prevent the projectile
            // from colliding with its caster.
            // TODO: this will break when the game is saved and reloaded, since there is currently
            // no way to write identifiers for non-actors to a savegame.
            std::string mCasterHandle;

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
