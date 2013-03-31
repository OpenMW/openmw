#include "sky.hpp"

#include <OgreCamera.h>
#include <OgreRenderWindow.h>
#include <OgreSceneNode.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreBillboardSet.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>

#include <boost/lexical_cast.hpp>

#include <components/nifogre/ogrenifloader.hpp>

#include <extern/shiny/Platforms/Ogre/OgreMaterial.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/fallback.hpp"

#include "renderconst.hpp"
#include "renderingmanager.hpp"

using namespace MWRender;
using namespace Ogre;

BillboardObject::BillboardObject( const String& textureName,
                    const float initialSize,
                    const Vector3& position,
                    SceneNode* rootNode,
                    const std::string& material)
{
    SceneManager* sceneMgr = rootNode->getCreator();

    Vector3 finalPosition = position.normalisedCopy() * 1000.f;

    static unsigned int bodyCount=0;

    /// \todo These billboards are not 100% correct, might want to revisit them later
    mBBSet = sceneMgr->createBillboardSet("SkyBillboardSet"+StringConverter::toString(bodyCount), 1);
    mBBSet->setDefaultDimensions(550.f*initialSize, 550.f*initialSize);
    mBBSet->setBillboardType(BBT_PERPENDICULAR_COMMON);
    mBBSet->setCommonDirection( -position.normalisedCopy() );
    mBBSet->setVisibilityFlags(RV_Sky);
    mNode = rootNode->createChildSceneNode();
    mNode->setPosition(finalPosition);
    mNode->attachObject(mBBSet);
    mBBSet->createBillboard(0,0,0);
    mBBSet->setCastShadows(false);

    mMaterial = sh::Factory::getInstance().createMaterialInstance ("BillboardMaterial"+StringConverter::toString(bodyCount), material);
    mMaterial->setProperty("texture", sh::makeProperty<sh::StringValue>(new sh::StringValue(textureName)));

    sh::Factory::getInstance().getMaterialInstance ("BillboardMaterial"+StringConverter::toString(bodyCount))->setListener(this);

    mBBSet->setMaterialName("BillboardMaterial"+StringConverter::toString(bodyCount));

    bodyCount++;
}

BillboardObject::BillboardObject()
{
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
    mBBSet->setVisible(visible);
}

void BillboardObject::setSize(const float size)
{
    mNode->setScale(size, size, size);
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

    mBBSet->setCommonDirection( -normalised );

    mNode->setPosition(finalPosition);
}

Vector3 BillboardObject::getPosition() const
{
    Vector3 p = mNode->_getDerivedPosition() - mNode->getParentSceneNode()->_getDerivedPosition();
    return p;
}

