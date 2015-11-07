#include "sky.hpp"

#include <OgreCamera.h>
#include <OgreRenderWindow.h>
#include <OgreSceneNode.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreParticle.h>
#include <OgreParticleSystem.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreTechnique.h>
#include <OgreControllerManager.h>

#include <OgreMeshManager.h>

#include <boost/lexical_cast.hpp>

#include <openengine/misc/rng.hpp>

#include <components/nifogre/ogrenifloader.hpp>
#include <components/misc/resourcehelpers.hpp>

#include <extern/shiny/Platforms/Ogre/OgreMaterial.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/fallback.hpp"

#include "renderconst.hpp"
#include "renderingmanager.hpp"

using namespace MWRender;
using namespace Ogre;

namespace
{

void setAlpha (NifOgre::ObjectScenePtr scene, Ogre::MovableObject* movable, float alpha)
{
    Ogre::MaterialPtr mat = scene->mMaterialControllerMgr.getWritableMaterial(movable);
    Ogre::Material::TechniqueIterator techs = mat->getTechniqueIterator();
    while(techs.hasMoreElements())
    {
        Ogre::Technique *tech = techs.getNext();
        Ogre::Technique::PassIterator passes = tech->getPassIterator();
        while(passes.hasMoreElements())
        {
            Ogre::Pass *pass = passes.getNext();
            Ogre::ColourValue diffuse = pass->getDiffuse();
            diffuse.a = alpha;
            pass->setDiffuse(diffuse);
        }
    }

}

void setAlpha (NifOgre::ObjectScenePtr scene, float alpha)
{
    for(size_t i = 0; i < scene->mParticles.size(); ++i)
        setAlpha(scene, scene->mParticles[i], alpha);
    for(size_t i = 0; i < scene->mEntities.size(); ++i)
    {
        if (scene->mEntities[i] != scene->mSkelBase)
            setAlpha(scene, scene->mEntities[i], alpha);
    }
}

}

BillboardObject::BillboardObject( const String& textureName,
                    const float initialSize,
                    const Vector3& position,
                    SceneNode* rootNode,
                    const std::string& material)
: mVisibility(1.0f)
{
    SceneManager* sceneMgr = rootNode->getCreator();

    Vector3 finalPosition = position.normalisedCopy() * 1000.f;

    static unsigned int bodyCount=0;

    mMaterial = sh::Factory::getInstance().createMaterialInstance ("BillboardMaterial"+StringConverter::toString(bodyCount), material);
    mMaterial->setProperty("texture", sh::makeProperty(new sh::StringValue(textureName)));

    static Ogre::Mesh* plane = MeshManager::getSingleton().createPlane("billboard",
                                                                       ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  Ogre::Plane(Ogre::Vector3(0,0,1), 0), 1, 1, 1, 1, true, 1, 1, 1, Vector3::UNIT_Y).get();
    plane->_setBounds(Ogre::AxisAlignedBox::BOX_INFINITE);
    mEntity = sceneMgr->createEntity("billboard");
    mEntity->setMaterialName("BillboardMaterial"+StringConverter::toString(bodyCount));
    mEntity->setVisibilityFlags(RV_Sky);
    mEntity->setCastShadows(false);

    mNode = rootNode->createChildSceneNode();
    mNode->setPosition(finalPosition);
    mNode->attachObject(mEntity);
    mNode->setScale(Ogre::Vector3(450.f*initialSize));
    mNode->setOrientation(Ogre::Vector3::UNIT_Z.getRotationTo(-position.normalisedCopy()));

    sh::Factory::getInstance().getMaterialInstance ("BillboardMaterial"+StringConverter::toString(bodyCount))->setListener(this);

    bodyCount++;
}

void BillboardObject::requestedConfiguration (sh::MaterialInstance* m, const std::string& configuration)
{
}

void BillboardObject::createdConfiguration (sh::MaterialInstance* m, const std::string& configuration)
{
    setVisibility(mVisibility);
    setColour(mColour);
}

void BillboardObject::setVisible(const bool visible)
{
    mEntity->setVisible(visible);
}

