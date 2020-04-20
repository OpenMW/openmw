#include "projectilemanager.hpp"

#include <iomanip>

#include <osg/PositionAttitudeTransform>

#include <components/debug/debuglog.hpp>

#include <components/esm/esmwriter.hpp>
#include <components/esm/projectilestate.hpp>

#include <components/misc/constants.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/lightmanager.hpp>

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
#include "../mwmechanics/aipackage.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "../mwrender/animation.hpp"
#include "../mwrender/vismask.hpp"
#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/util.hpp"

#include "../mwsound/sound.hpp"

#include "../mwphysics/physicssystem.hpp"

namespace
{
    ESM::EffectList getMagicBoltData(std::vector<std::string>& projectileIDs, std::set<std::string>& sounds, float& speed, std::string& texture, std::string& sourceName, const std::string& id)
    {
        const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();
        const ESM::EffectList* effects;
        if (const ESM::Spell* spell = esmStore.get<ESM::Spell>().search(id)) // check if it's a spell
        {
            sourceName = spell->mName;
            effects = &spell->mEffects;
        }
        else // check if it's an enchanted item
        {
            MWWorld::ManualRef ref(esmStore, id);
            MWWorld::Ptr ptr = ref.getPtr();
            const ESM::Enchantment* ench = esmStore.get<ESM::Enchantment>().find(ptr.getClass().getEnchantment(ptr));
            sourceName = ptr.getClass().getName(ptr);
            effects = &ench->mEffects;
        }

        int count = 0;
        speed = 0.0f;
        ESM::EffectList projectileEffects;
        for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects->mList.begin());
            iter!=effects->mList.end(); ++iter)
        {
            const ESM::MagicEffect *magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                iter->mEffectID);

            // Speed of multi-effect projectiles should be the average of the constituent effects,
            // based on observation of the original engine.
            speed += magicEffect->mData.mSpeed;
            count++;

            if (iter->mRange != ESM::RT_Target)
                continue;

            if (magicEffect->mBolt.empty())
                projectileIDs.push_back("VFX_DefaultBolt");
            else
                projectileIDs.push_back(magicEffect->mBolt);

            static const std::string schools[] = {
                "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
            };
            if (!magicEffect->mBoltSound.empty())
                sounds.emplace(magicEffect->mBoltSound);
            else
                sounds.emplace(schools[magicEffect->mData.mSchool] + " bolt");
            projectileEffects.mList.push_back(*iter);
        }
        
        if (count != 0)
            speed /= count;

        // the particle texture is only used if there is only one projectile
        if (projectileEffects.mList.size() == 1) 
        {
            const ESM::MagicEffect *magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                effects->mList.begin()->mEffectID);
            texture = magicEffect->mParticle;
        }
        
        if (projectileEffects.mList.size() > 1) // insert a VFX_Multiple projectile if there are multiple projectile effects
        {
            const std::string ID = "VFX_Multiple" + std::to_string(effects->mList.size());
            std::vector<std::string>::iterator it;
            it = projectileIDs.begin();
            it = projectileIDs.insert(it, ID);
        }
        return projectileEffects;
    }

    osg::Vec4 getMagicBoltLightDiffuseColor(const ESM::EffectList& effects)
    {
        // Calculate combined light diffuse color from magical effects
        osg::Vec4 lightDiffuseColor;
        float lightDiffuseRed = 0.0f;
        float lightDiffuseGreen = 0.0f;
        float lightDiffuseBlue = 0.0f;
        for (std::vector<ESM::ENAMstruct>::const_iterator iter(effects.mList.begin());
            iter != effects.mList.end(); ++iter)
        {
            const ESM::MagicEffect *magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(
                iter->mEffectID);
            lightDiffuseRed += (static_cast<float>(magicEffect->mData.mRed) / 255.f);
            lightDiffuseGreen += (static_cast<float>(magicEffect->mData.mGreen) / 255.f);
            lightDiffuseBlue += (static_cast<float>(magicEffect->mData.mBlue) / 255.f);
        }
        int numberOfEffects = effects.mList.size();
        lightDiffuseColor = osg::Vec4(lightDiffuseRed / numberOfEffects
            , lightDiffuseGreen / numberOfEffects
            , lightDiffuseBlue / numberOfEffects
            , 1.0f);

        return lightDiffuseColor;
    }
}

namespace MWWorld
{

    ProjectileManager::ProjectileManager(osg::Group* parent, Resource::ResourceSystem* resourceSystem,
                                         MWRender::RenderingManager* rendering, MWPhysics::PhysicsSystem* physics)
        : mParent(parent)
        , mResourceSystem(resourceSystem)
        , mRendering(rendering)
        , mPhysics(physics)
        , mCleanupTimer(0.0f)
    {

    }

