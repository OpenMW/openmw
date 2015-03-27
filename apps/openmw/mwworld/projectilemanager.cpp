#include "projectilemanager.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

#include <libs/openengine/bullet/physic.hpp>

#include <components/esm/projectilestate.hpp>

#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spellcasting.hpp"

#include "../mwrender/effectmanager.hpp"
#include "../mwrender/animation.hpp"
#include "../mwrender/renderconst.hpp"

#include "../mwsound/sound.hpp"

namespace MWWorld
{

    ProjectileManager::ProjectileManager(Ogre::SceneManager* sceneMgr, OEngine::Physic::PhysicEngine &engine)
        : mPhysEngine(engine)
        , mSceneMgr(sceneMgr)
    {

    }

    void ProjectileManager::createModel(State &state, const std::string &model)
    {
        state.mObject = NifOgre::Loader::createObjects(state.mNode, model);
        for(size_t i = 0;i < state.mObject->mControllers.size();i++)
        {
            if(state.mObject->mControllers[i].getSource().isNull())
                state.mObject->mControllers[i].setSource(Ogre::SharedPtr<MWRender::EffectAnimationTime> (new MWRender::EffectAnimationTime()));
        }

        MWRender::Animation::setRenderProperties(state.mObject, MWRender::RV_Effects,
                            MWRender::RQG_Main, MWRender::RQG_Alpha, 0.f, false, NULL);
    }

    void ProjectileManager::update(NifOgre::ObjectScenePtr object, float duration)
    {
        for(size_t i = 0; i < object->mControllers.size() ;i++)
        {
            MWRender::EffectAnimationTime* value = dynamic_cast<MWRender::EffectAnimationTime*>(object->mControllers[i].getSource().get());
            if (value)
                value->addTime(duration);

            object->mControllers[i].update();
        }
    }