void BillboardObject::setSize(const float size)
{
    mNode->setScale(450.f*size, 450.f*size, 450.f*size);
}

void BillboardObject::setVisibility(const float visibility)
{
    mVisibility = visibility;
    Ogre::MaterialPtr m = static_cast<sh::OgreMaterial*>(mMaterial->getMaterial ())->getOgreMaterial ();
    for (int i=0; i<m->getNumTechniques(); ++i)
    {
        Ogre::Technique* t = m->getTechnique(i);
        if (t->getNumPasses ())
            t->getPass(0)->setDiffuse (0,0,0, visibility);
    }
}

void BillboardObject::setPosition(const Vector3& pPosition)
{
    Vector3 normalised = pPosition.normalisedCopy();
    Vector3 finalPosition = normalised * 1000.f;
    mNode->setOrientation(Ogre::Vector3::UNIT_Z.getRotationTo(-normalised));
    mNode->setPosition(finalPosition);
}

Vector3 BillboardObject::getPosition() const
{
    return mNode->getPosition();
}

void BillboardObject::setVisibilityFlags(int flags)
{
    mEntity->setVisibilityFlags(flags);
}

void BillboardObject::setColour(const ColourValue& pColour)
{
    mColour = pColour;
    Ogre::MaterialPtr m = static_cast<sh::OgreMaterial*>(mMaterial->getMaterial ())->getOgreMaterial ();
    for (int i=0; i<m->getNumTechniques(); ++i)
    {
        Ogre::Technique* t = m->getTechnique(i);
        if (t->getNumPasses ())
            t->getPass(0)->setSelfIllumination (pColour);
    }
}

void BillboardObject::setRenderQueue(unsigned int id)
{
    mEntity->setRenderQueueGroup(id);
}

SceneNode* BillboardObject::getNode()
{
    return mNode;
}

Moon::Moon( const String& textureName,
                    const float initialSize,
                    const Vector3& position,
                    SceneNode* rootNode,
            const std::string& material)
    : BillboardObject(textureName, initialSize, position, rootNode, material)
    , mType(Type_Masser)
{
    setVisibility(1.0);

    mMaterial->setProperty("alphatexture", sh::makeProperty(new sh::StringValue(textureName + "_alpha")));

    mPhase = Moon::Phase_Full;
}

void Moon::setType(const Moon::Type& type)
{
    mType = type;
}

void Moon::setPhase(const Moon::Phase& phase)
{
    // Colour texture
    Ogre::String textureName = "textures\\tx_";

    if (mType == Moon::Type_Secunda) textureName += "secunda_";
    else textureName += "masser_";

    if      (phase == Moon::Phase_New)              textureName += "new";
    else if (phase == Moon::Phase_WaxingCrescent)   textureName += "one_wax";
    else if (phase == Moon::Phase_WaxingHalf)       textureName += "half_wax";
    else if (phase == Moon::Phase_WaxingGibbous)    textureName += "three_wax";
    else if (phase == Moon::Phase_WaningCrescent)   textureName += "one_wan";
    else if (phase == Moon::Phase_WaningHalf)       textureName += "half_wan";
    else if (phase == Moon::Phase_WaningGibbous)    textureName += "three_wan";
    else if (phase == Moon::Phase_Full)             textureName += "full";

    textureName += ".dds";

    if (mType == Moon::Type_Secunda)
    {
        sh::Factory::getInstance ().setTextureAlias ("secunda_texture", textureName);
        sh::Factory::getInstance ().setTextureAlias ("secunda_texture_alpha", "textures\\tx_mooncircle_full_s.dds");
    }
    else
    {
        sh::Factory::getInstance ().setTextureAlias ("masser_texture", textureName);
        sh::Factory::getInstance ().setTextureAlias ("masser_texture_alpha", "textures\\tx_mooncircle_full_m.dds");
    }

    mPhase = phase;
}

