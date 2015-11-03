#include "projectilemanager.hpp"

#include <osg/PositionAttitudeTransform>

#include <components/esm/esmwriter.hpp>
#include <components/esm/projectilestate.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/controller.hpp>

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
#include "../mwmechanics/actorutil.hpp"

#include "../mwrender/effectmanager.hpp"
#include "../mwrender/animation.hpp"
#include "../mwrender/vismask.hpp"

#include "../mwsound/sound.hpp"

#include "../mwphysics/physicssystem.hpp"


namespace MWWorld
{

    ProjectileManager::ProjectileManager(osg::Group* parent, Resource::ResourceSystem* resourceSystem, MWPhysics::PhysicsSystem* physics)
        : mParent(parent)
        , mResourceSystem(resourceSystem)
        , mPhysics(physics)
    {

    }

    void ProjectileManager::createModel(State &state, const std::string &model, const osg::Vec3f& pos, const osg::Quat& orient)
    {
        state.mNode = new osg::PositionAttitudeTransform;
        state.mNode->setNodeMask(MWRender::Mask_Effect);
        state.mNode->setPosition(pos);
        state.mNode->setAttitude(orient);
        mParent->addChild(state.mNode);

        mResourceSystem->getSceneManager()->createInstance(model, state.mNode);

        state.mEffectAnimationTime.reset(new MWRender::EffectAnimationTime);

        SceneUtil::AssignControllerSourcesVisitor assignVisitor (state.mEffectAnimationTime);
        state.mNode->accept(assignVisitor);
    }

    void ProjectileManager::update(State& state, float duration)
    {
        state.mEffectAnimationTime->addTime(duration);
    }

    void ProjectileManager::launchMagicBolt(const std::string &model, const std::string &sound,
                                            const std::string &spellId, float speed, bool stack,
                                            const ESM::EffectList &effects, const Ptr &caster, const std::string &sourceName,
                                            const osg::Vec3f& fallbackDirection)
    {
        osg::Vec3f pos = mPhysics->getPosition(caster) + osg::Vec3f(0,0,mPhysics->getHalfExtents(caster).z() * 0.5); // Spawn at 0.75 * ActorHeight

        if (MWBase::Environment::get().getWorld()->isUnderwater(caster.getCell(), pos)) // Underwater casting not possible
            return;

        osg::Quat orient;
        if (caster.getClass().isActor())
            orient = osg::Quat(caster.getRefData().getPosition().rot[0], osg::Vec3f(-1,0,0))
                    * osg::Quat(caster.getRefData().getPosition().rot[2], osg::Vec3f(0,0,-1));
        else
            orient.makeRotate(osg::Vec3f(0,1,0), osg::Vec3f(fallbackDirection));

        MagicBoltState state;
        state.mSourceName = sourceName;
        state.mId = model;
        state.mSpellId = spellId;
        state.mCasterHandle = caster;
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

        createModel(state, ptr.getClass().getModel(ptr), pos, orient);

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        state.mSound = sndMgr->playManualSound3D(pos, sound, 1.0f, 1.0f, MWBase::SoundManager::Play_TypeSfx, MWBase::SoundManager::Play_Loop);

        mMagicBolts.push_back(state);
    }