void BillboardObject::setVisibilityFlags(int flags)
{
    mBBSet->setVisibilityFlags(flags);
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
    mBBSet->setRenderQueueGroup(id);
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
{
    setVisibility(1.0);

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
        sh::Factory::getInstance ().setTextureAlias ("secunda_texture", textureName);
    else
        sh::Factory::getInstance ().setTextureAlias ("masser_texture", textureName);

    mPhase = phase;
}

Moon::Phase Moon::getPhase() const
{
    return mPhase;
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

SkyManager::SkyManager (SceneNode* root, Camera* pCamera)
    : mHour(0.0f)
    , mDay(0)
    , mMonth(0)
    , mSun(NULL)
    , mSunGlare(NULL)
    , mMasser(NULL)
    , mSecunda(NULL)
    , mCamera(pCamera)
    , mRootNode(NULL)
    , mSceneMgr(NULL)
    , mAtmosphereDay(NULL)
    , mAtmosphereNight(NULL)
    , mCloudFragmentShader()
    , mClouds()
    , mNextClouds()
    , mCloudBlendFactor(0.0f)
    , mCloudOpacity(0.0f)
    , mCloudSpeed(0.0f)
    , mStarsOpacity(0.0f)
    , mRemainingTransitionTime(0.0f)
    , mGlareFade(0.0f)
    , mGlare(0.0f)
    , mEnabled(true)
    , mSunEnabled(true)
    , mMasserEnabled(true)
    , mSecundaEnabled(true)
    , mCreated(false)
    , mCloudAnimationTimer(0.f)
    , mMoonRed(false)
{
    mSceneMgr = root->getCreator();
    mRootNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    mRootNode->setInheritOrientation(false);
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
    mLightning->setDirection (Ogre::Vector3(0.3, -0.7, 0.3));
    mLightning->setVisible (false);
    mLightning->setDiffuseColour (ColourValue(3,3,3));

    const MWWorld::Fallback* fallback=MWBase::Environment::get().getWorld()->getFallback();
    mSecunda = new Moon("secunda_texture", /*0.5*/fallback->getFallbackFloat("Moons_Secunda_Size")/100, Vector3(-0.4, 0.4, 0.5), mRootNode, "openmw_moon");
    mSecunda->setType(Moon::Type_Secunda);
    mSecunda->setRenderQueue(RQG_SkiesEarly+4);

    mMasser = new Moon("masser_texture", /*0.75*/fallback->getFallbackFloat("Moons_Masser_Size")/100, Vector3(-0.4, 0.4, 0.5), mRootNode, "openmw_moon");
    mMasser->setRenderQueue(RQG_SkiesEarly+3);
    mMasser->setType(Moon::Type_Masser);

    mSun = new BillboardObject("textures\\tx_sun_05.dds", 1, Vector3(0.4, 0.4, 0.4), mRootNode, "openmw_sun");
    mSun->setRenderQueue(RQG_SkiesEarly+4);
    mSunGlare = new BillboardObject("textures\\tx_sun_flash_grey_05.dds", 3, Vector3(0.4, 0.4, 0.4), mRootNode, "openmw_sun");
    mSunGlare->setRenderQueue(RQG_SkiesLate);
    mSunGlare->setVisibilityFlags(RV_NoReflection);

    Ogre::AxisAlignedBox aabInf;
    aabInf.setInfinite ();

    // Stars
    mAtmosphereNight = mRootNode->createChildSceneNode();
    NifOgre::EntityList entities = NifOgre::Loader::createEntities(mAtmosphereNight, "meshes\\sky_night_01.nif");
    for(size_t i = 0, matidx = 0;i < entities.mEntities.size();i++)
    {
        Entity* night1_ent = entities.mEntities[i];
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


    // Atmosphere (day)
    mAtmosphereDay = mRootNode->createChildSceneNode();
    entities = NifOgre::Loader::createEntities(mAtmosphereDay, "meshes\\sky_atmosphere.nif");
    for(size_t i = 0;i < entities.mEntities.size();i++)
    {
        Entity* atmosphere_ent = entities.mEntities[i];
        atmosphere_ent->setCastShadows(false);
        atmosphere_ent->setRenderQueueGroup(RQG_SkiesEarly);
        atmosphere_ent->setVisibilityFlags(RV_Sky);

        for(unsigned int j = 0;j < atmosphere_ent->getNumSubEntities();j++)
            atmosphere_ent->getSubEntity (j)->setMaterialName("openmw_atmosphere");

        // Using infinite AAB here to prevent being clipped by the custom near clip plane used for reflections/refractions
        atmosphere_ent->getMesh()->_setBounds (aabInf);
    }


    // Clouds
    SceneNode* clouds_node = mRootNode->createChildSceneNode();
    entities = NifOgre::Loader::createEntities(clouds_node, "meshes\\sky_clouds_01.nif");
    for(size_t i = 0;i < entities.mEntities.size();i++)
    {
        Entity* clouds_ent = entities.mEntities[i];
        clouds_ent->setVisibilityFlags(RV_Sky);
        clouds_ent->setRenderQueueGroup(RQG_SkiesEarly+5);
        for(unsigned int j = 0;j < clouds_ent->getNumSubEntities();j++)
            clouds_ent->getSubEntity(j)->setMaterialName("openmw_clouds");
        clouds_ent->setCastShadows(false);
        // Using infinite AAB here to prevent being clipped by the custom near clip plane used for reflections/refractions
        clouds_ent->getMesh()->_setBounds (aabInf);
    }

    mCreated = true;
}

SkyManager::~SkyManager()
{
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

void SkyManager::update(float duration)
{
    if (!mEnabled) return;
    const MWWorld::Fallback* fallback=MWBase::Environment::get().getWorld()->getFallback();
    mCamera->getParentSceneNode ()->needUpdate ();
    mRootNode->setPosition(mCamera->getDerivedPosition());

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
        val = (val*val*val*val)*2;

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

    mRootNode->setVisible(true);
    mEnabled = true;
}

void SkyManager::disable()
{
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

    if (mClouds != weather.mCloudTexture)
    {
        sh::Factory::getInstance().setTextureAlias ("cloud_texture_1", "textures\\"+weather.mCloudTexture);
        mClouds = weather.mCloudTexture;
    }

    if (mNextClouds != weather.mNextCloudTexture)
    {
        sh::Factory::getInstance().setTextureAlias ("cloud_texture_2", "textures\\"+weather.mNextCloudTexture);
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
        ColourValue clr( weather.mSunColor.r*0.7 + weather.mAmbientColor.r*0.7,
                        weather.mSunColor.g*0.7 + weather.mAmbientColor.g*0.7,
                        weather.mSunColor.b*0.7 + weather.mAmbientColor.b*0.7);

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
}

void SkyManager::setGlare(const float glare)
{
    mGlare = glare;
}

Vector3 SkyManager::getRealSunPos()
{
    if (!mCreated) return Vector3(0,0,0);
    return mSun->getNode()->_getDerivedPosition();
}

void SkyManager::sunEnable()
{
    mSunEnabled = true;
}

void SkyManager::sunDisable()
{
    mSunEnabled = false;
}

void SkyManager::setSunDirection(const Vector3& direction)
{
    if (!mCreated) return;
    mSun->setPosition(direction);
    mSunGlare->setPosition(direction);

    float height = direction.z;
    float fade = ( height > 0.5) ? 1.0 : height * 2;
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
void SkyManager::setLightningEnabled(bool enabled)
{
    /// \todo
}

void SkyManager::setLightningDirection(const Ogre::Vector3& dir)
{
    if (!mCreated) return;
    mLightning->setDirection (dir);
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
    mHour = hour;
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

void SkyManager::setSkyPosition(const Ogre::Vector3& position)
{
    mRootNode->setPosition(position);
}

void SkyManager::resetSkyPosition()
{
    mCamera->getParentSceneNode ()->needUpdate ();
    mRootNode->setPosition(mCamera->getDerivedPosition());
}

void SkyManager::scaleSky(float scale)
{
    mRootNode->setScale(scale, scale, scale);
}

void SkyManager::setGlareEnabled (bool enabled)
{
    if (!mCreated)
        return;
    mSunGlare->setVisible (mSunEnabled && enabled);
}