unsigned int Moon::getPhaseInt() const
{
    if      (mPhase == Moon::Phase_New)              return 0;
    else if (mPhase == Moon::Phase_WaxingCrescent)   return 1;
    else if (mPhase == Moon::Phase_WaningCrescent)   return 1;
    else if (mPhase == Moon::Phase_WaxingHalf)       return 2;
    else if (mPhase == Moon::Phase_WaningHalf)       return 2;
    else if (mPhase == Moon::Phase_WaxingGibbous)    return 3;
    else if (mPhase == Moon::Phase_WaningGibbous)    return 3;
    else if (mPhase == Moon::Phase_Full)             return 4;

    return 0;
}

SkyManager::SkyManager(Ogre::SceneNode *root, Ogre::Camera *pCamera)
    : mCreated(false)
    , mMoonRed(false)
    , mIsStorm(false)
    , mHour(0.0f)
    , mDay(0)
    , mMonth(0)
    , mCloudAnimationTimer(0.f)
    , mSun(NULL)
    , mSunGlare(NULL)
    , mMasser(NULL)
    , mSecunda(NULL)
    , mCamera(pCamera)
    , mRootNode(NULL)
    , mSceneMgr(NULL)
    , mAtmosphereDay(NULL)
    , mAtmosphereNight(NULL)
    , mCloudNode(NULL)
    , mParticleNode(NULL)
    , mRainTimer(0)
    , mStormDirection(0,-1,0)
    , mClouds()
    , mNextClouds()
    , mCloudBlendFactor(0.0f)
    , mCloudOpacity(0.0f)
    , mCloudSpeed(0.0f)
    , mStarsOpacity(0.0f)
    , mLightning(NULL)
    , mRemainingTransitionTime(0.0f)
    , mGlare(0.0f)
    , mGlareFade(0.0f)
    , mRainEnabled(false)
    , mRainSpeed(0)
    , mRainFrequency(1)
    , mEnabled(true)
    , mSunEnabled(true)
    , mMasserEnabled(true)
    , mSecundaEnabled(true)
{
    mSceneMgr = root->getCreator();
    mRootNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
}