    void ProjectileManager::launchProjectile(Ptr actor, Ptr projectile, const osg::Vec3f &pos, const osg::Quat &orient, Ptr bow, float speed, float attackStrength)
    {
        ProjectileState state;
        state.mActorId = actor.getClass().getCreatureStats(actor).getActorId();
        state.mBowId = bow.getCellRef().getRefId();
        state.mVelocity = orient * osg::Vec3f(0,1,0) * speed;
        state.mId = projectile.getCellRef().getRefId();
        state.mCasterHandle = actor;
        state.mAttackStrength = attackStrength;

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), projectile.getCellRef().getRefId());
        MWWorld::Ptr ptr = ref.getPtr();

        createModel(state, ptr.getClass().getModel(ptr), pos, orient);

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
            osg::Quat orient = it->mNode->getAttitude();
            static float fTargetSpellMaxSpeed = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("fTargetSpellMaxSpeed")->getFloat();
            float speed = fTargetSpellMaxSpeed * it->mSpeed;

            osg::Vec3f direction = orient * osg::Vec3f(0,1,0);
            direction.normalize();
            osg::Vec3f pos(it->mNode->getPosition());
            osg::Vec3f newPos = pos + direction * duration * speed;

            if (it->mSound.get())
                it->mSound->setPosition(newPos);

            it->mNode->setPosition(newPos);

            update(*it, duration);

            MWWorld::Ptr caster = it->getCaster();

            // Check for impact
            // TODO: use a proper btRigidBody / btGhostObject?
            MWPhysics::PhysicsSystem::RayResult result = mPhysics->castRay(pos, newPos, caster, 0xff, MWPhysics::CollisionType_Projectile);

            bool hit = false;
            if (result.mHit)
            {
                hit = true;
                if (result.mHitObject.isEmpty())
                {
                    // terrain
                }
                else
                {
                    MWMechanics::CastSpell cast(caster, result.mHitObject);
                    cast.mHitPosition = pos;
                    cast.mId = it->mSpellId;
                    cast.mSourceName = it->mSourceName;
                    cast.mStack = it->mStack;
                    cast.inflict(result.mHitObject, caster, it->mEffects, ESM::RT_Target, false, true);
                }
            }

            // Explodes when hitting water
            if (MWBase::Environment::get().getWorld()->isUnderwater(MWMechanics::getPlayer().getCell(), newPos))
                hit = true;

            if (hit)
            {
                MWBase::Environment::get().getWorld()->explodeSpell(pos, it->mEffects, caster, ESM::RT_Target, it->mSpellId, it->mSourceName);

                MWBase::Environment::get().getSoundManager()->stopSound(it->mSound);

                mParent->removeChild(it->mNode);

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
            it->mVelocity -= osg::Vec3f(0, 0, 627.2f * 0.1f) * duration;

            osg::Vec3f pos(it->mNode->getPosition());
            osg::Vec3f newPos = pos + it->mVelocity * duration;

            osg::Quat orient;
            orient.makeRotate(osg::Vec3f(0,1,0), it->mVelocity);
            it->mNode->setAttitude(orient);
            it->mNode->setPosition(newPos);

            update(*it, duration);

            MWWorld::Ptr caster = it->getCaster();

            // Check for impact
            // TODO: use a proper btRigidBody / btGhostObject?
            MWPhysics::PhysicsSystem::RayResult result = mPhysics->castRay(pos, newPos, caster, 0xff, MWPhysics::CollisionType_Projectile);

            if (result.mHit)
            {
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
                    caster = result.mHitObject;

                MWMechanics::projectileHit(caster, result.mHitObject, bow, projectileRef.getPtr(), result.mHitPos, it->mAttackStrength);

                mParent->removeChild(it->mNode);

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
            mParent->removeChild(it->mNode);
        }
        mProjectiles.clear();
        for (std::vector<MagicBoltState>::iterator it = mMagicBolts.begin(); it != mMagicBolts.end(); ++it)
        {
            mParent->removeChild(it->mNode);
            MWBase::Environment::get().getSoundManager()->stopSound(it->mSound);
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
            state.mPosition = ESM::Vector3(osg::Vec3f(it->mNode->getPosition()));
            state.mOrientation = ESM::Quaternion(osg::Quat(it->mNode->getAttitude()));
            state.mActorId = it->mActorId;

            state.mBowId = it->mBowId;
            state.mVelocity = it->mVelocity;
            state.mAttackStrength = it->mAttackStrength;

            state.save(writer);

            writer.endRecord(ESM::REC_PROJ);
        }

        for (std::vector<MagicBoltState>::const_iterator it = mMagicBolts.begin(); it != mMagicBolts.end(); ++it)
        {
            writer.startRecord(ESM::REC_MPRJ);

            ESM::MagicBoltState state;
            state.mId = it->mId;
            state.mPosition = ESM::Vector3(osg::Vec3f(it->mNode->getPosition()));
            state.mOrientation = ESM::Quaternion(osg::Quat(it->mNode->getAttitude()));
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
            state.mAttackStrength = esm.mAttackStrength;

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

            createModel(state, model, osg::Vec3f(esm.mPosition), osg::Quat(esm.mOrientation));

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

            createModel(state, model, osg::Vec3f(esm.mPosition), osg::Quat(esm.mOrientation));

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

    MWWorld::Ptr ProjectileManager::State::getCaster()
    {
        if (!mCasterHandle.isEmpty())
            return mCasterHandle;

        return MWBase::Environment::get().getWorld()->searchPtrViaActorId(mActorId);
    }

}