    /// Rotates an osg::PositionAttitudeTransform over time.
    class RotateCallback : public osg::NodeCallback
    {
    public:
        RotateCallback(const osg::Vec3f& axis = osg::Vec3f(0,-1,0), float rotateSpeed = osg::PI*2)
            : mAxis(axis)
            , mRotateSpeed(rotateSpeed)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osg::PositionAttitudeTransform* transform = static_cast<osg::PositionAttitudeTransform*>(node);

            double time = nv->getFrameStamp()->getSimulationTime();

            osg::Quat orient = osg::Quat(time * mRotateSpeed, mAxis);
            transform->setAttitude(orient);

            traverse(node, nv);
        }

    private:
        osg::Vec3f mAxis;
        float mRotateSpeed;
    };


    void ProjectileManager::createModel(State &state, const std::string &model, const osg::Vec3f& pos, const osg::Quat& orient,
                                        bool rotate, bool createLight, osg::Vec4 lightDiffuseColor, std::string texture)
    {
        state.mNode = new osg::PositionAttitudeTransform;
        state.mNode->setNodeMask(MWRender::Mask_Effect);
        state.mNode->setPosition(pos);
        state.mNode->setAttitude(orient);

        osg::Group* attachTo = state.mNode;

        if (rotate)
        {
            osg::ref_ptr<osg::PositionAttitudeTransform> rotateNode (new osg::PositionAttitudeTransform);
            rotateNode->addUpdateCallback(new RotateCallback());
            state.mNode->addChild(rotateNode);
            attachTo = rotateNode;
        }

        osg::ref_ptr<osg::Node> projectile = mResourceSystem->getSceneManager()->getInstance(model, attachTo);

        if (state.mIdMagic.size() > 1)
            for (size_t iter = 1; iter != state.mIdMagic.size(); ++iter)
            {
                std::ostringstream nodeName;
                nodeName << "Dummy" << std::setw(2) << std::setfill('0') << iter;
                const ESM::Weapon* weapon = MWBase::Environment::get().getWorld()->getStore().get<ESM::Weapon>().find (state.mIdMagic.at(iter));
                SceneUtil::FindByNameVisitor findVisitor(nodeName.str());
                attachTo->accept(findVisitor);
                if (findVisitor.mFoundNode)
                    mResourceSystem->getSceneManager()->getInstance("meshes\\" + weapon->mModel, findVisitor.mFoundNode);
            }

        if (createLight)
        {
            osg::ref_ptr<osg::Light> projectileLight(new osg::Light);
            projectileLight->setAmbient(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
            projectileLight->setDiffuse(lightDiffuseColor);
            projectileLight->setSpecular(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
            projectileLight->setConstantAttenuation(0.f);
            projectileLight->setLinearAttenuation(0.1f);
            projectileLight->setQuadraticAttenuation(0.f);
            projectileLight->setPosition(osg::Vec4(pos, 1.0));
            
            SceneUtil::LightSource* projectileLightSource = new SceneUtil::LightSource;
            projectileLightSource->setNodeMask(MWRender::Mask_Lighting);
            projectileLightSource->setRadius(66.f);
            
            state.mNode->addChild(projectileLightSource);
            projectileLightSource->setLight(projectileLight);
        }
        
        SceneUtil::DisableFreezeOnCullVisitor disableFreezeOnCullVisitor;
        state.mNode->accept(disableFreezeOnCullVisitor);

        state.mNode->addCullCallback(new SceneUtil::LightListCallback);

        mParent->addChild(state.mNode);

        state.mEffectAnimationTime.reset(new MWRender::EffectAnimationTime);

        SceneUtil::AssignControllerSourcesVisitor assignVisitor (state.mEffectAnimationTime);
        state.mNode->accept(assignVisitor);

        MWRender::overrideFirstRootTexture(texture, mResourceSystem, projectile);
    }

    void ProjectileManager::update(State& state, float duration)
    {
        state.mEffectAnimationTime->addTime(duration);
    }

    void ProjectileManager::launchMagicBolt(const std::string &spellId, const Ptr &caster, const osg::Vec3f& fallbackDirection)
    {
        osg::Vec3f pos = caster.getRefData().getPosition().asVec3();
        if (caster.getClass().isActor())
        {
            // Spawn at 0.75 * ActorHeight
            // Note: we ignore the collision box offset, this is required to make some flying creatures work as intended.
            pos.z() += mPhysics->getRenderingHalfExtents(caster).z() * 2 * 0.75;
        }

        if (MWBase::Environment::get().getWorld()->isUnderwater(caster.getCell(), pos)) // Underwater casting not possible
            return;

        osg::Quat orient;
        if (caster.getClass().isActor())
            orient = osg::Quat(caster.getRefData().getPosition().rot[0], osg::Vec3f(-1,0,0))
                    * osg::Quat(caster.getRefData().getPosition().rot[2], osg::Vec3f(0,0,-1));
        else
            orient.makeRotate(osg::Vec3f(0,1,0), osg::Vec3f(fallbackDirection));

        MagicBoltState state;
        state.mSpellId = spellId;
        state.mCasterHandle = caster;
        if (caster.getClass().isActor())
            state.mActorId = caster.getClass().getCreatureStats(caster).getActorId();
        else
            state.mActorId = -1;

        std::string texture = "";

        state.mEffects = getMagicBoltData(state.mIdMagic, state.mSoundIds, state.mSpeed, texture, state.mSourceName, state.mSpellId);

        // Non-projectile should have been removed by getMagicBoltData
        if (state.mEffects.mList.empty())
            return;

        if (!caster.getClass().isActor() && fallbackDirection.length2() <= 0)
        {
            Log(Debug::Warning) << "Unable to launch magic bolt (direction to target is empty)";
            return;
        }

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), state.mIdMagic.at(0));
        MWWorld::Ptr ptr = ref.getPtr();

        osg::Vec4 lightDiffuseColor = getMagicBoltLightDiffuseColor(state.mEffects);
        createModel(state, ptr.getClass().getModel(ptr), pos, orient, true, true, lightDiffuseColor, texture);

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        for (const std::string &soundid : state.mSoundIds)
        {
            MWBase::Sound *sound = sndMgr->playSound3D(pos, soundid, 1.0f, 1.0f,
                                                       MWSound::Type::Sfx, MWSound::PlayMode::Loop);
            if (sound)
                state.mSounds.push_back(sound);
        }
            
        mMagicBolts.push_back(state);
    }

    void ProjectileManager::launchProjectile(Ptr actor, ConstPtr projectile, const osg::Vec3f &pos, const osg::Quat &orient, Ptr bow, float speed, float attackStrength)
    {
        ProjectileState state;
        state.mActorId = actor.getClass().getCreatureStats(actor).getActorId();
        state.mBowId = bow.getCellRef().getRefId();
        state.mVelocity = orient * osg::Vec3f(0,1,0) * speed;
        state.mIdArrow = projectile.getCellRef().getRefId();
        state.mCasterHandle = actor;
        state.mAttackStrength = attackStrength;

        int type = projectile.get<ESM::Weapon>()->mBase->mData.mType;
        state.mThrown = MWMechanics::getWeaponType(type)->mWeaponClass == ESM::WeaponType::Thrown;

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), projectile.getCellRef().getRefId());
        MWWorld::Ptr ptr = ref.getPtr();

        createModel(state, ptr.getClass().getModel(ptr), pos, orient, false, false, osg::Vec4(0,0,0,0));
        if (!ptr.getClass().getEnchantment(ptr).empty())
            SceneUtil::addEnchantedGlow(state.mNode, mResourceSystem, ptr.getClass().getEnchantmentColor(ptr));

        mProjectiles.push_back(state);
    }

    void ProjectileManager::update(float dt)
    {
        periodicCleanup(dt);
        moveProjectiles(dt);
        moveMagicBolts(dt);
    }

    void ProjectileManager::periodicCleanup(float dt)
    {
        mCleanupTimer -= dt;
        if (mCleanupTimer <= 0.0f)
        {
            mCleanupTimer = 2.0f;

            auto isCleanable = [](const ProjectileManager::State& state) -> bool
            {
                const float farawayThreshold = 72000.0f;
                osg::Vec3 playerPos = MWMechanics::getPlayer().getRefData().getPosition().asVec3();
                return (state.mNode->getPosition() - playerPos).length2() >= farawayThreshold*farawayThreshold;
            };

            for (std::vector<ProjectileState>::iterator it = mProjectiles.begin(); it != mProjectiles.end();)
            {
                if (isCleanable(*it))
                {
                    cleanupProjectile(*it);
                    it = mProjectiles.erase(it);
                }
                else
                    ++it;
            }

            for (std::vector<MagicBoltState>::iterator it = mMagicBolts.begin(); it != mMagicBolts.end();)
            {
                if (isCleanable(*it))
                {
                    cleanupMagicBolt(*it);
                    it = mMagicBolts.erase(it);
                }
                else
                    ++it;
            }
        }
    }

    void ProjectileManager::moveMagicBolts(float duration)
    {
        for (std::vector<MagicBoltState>::iterator it = mMagicBolts.begin(); it != mMagicBolts.end();)
        {
            osg::Quat orient = it->mNode->getAttitude();
            static float fTargetSpellMaxSpeed = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("fTargetSpellMaxSpeed")->mValue.getFloat();
            float speed = fTargetSpellMaxSpeed * it->mSpeed;
            osg::Vec3f direction = orient * osg::Vec3f(0,1,0);
            direction.normalize();
            osg::Vec3f pos(it->mNode->getPosition());
            osg::Vec3f newPos = pos + direction * duration * speed;

            for (size_t soundIter = 0; soundIter != it->mSounds.size(); soundIter++)
            {
                it->mSounds.at(soundIter)->setPosition(newPos);
            }

            it->mNode->setPosition(newPos);

            update(*it, duration);

            MWWorld::Ptr caster = it->getCaster();

            // For AI actors, get combat targets to use in the ray cast. Only those targets will return a positive hit result.
            std::vector<MWWorld::Ptr> targetActors;
            if (!caster.isEmpty() && caster.getClass().isActor() && caster != MWMechanics::getPlayer())
                caster.getClass().getCreatureStats(caster).getAiSequence().getCombatTargets(targetActors);

            // Check for impact
            // TODO: use a proper btRigidBody / btGhostObject?
            MWPhysics::PhysicsSystem::RayResult result = mPhysics->castRay(pos, newPos, caster, targetActors, 0xff, MWPhysics::CollisionType_Projectile);

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
                    cast.mStack = false;
                    cast.inflict(result.mHitObject, caster, it->mEffects, ESM::RT_Target, false, true);
                }
            }

            // Explodes when hitting water
            if (MWBase::Environment::get().getWorld()->isUnderwater(MWMechanics::getPlayer().getCell(), newPos))
                hit = true;

            if (hit)
            {
                MWBase::Environment::get().getWorld()->explodeSpell(pos, it->mEffects, caster, result.mHitObject,
                                                                    ESM::RT_Target, it->mSpellId, it->mSourceName);

                MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                for (size_t soundIter = 0; soundIter != it->mSounds.size(); soundIter++)
                    sndMgr->stopSound(it->mSounds.at(soundIter));

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
            it->mVelocity -= osg::Vec3f(0, 0, Constants::GravityConst * Constants::UnitsPerMeter * 0.1f) * duration;

            osg::Vec3f pos(it->mNode->getPosition());
            osg::Vec3f newPos = pos + it->mVelocity * duration;

            // rotation does not work well for throwing projectiles - their roll angle will depend on shooting direction.
            if (!it->mThrown)
            {
                osg::Quat orient;
                orient.makeRotate(osg::Vec3f(0,1,0), it->mVelocity);
                it->mNode->setAttitude(orient);
            }

            it->mNode->setPosition(newPos);

            update(*it, duration);

            MWWorld::Ptr caster = it->getCaster();

            // For AI actors, get combat targets to use in the ray cast. Only those targets will return a positive hit result.
            std::vector<MWWorld::Ptr> targetActors;
            if (!caster.isEmpty() && caster.getClass().isActor() && caster != MWMechanics::getPlayer())
                caster.getClass().getCreatureStats(caster).getAiSequence().getCombatTargets(targetActors);

            // Check for impact
            // TODO: use a proper btRigidBody / btGhostObject?
            MWPhysics::PhysicsSystem::RayResult result = mPhysics->castRay(pos, newPos, caster, targetActors, 0xff, MWPhysics::CollisionType_Projectile);

            bool underwater = MWBase::Environment::get().getWorld()->isUnderwater(MWMechanics::getPlayer().getCell(), newPos);

            if (result.mHit || underwater)
            {
                MWWorld::ManualRef projectileRef(MWBase::Environment::get().getWorld()->getStore(), it->mIdArrow);

                // Try to get a Ptr to the bow that was used. It might no longer exist.
                MWWorld::Ptr bow = projectileRef.getPtr();
                if (!caster.isEmpty() && it->mIdArrow != it->mBowId)
                {
                    MWWorld::InventoryStore& inv = caster.getClass().getInventoryStore(caster);
                    MWWorld::ContainerStoreIterator invIt = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                    if (invIt != inv.end() && Misc::StringUtils::ciEqual(invIt->getCellRef().getRefId(), it->mBowId))
                        bow = *invIt;
                }

                if (caster.isEmpty())
                    caster = result.mHitObject;

                MWMechanics::projectileHit(caster, result.mHitObject, bow, projectileRef.getPtr(), result.mHit ? result.mHitPos : newPos, it->mAttackStrength);

                if (underwater)
                    mRendering->emitWaterRipple(newPos);

                mParent->removeChild(it->mNode);
                it = mProjectiles.erase(it);
                continue;
            }

            ++it;
        }
    }

    void ProjectileManager::cleanupProjectile(ProjectileManager::ProjectileState& state)
    {
        mParent->removeChild(state.mNode);
    }

    void ProjectileManager::cleanupMagicBolt(ProjectileManager::MagicBoltState& state)
    {
        mParent->removeChild(state.mNode);
        for (size_t soundIter = 0; soundIter != state.mSounds.size(); soundIter++)
        {
            MWBase::Environment::get().getSoundManager()->stopSound(state.mSounds.at(soundIter));
        }
    }

    void ProjectileManager::clear()
    {
        for (std::vector<ProjectileState>::iterator it = mProjectiles.begin(); it != mProjectiles.end(); ++it)
        {
            cleanupProjectile(*it);
        }
        mProjectiles.clear();
        for (std::vector<MagicBoltState>::iterator it = mMagicBolts.begin(); it != mMagicBolts.end(); ++it)
        {
            cleanupMagicBolt(*it);
        }
        mMagicBolts.clear();
    }

    void ProjectileManager::write(ESM::ESMWriter &writer, Loading::Listener &progress) const
    {
        for (std::vector<ProjectileState>::const_iterator it = mProjectiles.begin(); it != mProjectiles.end(); ++it)
        {
            writer.startRecord(ESM::REC_PROJ);

            ESM::ProjectileState state;
            state.mId = it->mIdArrow;
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
            state.mId = it->mIdMagic.at(0);
            state.mPosition = ESM::Vector3(osg::Vec3f(it->mNode->getPosition()));
            state.mOrientation = ESM::Quaternion(osg::Quat(it->mNode->getAttitude()));
            state.mActorId = it->mActorId;

            state.mSpellId = it->mSpellId;
            state.mSpeed = it->mSpeed;

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
            state.mIdArrow = esm.mId;
            state.mAttackStrength = esm.mAttackStrength;

            std::string model;
            try
            {
                MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), esm.mId);
                MWWorld::Ptr ptr = ref.getPtr();
                model = ptr.getClass().getModel(ptr);

                int weaponType = ptr.get<ESM::Weapon>()->mBase->mData.mType;
                state.mThrown = MWMechanics::getWeaponType(weaponType)->mWeaponClass == ESM::WeaponType::Thrown;
            }
            catch(...)
            {
                return true;
            }

            createModel(state, model, osg::Vec3f(esm.mPosition), osg::Quat(esm.mOrientation), false, false, osg::Vec4(0,0,0,0));

            mProjectiles.push_back(state);
            return true;
        }
        else if (type == ESM::REC_MPRJ)
        {
            ESM::MagicBoltState esm;
            esm.load(reader);

            MagicBoltState state;
            state.mIdMagic.push_back(esm.mId);
            state.mSpellId = esm.mSpellId;
            state.mActorId = esm.mActorId;
            std::string texture = "";

            try
            {
                state.mEffects = getMagicBoltData(state.mIdMagic, state.mSoundIds, state.mSpeed, texture, state.mSourceName, state.mSpellId);
            }
            catch(...)
            {
                Log(Debug::Warning) << "Warning: Failed to recreate magic projectile from saved data (id \"" << state.mSpellId << "\" no longer exists?)";
                return true;
            }

            state.mSpeed = esm.mSpeed; // speed is derived from non-projectile effects as well as
                                       // projectile effects, so we can't calculate it from the save
                                       // file's effect list, which is already trimmed of non-projectile
                                       // effects. We need to use the stored value.

            std::string model;
            try
            {
                MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), state.mIdMagic.at(0));
                MWWorld::Ptr ptr = ref.getPtr();
                model = ptr.getClass().getModel(ptr);
            }
            catch(...)
            {
                return true;
            }

            osg::Vec4 lightDiffuseColor = getMagicBoltLightDiffuseColor(state.mEffects);
            createModel(state, model, osg::Vec3f(esm.mPosition), osg::Quat(esm.mOrientation), true, true, lightDiffuseColor, texture);

            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            for (const std::string &soundid : state.mSoundIds)
            {
                MWBase::Sound *sound = sndMgr->playSound3D(esm.mPosition, soundid, 1.0f, 1.0f,
                                                           MWSound::Type::Sfx, MWSound::PlayMode::Loop);
                if (sound)
                    state.mSounds.push_back(sound);
            }

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