void SkyManager::create()
{
    assert(!mCreated);

    sh::Factory::getInstance().setSharedParameter ("cloudBlendFactor",
        sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));
    sh::Factory::getInstance().setSharedParameter ("cloudOpacity",
        sh::makeProperty<sh::FloatValue>(new sh::FloatValue(1)));
    sh::Factory::getInstance().setSharedParameter ("cloudColour",
        sh::makeProperty<sh::Vector3>(new sh::Vector3(1,1,1)));
    sh::Factory::getInstance().setSharedParameter ("cloudAnimationTimer",
        sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));
    sh::Factory::getInstance().setSharedParameter ("nightFade",
        sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));
    sh::Factory::getInstance().setSharedParameter ("atmosphereColour", sh::makeProperty<sh::Vector4>(new sh::Vector4(0,0,0,1)));
    sh::Factory::getInstance().setSharedParameter ("horizonColour", sh::makeProperty<sh::Vector4>(new sh::Vector4(0,0,0,1)));

    sh::Factory::getInstance().setTextureAlias ("cloud_texture_1", "");
    sh::Factory::getInstance().setTextureAlias ("cloud_texture_2", "");

    // Create light used for thunderstorm
    mLightning = mSceneMgr->createLight();
    mLightning->setType (Ogre::Light::LT_DIRECTIONAL);
    mLightning->setDirection (Ogre::Vector3(0.3f, -0.7f, 0.3f));
    mLightning->setVisible (false);
    mLightning->setDiffuseColour (ColourValue(3,3,3));

    const MWWorld::Fallback* fallback=MWBase::Environment::get().getWorld()->getFallback();
    mSecunda = new Moon("secunda_texture", fallback->getFallbackFloat("Moons_Secunda_Size")/100, Vector3(-0.4f, 0.4f, 0.5f), mRootNode, "openmw_moon");
    mSecunda->setType(Moon::Type_Secunda);
    mSecunda->setRenderQueue(RQG_SkiesEarly+4);

    mMasser = new Moon("masser_texture", fallback->getFallbackFloat("Moons_Masser_Size")/100, Vector3(-0.4f, 0.4f, 0.5f), mRootNode, "openmw_moon");
    mMasser->setRenderQueue(RQG_SkiesEarly+3);
    mMasser->setType(Moon::Type_Masser);

    mSun = new BillboardObject("textures\\tx_sun_05.dds", 1, Vector3(0.4f, 0.4f, 0.4f), mRootNode, "openmw_sun");
    mSun->setRenderQueue(RQG_SkiesEarly+4);
    mSunGlare = new BillboardObject("textures\\tx_sun_flash_grey_05.dds", 3, Vector3(0.4f, 0.4f, 0.4f), mRootNode, "openmw_sun");
    mSunGlare->setRenderQueue(RQG_SkiesLate);
    mSunGlare->setVisibilityFlags(RV_NoReflection);

    Ogre::AxisAlignedBox aabInf = Ogre::AxisAlignedBox::BOX_INFINITE;

    // Stars
    mAtmosphereNight = mRootNode->createChildSceneNode();
    NifOgre::ObjectScenePtr objects;
    if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup("meshes\\sky_night_02.nif"))
        objects = NifOgre::Loader::createObjects(mAtmosphereNight, "meshes\\sky_night_02.nif");
    else
        objects = NifOgre::Loader::createObjects(mAtmosphereNight, "meshes\\sky_night_01.nif");

    for(size_t i = 0, matidx = 0;i < objects->mEntities.size();i++)
    {
        Entity* night1_ent = objects->mEntities[i];
        night1_ent->setRenderQueueGroup(RQG_SkiesEarly+1);
        night1_ent->setVisibilityFlags(RV_Sky);
        night1_ent->setCastShadows(false);
        night1_ent->getMesh()->_setBounds (aabInf);

        for (unsigned int j=0; j<night1_ent->getNumSubEntities(); ++j)
        {
            std::string matName = "openmw_stars_" + boost::lexical_cast<std::string>(matidx++);
            sh::MaterialInstance* m = sh::Factory::getInstance().createMaterialInstance(matName, "openmw_stars");

            std::string textureName = sh::retrieveValue<sh::StringValue>(
                        sh::Factory::getInstance().getMaterialInstance(night1_ent->getSubEntity(j)->getMaterialName())->getProperty("diffuseMap"), NULL).get();

            m->setProperty("texture", sh::makeProperty<sh::StringValue>(new sh::StringValue(textureName)));

            night1_ent->getSubEntity(j)->setMaterialName(matName);
        }
    }
    mObjects.push_back(objects);

    // Atmosphere (day)
    mAtmosphereDay = mRootNode->createChildSceneNode();
    objects = NifOgre::Loader::createObjects(mAtmosphereDay, "meshes\\sky_atmosphere.nif");
    for(size_t i = 0;i < objects->mEntities.size();i++)
    {
        Entity* atmosphere_ent = objects->mEntities[i];
        atmosphere_ent->setCastShadows(false);
        atmosphere_ent->setRenderQueueGroup(RQG_SkiesEarly);
        atmosphere_ent->setVisibilityFlags(RV_Sky);

        for(unsigned int j = 0;j < atmosphere_ent->getNumSubEntities();j++)
            atmosphere_ent->getSubEntity (j)->setMaterialName("openmw_atmosphere");

        // Using infinite AAB here to prevent being clipped by the custom near clip plane used for reflections/refractions
        atmosphere_ent->getMesh()->_setBounds (aabInf);
    }
    mObjects.push_back(objects);

    // Clouds
    mCloudNode = mRootNode->createChildSceneNode();
    objects = NifOgre::Loader::createObjects(mCloudNode, "meshes\\sky_clouds_01.nif");
    for(size_t i = 0;i < objects->mEntities.size();i++)
    {
        Entity* clouds_ent = objects->mEntities[i];
        clouds_ent->setVisibilityFlags(RV_Sky);
        clouds_ent->setRenderQueueGroup(RQG_SkiesEarly+5);
        for(unsigned int j = 0;j < clouds_ent->getNumSubEntities();j++)
            clouds_ent->getSubEntity(j)->setMaterialName("openmw_clouds");
        clouds_ent->setCastShadows(false);
        // Using infinite AAB here to prevent being clipped by the custom near clip plane used for reflections/refractions
        clouds_ent->getMesh()->_setBounds (aabInf);
    }
    mObjects.push_back(objects);

    mCreated = true;
}