    void ProjectileManager::launchMagicBolt(const std::string &model, const std::string &sound,
                                            const std::string &spellId, float speed, bool stack,
                                            const ESM::EffectList &effects, const Ptr &caster, const std::string &sourceName,
                                            const Ogre::Vector3& fallbackDirection)
    {
        float height = 0;
        if (OEngine::Physic::PhysicActor* actor = mPhysEngine.getCharacter(caster.getRefData().getHandle()))
            height = actor->getHalfExtents().z * 2 * 0.75f;         // Spawn at 0.75 * ActorHeight

        Ogre::Vector3 pos(caster.getRefData().getPosition().pos);
        pos.z += height;

        if (MWBase::Environment::get().getWorld()->isUnderwater(caster.getCell(), pos)) // Underwater casting not possible
            return;

        Ogre::Quaternion orient;
        if (caster.getClass().isActor())
            orient = Ogre::Quaternion(Ogre::Radian(caster.getRefData().getPosition().rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z) *
                    Ogre::Quaternion(Ogre::Radian(caster.getRefData().getPosition().rot[0]), Ogre::Vector3::NEGATIVE_UNIT_X);
        else
            orient = Ogre::Vector3::UNIT_Y.getRotationTo(fallbackDirection);

        MagicBoltState state;
        state.mSourceName = sourceName;
        state.mId = model;
        state.mSpellId = spellId;
        state.mCasterHandle = caster.getRefData().getHandle();
        if (caster.getClass().isActor())
            state.mActorId = caster.getClass().getCreatureStats(caster).getActorId();
        else
            state.mActorId = -1;
        state.mSpeed = speed;
        state.mStack = stack;
        state.mSoundId = sound;

        // Only interested in "on target" effects
        for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.mList.begin());
            iter!=effects.mList.end(); ++iter)
        {
            if (iter->mRange == ESM::RT_Target)
                state.mEffects.mList.push_back(*iter);
        }

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), model);
        MWWorld::Ptr ptr = ref.getPtr();

        state.mNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos, orient);
        createModel(state, ptr.getClass().getModel(ptr));

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        state.mSound = sndMgr->playManualSound3D(pos, sound, 1.0f, 1.0f, MWBase::SoundManager::Play_TypeSfx, MWBase::SoundManager::Play_Loop);

        mMagicBolts.push_back(state);
    }

    void ProjectileManager::launchProjectile(Ptr actor, Ptr projectile, const Ogre::Vector3 &pos,
                                             const Ogre::Quaternion &orient, Ptr bow, float speed)
    {
        ProjectileState state;
        state.mActorId = actor.getClass().getCreatureStats(actor).getActorId();
        state.mBowId = bow.getCellRef().getRefId();
        state.mVelocity = orient.yAxis() * speed;
        state.mId = projectile.getCellRef().getRefId();

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), projectile.getCellRef().getRefId());
        MWWorld::Ptr ptr = ref.getPtr();

        state.mNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos, orient);
        createModel(state, ptr.getClass().getModel(ptr));

        mProjectiles.push_back(state);
    }

    void ProjectileManager::update(float dt)
    {
        moveProjectiles(dt);
        moveMagicBolts(dt);
    }

    void ProjectileManager::moveMagicBolts(float duration)
    {
        for (std::vector<MagicBoltState>::iterator it = mMagicBolts.begin(); it != mMagicBolts.end();)
        {
            Ogre::Quaternion orient = it->mNode->getOrientation();
            static float fTargetSpellMaxSpeed = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("fTargetSpellMaxSpeed")->getFloat();
            float speed = fTargetSpellMaxSpeed * it->mSpeed;

            Ogre::Vector3 direction = orient.yAxis();
            direction.normalise();
            Ogre::Vector3 pos(it->mNode->getPosition());
            Ogre::Vector3 newPos = pos + direction * duration * speed;

            if (it->mSound.get())
                it->mSound->setPosition(newPos);

            it->mNode->setPosition(newPos);

            update(it->mObject, duration);

            // Check for impact
            // TODO: use a proper btRigidBody / btGhostObject?
            btVector3 from(pos.x, pos.y, pos.z);
            btVector3 to(newPos.x, newPos.y, newPos.z);

            std::vector<std::pair<float, std::string> > collisions = mPhysEngine.rayTest2(from, to, OEngine::Physic::CollisionType_Projectile);
            bool hit=false;

            for (std::vector<std::pair<float, std::string> >::iterator cIt = collisions.begin(); cIt != collisions.end() && !hit; ++cIt)
            {
                MWWorld::Ptr obstacle = MWBase::Environment::get().getWorld()->searchPtrViaHandle(cIt->second);

                MWWorld::Ptr caster = MWBase::Environment::get().getWorld()->searchPtrViaHandle(it->mCasterHandle);
                if (caster.isEmpty())
                    caster = MWBase::Environment::get().getWorld()->searchPtrViaActorId(it->mActorId);

                if (!obstacle.isEmpty() && obstacle == caster)
                    continue;

                if (caster.isEmpty())
                    caster = obstacle;

                if (obstacle.isEmpty())
                {
                    // Terrain
                }
                else
                {
                    MWMechanics::CastSpell cast(caster, obstacle);
                    cast.mHitPosition = pos;
                    cast.mId = it->mSpellId;
                    cast.mSourceName = it->mSourceName;
                    cast.mStack = it->mStack;
                    cast.inflict(obstacle, caster, it->mEffects, ESM::RT_Target, false, true);
                }

                hit = true;
            }

            // Explodes when hitting water
            if (MWBase::Environment::get().getWorld()->isUnderwater(MWBase::Environment::get().getWorld()->getPlayerPtr().getCell(), newPos))
                hit = true;

            if (hit)
            {
                MWWorld::Ptr caster = MWBase::Environment::get().getWorld()->searchPtrViaActorId(it->mActorId);
                MWBase::Environment::get().getWorld()->explodeSpell(pos, it->mEffects, caster, ESM::RT_Target, it->mSpellId, it->mSourceName);

                MWBase::Environment::get().getSoundManager()->stopSound(it->mSound);

                mSceneMgr->destroySceneNode(it->mNode);

                it = mMagicBolts.erase(it);
                continue;
            }
            else
                ++it;
        }
    }

    void ProjectileManager::moveProjectiles(float duration)
    {
        for (std::vector<ProjectileState>::iterator it = mProjectiles.begin(); it != mProjectiles.end();)
        {
            // gravity constant - must be way lower than the gravity affecting actors, since we're not
            // simulating aerodynamics at all
            it->mVelocity -= Ogre::Vector3(0, 0, 627.2f * 0.1f) * duration;

            Ogre::Vector3 pos(it->mNode->getPosition());
            Ogre::Vector3 newPos = pos + it->mVelocity * duration;

            Ogre::Quaternion orient = Ogre::Vector3::UNIT_Y.getRotationTo(it->mVelocity);
            it->mNode->setOrientation(orient);
            it->mNode->setPosition(newPos);

            update(it->mObject, duration);

            // Check for impact
            // TODO: use a proper btRigidBody / btGhostObject?
            btVector3 from(pos.x, pos.y, pos.z);
            btVector3 to(newPos.x, newPos.y, newPos.z);
            std::vector<std::pair<float, std::string> > collisions = mPhysEngine.rayTest2(from, to, OEngine::Physic::CollisionType_Projectile);
            bool hit=false;

            for (std::vector<std::pair<float, std::string> >::iterator cIt = collisions.begin(); cIt != collisions.end() && !hit; ++cIt)
            {
                MWWorld::Ptr obstacle = MWBase::Environment::get().getWorld()->searchPtrViaHandle(cIt->second);

                MWWorld::Ptr caster = MWBase::Environment::get().getWorld()->searchPtrViaActorId(it->mActorId);

                // Arrow intersects with player immediately after shooting :/
                if (obstacle == caster)
                    continue;

                MWWorld::ManualRef projectileRef(MWBase::Environment::get().getWorld()->getStore(), it->mId);

                // Try to get a Ptr to the bow that was used. It might no longer exist.
                MWWorld::Ptr bow = projectileRef.getPtr();
                if (!caster.isEmpty())
                {
                    MWWorld::InventoryStore& inv = caster.getClass().getInventoryStore(caster);
                    MWWorld::ContainerStoreIterator invIt = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                    if (invIt != inv.end() && Misc::StringUtils::ciEqual(invIt->getCellRef().getRefId(), it->mBowId))
                        bow = *invIt;
                }

                if (caster.isEmpty())
                    caster = obstacle;

                MWMechanics::projectileHit(caster, obstacle, bow, projectileRef.getPtr(), pos + (newPos - pos) * cIt->first);

                hit = true;
            }
            if (hit)
            {
                mSceneMgr->destroySceneNode(it->mNode);

                it = mProjectiles.erase(it);
                continue;
            }
            else
                ++it;
        }
    }

    void ProjectileManager::clear()
    {
        for (std::vector<ProjectileState>::iterator it = mProjectiles.begin(); it != mProjectiles.end(); ++it)
        {
            mSceneMgr->destroySceneNode(it->mNode);
        }
        mProjectiles.clear();
        for (std::vector<MagicBoltState>::iterator it = mMagicBolts.begin(); it != mMagicBolts.end(); ++it)
        {
            MWBase::Environment::get().getSoundManager()->stopSound(it->mSound);
            mSceneMgr->destroySceneNode(it->mNode);
        }
        mMagicBolts.clear();
    }

    void ProjectileManager::write(ESM::ESMWriter &writer, Loading::Listener &progress) const
    {
        for (std::vector<ProjectileState>::const_iterator it = mProjectiles.begin(); it != mProjectiles.end(); ++it)
        {
            writer.startRecord(ESM::REC_PROJ);

            ESM::ProjectileState state;
            state.mId = it->mId;
            state.mPosition = it->mNode->getPosition();
            state.mOrientation = it->mNode->getOrientation();
            state.mActorId = it->mActorId;

            state.mBowId = it->mBowId;
            state.mVelocity = it->mVelocity;

            state.save(writer);

            writer.endRecord(ESM::REC_PROJ);
        }

        for (std::vector<MagicBoltState>::const_iterator it = mMagicBolts.begin(); it != mMagicBolts.end(); ++it)
        {
            writer.startRecord(ESM::REC_MPRJ);

            ESM::MagicBoltState state;
            state.mId = it->mId;
            state.mPosition = it->mNode->getPosition();
            state.mOrientation = it->mNode->getOrientation();
            state.mActorId = it->mActorId;

            state.mSpellId = it->mSpellId;
            state.mEffects = it->mEffects;
            state.mSound = it->mSoundId;
            state.mSourceName = it->mSourceName;
            state.mSpeed = it->mSpeed;
            state.mStack = it->mStack;

            state.save(writer);

            writer.endRecord(ESM::REC_MPRJ);
        }
    }

    bool ProjectileManager::readRecord(ESM::ESMReader &reader, uint32_t type)
    {
        if (type == ESM::REC_PROJ)
        {
            ESM::ProjectileState esm;
            esm.load(reader);

            ProjectileState state;
            state.mActorId = esm.mActorId;
            state.mBowId = esm.mBowId;
            state.mVelocity = esm.mVelocity;
            state.mId = esm.mId;

            std::string model;
            try
            {
                MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), esm.mId);
                MWWorld::Ptr ptr = ref.getPtr();
                model = ptr.getClass().getModel(ptr);
            }
            catch(...)
            {
                return true;
            }

            state.mNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(esm.mPosition, esm.mOrientation);
            createModel(state, model);

            mProjectiles.push_back(state);
            return true;
        }
        else if (type == ESM::REC_MPRJ)
        {
            ESM::MagicBoltState esm;
            esm.load(reader);

            MagicBoltState state;
            state.mSourceName = esm.mSourceName;
            state.mId = esm.mId;
            state.mSpellId = esm.mSpellId;
            state.mActorId = esm.mActorId;
            state.mSpeed = esm.mSpeed;
            state.mStack = esm.mStack;
            state.mEffects = esm.mEffects;

            std::string model;
            try
            {
                MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), esm.mId);
                MWWorld::Ptr ptr = ref.getPtr();
                model = ptr.getClass().getModel(ptr);
            }
            catch(...)
            {
                return true;
            }

            state.mNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(esm.mPosition, esm.mOrientation);
            createModel(state, model);

            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            state.mSound = sndMgr->playManualSound3D(esm.mPosition, esm.mSound, 1.0f, 1.0f,
                                                     MWBase::SoundManager::Play_TypeSfx, MWBase::SoundManager::Play_Loop);
            state.mSoundId = esm.mSound;

            mMagicBolts.push_back(state);
            return true;
        }

        return false;
    }

    int ProjectileManager::countSavedGameRecords() const
    {
        return mMagicBolts.size() + mProjectiles.size();
    }

}