SkyManager::~SkyManager()
{
    clearRain();
    delete mSun;
    delete mSunGlare;
    delete mMasser;
    delete mSecunda;
}

int SkyManager::getMasserPhase() const
{
    if (!mCreated) return 0;
    return mMasser->getPhaseInt();
}

int SkyManager::getSecundaPhase() const
{
    if (!mCreated) return 0;
    return mSecunda->getPhaseInt();
}

void SkyManager::clearRain()
{
    for (std::map<Ogre::SceneNode*, NifOgre::ObjectScenePtr>::iterator it = mRainModels.begin(); it != mRainModels.end();)
    {
        it->second.setNull();
        mSceneMgr->destroySceneNode(it->first);
        mRainModels.erase(it++);
    }
}

void SkyManager::updateRain(float dt)
{
    // Move existing rain
    // Note: if rain gets disabled, we let the existing rain drops finish falling down.
    float minHeight = 200;
    for (std::map<Ogre::SceneNode*, NifOgre::ObjectScenePtr>::iterator it = mRainModels.begin(); it != mRainModels.end();)
    {
        Ogre::Vector3 pos = it->first->getPosition();
        pos.z -= mRainSpeed * dt;
        it->first->setPosition(pos);
        if (pos.z < -minHeight
                // Here we might want to add a "splash" effect later
                || MWBase::Environment::get().getWorld()->isUnderwater(
                    MWBase::Environment::get().getWorld()->getPlayerPtr().getCell(), it->first->_getDerivedPosition()))
        {
            it->second.setNull();
            mSceneMgr->destroySceneNode(it->first);
            mRainModels.erase(it++);
        }
        else
            ++it;
    }

    // Spawn new rain
    float rainFrequency = mRainFrequency;
    if (mRainEnabled)
    {
        mRainTimer += dt;
        if (mRainTimer >= 1.f/rainFrequency)
        {
            mRainTimer = 0;

            // TODO: handle rain settings from Morrowind.ini
            const float rangeRandom = 100;
            float xOffs = OEngine::Misc::Rng::rollProbability() * rangeRandom - (rangeRandom / 2);
            float yOffs = OEngine::Misc::Rng::rollProbability() * rangeRandom - (rangeRandom / 2);

            // Create a separate node to control the offset, since a node with setInheritOrientation(false) will still
            // consider the orientation of the parent node for its position, just not for its orientation
            float startHeight = 700;
            Ogre::Vector3 worldPos = mParticleNode->_getDerivedPosition();
            worldPos += Ogre::Vector3(xOffs, yOffs, startHeight);
            if (MWBase::Environment::get().getWorld()->isUnderwater(
                                        MWBase::Environment::get().getWorld()->getPlayerPtr().getCell(), worldPos))
                return;

            Ogre::SceneNode* offsetNode = mParticleNode->createChildSceneNode(Ogre::Vector3(xOffs,yOffs,startHeight));

            // Spawn a new rain object for each instance.
            // TODO: this is inefficient. We could try to use an Ogre::ParticleSystem instead, but then we would need to make assumptions
            // about the rain meshes being Quads and their dimensions.
            // Or we could clone meshes into one vertex buffer manually.
            NifOgre::ObjectScenePtr objects = NifOgre::Loader::createObjects(offsetNode, mRainEffect);
            for (unsigned int i=0; i<objects->mEntities.size(); ++i)
            {
                objects->mEntities[i]->setRenderQueueGroup(RQG_Alpha);
                objects->mEntities[i]->setVisibilityFlags(RV_Sky);
            }
            for (unsigned int i=0; i<objects->mParticles.size(); ++i)
            {
                objects->mParticles[i]->setRenderQueueGroup(RQG_Alpha);
                objects->mParticles[i]->setVisibilityFlags(RV_Sky);
            }
            mRainModels[offsetNode] = objects;
        }
    }
}

void SkyManager::update(float duration)
{
    if (!mEnabled) return;
    const MWWorld::Fallback* fallback=MWBase::Environment::get().getWorld()->getFallback();

    if (!mParticle.isNull())
    {
        for (unsigned int i=0; i<mParticle->mControllers.size(); ++i)
            mParticle->mControllers[i].update();

        for (unsigned int i=0; i<mParticle->mParticles.size(); ++i)
        {
            Ogre::ParticleSystem* psys = mParticle->mParticles[i];
            Ogre::ParticleIterator pi = psys->_getIterator();
            while (!pi.end())
            {
                Ogre::Particle *p = pi.getNext();
                #if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
                Ogre::Vector3 pos = p->mPosition;
                Ogre::Real& timeToLive = p->mTimeToLive;
                #else
                Ogre::Vector3 pos = p->position;
                Ogre::Real& timeToLive = p->timeToLive;
                #endif

                if (psys->getKeepParticlesInLocalSpace() && psys->getParentNode())
                    pos = psys->getParentNode()->convertLocalToWorldPosition(pos);

                if (MWBase::Environment::get().getWorld()->isUnderwater(
                            MWBase::Environment::get().getWorld()->getPlayerPtr().getCell(), pos))
                    timeToLive = 0;
            }
        }

        if (mIsStorm)
            mParticleNode->setOrientation(Ogre::Vector3::UNIT_Y.getRotationTo(mStormDirection));
    }

    if (mIsStorm)
        mCloudNode->setOrientation(Ogre::Vector3::UNIT_Y.getRotationTo(mStormDirection));
    else
        mCloudNode->setOrientation(Ogre::Quaternion::IDENTITY);

    updateRain(duration);

    // UV Scroll the clouds
    mCloudAnimationTimer += duration * mCloudSpeed;
    sh::Factory::getInstance().setSharedParameter ("cloudAnimationTimer",
        sh::makeProperty<sh::FloatValue>(new sh::FloatValue(mCloudAnimationTimer)));

    /// \todo improve this
    mMasser->setPhase( static_cast<Moon::Phase>( (int) ((mDay % 32)/4.f)) );
    mSecunda->setPhase ( static_cast<Moon::Phase>( (int) ((mDay % 32)/4.f)) );

    mSecunda->setColour ( mMoonRed ? fallback->getFallbackColour("Moons_Script_Color") : ColourValue(1,1,1,1));
    mMasser->setColour (ColourValue(1,1,1,1));

    if (mSunEnabled)
    {
        // take 1/10 sec for fading the glare effect from invisible to full
        if (mGlareFade > mGlare)
        {
            mGlareFade -= duration*10;
            if (mGlareFade < mGlare) mGlareFade = mGlare;
        }
        else if (mGlareFade < mGlare)
        {
            mGlareFade += duration*10;
            if (mGlareFade > mGlare) mGlareFade = mGlare;
        }

        // increase the strength of the sun glare effect depending
        // on how directly the player is looking at the sun
        Vector3 sun = mSunGlare->getPosition();
        Vector3 cam = mCamera->getRealDirection();
        const Degree angle = sun.angleBetween( cam );
        float val = 1- (angle.valueDegrees() / 180.f);
        val = (val*val*val*val)*6;
        mSunGlare->setSize(val * mGlareFade);
    }

    mSunGlare->setVisible(mSunEnabled);
    mSun->setVisible(mSunEnabled);
    mMasser->setVisible(mMasserEnabled);
    mSecunda->setVisible(mSecundaEnabled);

    // rotate the stars by 360 degrees every 4 days
    mAtmosphereNight->roll(Degree(MWBase::Environment::get().getWorld()->getTimeScaleFactor()*duration*360 / (3600*96.f)));
}

void SkyManager::enable()
{
    if (!mCreated)
        create();

    if (mParticleNode)
        mParticleNode->setVisible(true);

    mRootNode->setVisible(true);
    mEnabled = true;
}

void SkyManager::disable()
{
    if (mParticleNode)
        mParticleNode->setVisible(false);

    clearRain();

    mRootNode->setVisible(false);

    mEnabled = false;
}

void SkyManager::setMoonColour (bool red)
{
    mMoonRed = red;
}

void SkyManager::setWeather(const MWWorld::WeatherResult& weather)
{
    if (!mCreated) return;

    mRainEffect = weather.mRainEffect;
    mRainEnabled = !mRainEffect.empty();
    mRainFrequency = weather.mRainFrequency;
    mRainSpeed = weather.mRainSpeed;
    mIsStorm = weather.mIsStorm;

    if (mCurrentParticleEffect != weather.mParticleEffect)
    {
        mCurrentParticleEffect = weather.mParticleEffect;

        if (mCurrentParticleEffect.empty())
        {
            mParticle.setNull();
        }
        else
        {
            mParticle = NifOgre::Loader::createObjects(mParticleNode, mCurrentParticleEffect);
            for(size_t i = 0; i < mParticle->mParticles.size(); ++i)
            {
                ParticleSystem* particle = mParticle->mParticles[i];
                particle->setRenderQueueGroup(RQG_Alpha);
                particle->setVisibilityFlags(RV_Sky);
            }
            for (size_t i = 0; i < mParticle->mControllers.size(); ++i)
            {
                if (mParticle->mControllers[i].getSource().isNull())
                    mParticle->mControllers[i].setSource(Ogre::ControllerManager::getSingleton().getFrameTimeSource());
            }
        }
    }

    if (mClouds != weather.mCloudTexture)
    {
        sh::Factory::getInstance().setTextureAlias ("cloud_texture_1", Misc::ResourceHelpers::correctTexturePath(weather.mCloudTexture));
        mClouds = weather.mCloudTexture;
    }

    if (mNextClouds != weather.mNextCloudTexture)
    {
        sh::Factory::getInstance().setTextureAlias ("cloud_texture_2", Misc::ResourceHelpers::correctTexturePath(weather.mNextCloudTexture));
        mNextClouds = weather.mNextCloudTexture;
    }

    if (mCloudBlendFactor != weather.mCloudBlendFactor)
    {
        mCloudBlendFactor = weather.mCloudBlendFactor;
        sh::Factory::getInstance().setSharedParameter ("cloudBlendFactor",
            sh::makeProperty<sh::FloatValue>(new sh::FloatValue(weather.mCloudBlendFactor)));
    }

    if (mCloudOpacity != weather.mCloudOpacity)
    {
        mCloudOpacity = weather.mCloudOpacity;
        sh::Factory::getInstance().setSharedParameter ("cloudOpacity",
            sh::makeProperty<sh::FloatValue>(new sh::FloatValue(weather.mCloudOpacity)));
    }

    if (mCloudColour != weather.mSunColor)
    {
        ColourValue clr( weather.mSunColor.r*0.7f + weather.mAmbientColor.r*0.7f,
                        weather.mSunColor.g*0.7f + weather.mAmbientColor.g*0.7f,
                        weather.mSunColor.b*0.7f + weather.mAmbientColor.b*0.7f);

        sh::Factory::getInstance().setSharedParameter ("cloudColour",
            sh::makeProperty<sh::Vector3>(new sh::Vector3(clr.r, clr.g, clr.b)));

        mCloudColour = weather.mSunColor;
    }

    if (mSkyColour != weather.mSkyColor)
    {
        mSkyColour = weather.mSkyColor;
        sh::Factory::getInstance().setSharedParameter ("atmosphereColour", sh::makeProperty<sh::Vector4>(new sh::Vector4(
            weather.mSkyColor.r, weather.mSkyColor.g, weather.mSkyColor.b, weather.mSkyColor.a)));
    }

    if (mFogColour != weather.mFogColor)
    {
        mFogColour = weather.mFogColor;
        sh::Factory::getInstance().setSharedParameter ("horizonColour", sh::makeProperty<sh::Vector4>(new sh::Vector4(
            weather.mFogColor.r, weather.mFogColor.g, weather.mFogColor.b, weather.mFogColor.a)));
    }

    mCloudSpeed = weather.mCloudSpeed;

    if (weather.mNight && mStarsOpacity != weather.mNightFade)
    {
        if (weather.mNightFade == 0)
            mAtmosphereNight->setVisible(false);
        else
        {
            mAtmosphereNight->setVisible(true);

            sh::Factory::getInstance().setSharedParameter ("nightFade",
                sh::makeProperty<sh::FloatValue>(new sh::FloatValue(weather.mNightFade)));

            mStarsOpacity = weather.mNightFade;
        }
    }


    float strength;
    float timeofday_angle = std::abs(mSunGlare->getPosition().z/mSunGlare->getPosition().length());
    if (timeofday_angle <= 0.44)
        strength = timeofday_angle/0.44f;
    else
        strength = 1.f;

    mSunGlare->setVisibility(weather.mGlareView * mGlareFade * strength);

    mSun->setVisibility(weather.mGlareView * strength);

    mAtmosphereNight->setVisible(weather.mNight && mEnabled);

    if (mParticle.get())
        setAlpha(mParticle, weather.mEffectFade);
    for (std::map<Ogre::SceneNode*, NifOgre::ObjectScenePtr>::iterator it = mRainModels.begin(); it != mRainModels.end(); ++it)
        setAlpha(it->second, weather.mEffectFade);
}

void SkyManager::setGlare(const float glare)
{
    mGlare = glare;
}

Vector3 SkyManager::getRealSunPos()
{
    if (!mCreated) return Vector3(0,0,0);
    return mSun->getNode()->getPosition() + mCamera->getRealPosition();
}

void SkyManager::sunEnable()
{
    mSunEnabled = true;
}

void SkyManager::sunDisable()
{
    mSunEnabled = false;
}

void SkyManager::setStormDirection(const Vector3 &direction)
{
    mStormDirection = direction;
}

void SkyManager::setSunDirection(const Vector3& direction, bool is_night)
{
    if (!mCreated) return;
    mSun->setPosition(direction);
    mSunGlare->setPosition(direction);

    float height = direction.z;
    float fade = is_night ? 0.0f : (( height > 0.5) ? 1.0f : height * 2);
    sh::Factory::getInstance ().setSharedParameter ("waterSunFade_sunHeight", sh::makeProperty<sh::Vector2>(new sh::Vector2(fade, height)));
}

void SkyManager::setMasserDirection(const Vector3& direction)
{
    if (!mCreated) return;
    mMasser->setPosition(direction);
}

void SkyManager::setSecundaDirection(const Vector3& direction)
{
    if (!mCreated) return;
    mSecunda->setPosition(direction);
}

void SkyManager::masserEnable()
{
    mMasserEnabled = true;
}

void SkyManager::secundaEnable()
{
    mSecundaEnabled = true;
}

void SkyManager::masserDisable()
{
    mMasserEnabled = false;
}

void SkyManager::secundaDisable()
{
    mSecundaEnabled = false;
}

void SkyManager::setLightningStrength(const float factor)
{
    if (!mCreated) return;
    if (factor > 0.f)
    {
        mLightning->setDiffuseColour (ColourValue(2*factor, 2*factor, 2*factor));
        mLightning->setVisible(true);
    }
    else
        mLightning->setVisible(false);
}

void SkyManager::setMasserFade(const float fade)
{
    if (!mCreated) return;
    mMasser->setVisibility(fade);
}

void SkyManager::setSecundaFade(const float fade)
{
    if (!mCreated) return;
    mSecunda->setVisibility(fade);
}

void SkyManager::setHour(double hour)
{
    mHour = static_cast<float>(hour);
}

void SkyManager::setDate(int day, int month)
{
    mDay = day;
    mMonth = month;
}

Ogre::SceneNode* SkyManager::getSunNode()
{
    if (!mCreated) return 0;
    return mSun->getNode();
}

void SkyManager::setGlareEnabled (bool enabled)
{
    if (!mCreated || !mEnabled)
        return;
    mSunGlare->setVisible (mSunEnabled && enabled);
}

void SkyManager::attachToNode(SceneNode *sceneNode)
{
    if (!mParticleNode)
    {
        mParticleNode = sceneNode->createChildSceneNode();
        mParticleNode->setInheritOrientation(false);
    }
    else
    {
        sceneNode->addChild(mParticleNode);
    }
}
