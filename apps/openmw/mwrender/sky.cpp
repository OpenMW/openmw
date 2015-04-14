#include "sky.hpp"

#include <osg/Transform>
#include <osg/Geode>
#include <osg/Depth>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

#include <osg/io_utils>

#include <osgUtil/CullVisitor>

#include <boost/lexical_cast.hpp>

#include <openengine/misc/rng.hpp>

#include <components/misc/resourcehelpers.hpp>

#include <components/resource/scenemanager.hpp>
#include <components/resource/texturemanager.hpp>

#include <components/vfs/manager.hpp>

#include <components/sceneutil/util.hpp>
#include <components/sceneutil/statesetcontroller.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/fallback.hpp"

#include "renderconst.hpp"
#include "renderingmanager.hpp"

namespace
{

    osg::StateSet* getWritableStateSet(osg::Node* node)
    {
        osg::StateSet* stateset = node->getOrCreateStateSet();
        osg::ref_ptr<osg::StateSet> cloned = static_cast<osg::StateSet*>(stateset->clone(osg::CopyOp::SHALLOW_COPY));
        node->setStateSet(cloned);
        return cloned;
    }

    osg::ref_ptr<osg::Material> createAlphaTrackingUnlitMaterial()
    {
        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        mat->setColorMode(osg::Material::DIFFUSE);
        return mat;
    }

    osg::ref_ptr<osg::Material> createUnlitMaterial()
    {
        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        mat->setColorMode(osg::Material::OFF);
        return mat;
    }

    osg::ref_ptr<osg::Geometry> createTexturedQuad()
    {
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-0.5, -0.5, 0));
        verts->push_back(osg::Vec3f(-0.5, 0.5, 0));
        verts->push_back(osg::Vec3f(0.5, 0.5, 0));
        verts->push_back(osg::Vec3f(0.5, -0.5, 0));

        geom->setVertexArray(verts);

        osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;
        texcoords->push_back(osg::Vec2f(0, 0));
        texcoords->push_back(osg::Vec2f(0, 1));
        texcoords->push_back(osg::Vec2f(1, 1));
        texcoords->push_back(osg::Vec2f(1, 0));

        geom->setTexCoordArray(0, texcoords, osg::Array::BIND_PER_VERTEX);

        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4));

        return geom;
    }

}

namespace MWRender
{

class AtmosphereUpdater : public SceneUtil::StateSetController
{
public:
    void setEmissionColor(osg::Vec4f emissionColor)
    {
        mEmissionColor = emissionColor;
    }

protected:
    virtual void setDefaults(osg::StateSet* stateset)
    {
        stateset->setAttribute(createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    }

    virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
    {
        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        mat->setEmission(osg::Material::FRONT_AND_BACK, mEmissionColor);
    }

private:
    osg::Vec4f mEmissionColor;
};

/// Transform that removes the eyepoint of the modelview matrix,
/// i.e. its children are positioned relative to the camera.
class CameraRelativeTransform : public osg::Transform
{
public:
    CameraRelativeTransform()
    {
    }

    CameraRelativeTransform(const CameraRelativeTransform& copy, const osg::CopyOp& copyop)
        : osg::Transform(copy, copyop)
    {
    }

    META_Node(MWRender, CameraRelativeTransform)

    virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor*) const
    {
        if (_referenceFrame==RELATIVE_RF)
        {
            matrix.setTrans(osg::Vec3f(0.f,0.f,0.f));
            return false;
        }
        else // absolute
        {
            matrix.makeIdentity();
            return true;
        }
    }

    osg::BoundingSphere computeBound() const
    {
        return osg::BoundingSphere(osg::Vec3f(0,0,0), 0);
    }
};

class DisableCullingVisitor : public osg::NodeVisitor
{
public:
    DisableCullingVisitor()
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
    {
    }

    void apply(osg::Geode &geode)
    {
        geode.setCullingActive(false);
    }
};

class ModVertexAlphaVisitor : public osg::NodeVisitor
{
public:
    ModVertexAlphaVisitor(int meshType)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mMeshType(meshType)
    {
    }

    void apply(osg::Geode &geode)
    {
        for (unsigned int i=0; i<geode.getNumDrawables(); ++i)
        {
            osg::Drawable* drw = geode.getDrawable(i);

            osg::Geometry* geom = drw->asGeometry();
            if (!geom)
                continue;

            // might want to use fog coordinates instead of vertex colors so we can apply a separate fade to the diffuse alpha
            // (that isn't possible now, with the diffuse tracking the vertex colors)

            osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(geom->getVertexArray()->getNumElements());
            for (unsigned int i=0; i<colors->size(); ++i)
            {
                float alpha = 1.f;
                if (mMeshType == 0) alpha = i%2 ? 0.f : 1.f; // this is a cylinder, so every second vertex belongs to the bottom-most row
                else if (mMeshType == 1)
                {
                    if (i>= 49 && i <= 64) alpha = 0.f; // bottom-most row
                    else if (i>= 33 && i <= 48) alpha = 0.25098; // second row
                    else alpha = 1.f;
                }
                else if (mMeshType == 2)
                {
                    osg::Vec4Array* origColors = static_cast<osg::Vec4Array*>(geom->getColorArray());
                    alpha = ((*origColors)[i].x() == 1.f) ? 1.f : 0.f;
                }

                (*colors)[i] = osg::Vec4f(alpha, alpha, alpha, alpha);
            }

            geom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
        }
    }

private:
    int mMeshType;
};

SkyManager::SkyManager(osg::Group* parentNode, Resource::SceneManager* sceneManager)
    : mSceneManager(sceneManager)
    , mHour(0.0f)
    , mDay(0)
    , mMonth(0)
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
    , mRainEnabled(false)
    , mRainTimer(0)
    , mRainSpeed(0)
    , mRainFrequency(1)
    , mStormDirection(0,-1,0)
    , mIsStorm(false)
{
    osg::ref_ptr<CameraRelativeTransform> skyroot (new CameraRelativeTransform);
    skyroot->setCullingActive(false);
    parentNode->addChild(skyroot);

    mRootNode = skyroot;

    // By default render before the world is rendered
    mRootNode->getOrCreateStateSet()->setRenderBinDetails(-1, "RenderBin");
}

void SkyManager::create()
{
    assert(!mCreated);

    mAtmosphereDay = mSceneManager->createInstance("meshes/sky_atmosphere.nif", mRootNode);
    ModVertexAlphaVisitor modAtmosphere(0);
    mAtmosphereDay->accept(modAtmosphere);

    // osg::Node* alphaBlendedRoot =

    mAtmosphereUpdater = new AtmosphereUpdater;
    mAtmosphereDay->addUpdateCallback(mAtmosphereUpdater);

    if (mSceneManager->getVFS()->exists("meshes/sky_night_02.nif"))
        mAtmosphereNight = mSceneManager->createInstance("meshes/sky_night_02.nif", mRootNode);
    else
        mAtmosphereNight = mSceneManager->createInstance("meshes/sky_night_01.nif", mRootNode);
    mAtmosphereNight->getOrCreateStateSet()->setAttributeAndModes(createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    ModVertexAlphaVisitor modStars(2);
    mAtmosphereNight->accept(modStars);
    mAtmosphereNight->setNodeMask(0);

    osg::Geode* geode = new osg::Geode;
    osg::ref_ptr<osg::Geometry> sun = createTexturedQuad();
    geode->addDrawable(sun);
    osg::PositionAttitudeTransform* trans = new osg::PositionAttitudeTransform;
    trans->setScale(osg::Vec3f(450,450,450));
    trans->addChild(geode);
    mRootNode->addChild(trans);

    mSunTransform = trans;

    mCloudNode = mSceneManager->createInstance("meshes/sky_clouds_01.nif", mRootNode);
    ModVertexAlphaVisitor modClouds(1);
    mCloudNode->accept(modClouds);

    osg::ref_ptr<osg::Texture2D> sunTex = mSceneManager->getTextureManager()->getTexture2D("textures/tx_sun_05.dds",
                                                                                           osg::Texture::CLAMP, osg::Texture::CLAMP);
    trans->getOrCreateStateSet()->setTextureAttributeAndModes(0, sunTex, osg::StateAttribute::ON);
    trans->getOrCreateStateSet()->setAttributeAndModes(createUnlitMaterial(), osg::StateAttribute::ON);

    mCloudNode->getOrCreateStateSet()->setAttributeAndModes(createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

    osg::ref_ptr<osg::Depth> depth = new osg::Depth;
    depth->setWriteMask(false);
    mRootNode->getOrCreateStateSet()->setAttributeAndModes(depth, osg::StateAttribute::ON);
    mRootNode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);

    mCreated = true;
}

SkyManager::~SkyManager()
{
    clearRain();
}

int SkyManager::getMasserPhase() const
{
    return 0;
    if (!mCreated) return 0;
    //return mMasser->getPhaseInt();
}

int SkyManager::getSecundaPhase() const
{
    return 0;
    if (!mCreated) return 0;
    //return mSecunda->getPhaseInt();
}

void SkyManager::clearRain()
{
}

void SkyManager::updateRain(float dt)
{
}

void SkyManager::update(float duration)
{
    if (!mEnabled) return;
    //const MWWorld::Fallback* fallback=MWBase::Environment::get().getWorld()->getFallback();

    //if (mIsStorm)
    //    mCloudNode->setOrientation(Ogre::Vector3::UNIT_Y.getRotationTo(mStormDirection));
    //else
    //    mCloudNode->setOrientation(Ogre::Quaternion::IDENTITY);

    updateRain(duration);

    // UV Scroll the clouds
    mCloudAnimationTimer += duration * mCloudSpeed;

    /// \todo improve this
    //mMasser->setPhase( static_cast<Moon::Phase>( (int) ((mDay % 32)/4.f)) );
    //mSecunda->setPhase ( static_cast<Moon::Phase>( (int) ((mDay % 32)/4.f)) );

    //mSecunda->setColour ( mMoonRed ? fallback->getFallbackColour("Moons_Script_Color") : ColourValue(1,1,1,1));
    //mMasser->setColour (ColourValue(1,1,1,1));

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
        /*
        Vector3 sun = mSunGlare->getPosition();
        Vector3 cam = mCamera->getRealDirection();
        const Degree angle = sun.angleBetween( cam );
        float val = 1- (angle.valueDegrees() / 180.f);
        val = (val*val*val*val)*6;
        mSunGlare->setSize(val * mGlareFade);
        */
    }

    //mSunGlare->setVisible(mSunEnabled);
    //mSun->setVisible(mSunEnabled);
    //mMasser->setVisible(mMasserEnabled);
    //mSecunda->setVisible(mSecundaEnabled);

    // rotate the stars by 360 degrees every 4 days
    //mAtmosphereNight->roll(Degree(MWBase::Environment::get().getWorld()->getTimeScaleFactor()*duration*360 / (3600*96.f)));
}

void SkyManager::setEnabled(bool enabled)
{
    if (enabled && !mCreated)
        create();

    if (!enabled)
        clearRain();

    mRootNode->setNodeMask(enabled ? ~((unsigned int)(0)) : 0);

    mEnabled = enabled;
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
            if (mParticleEffect)
                mRootNode->removeChild(mParticleEffect);
            mParticleEffect = NULL;
        }
        else
        {
            mParticleEffect = mSceneManager->createInstance(mCurrentParticleEffect, mRootNode);
            DisableCullingVisitor visitor;
            mParticleEffect->accept(visitor);

            /*
            for (size_t i = 0; i < mParticle->mControllers.size(); ++i)
            {
                if (mParticle->mControllers[i].getSource().isNull())
                    mParticle->mControllers[i].setSource(Ogre::ControllerManager::getSingleton().getFrameTimeSource());
            }
            */
        }
    }

    if (mClouds != weather.mCloudTexture)
    {
        mClouds = weather.mCloudTexture;
    }

    if (mNextClouds != weather.mNextCloudTexture)
    {
        mNextClouds = weather.mNextCloudTexture;
    }

    if (mCloudBlendFactor != weather.mCloudBlendFactor)
    {
        mCloudBlendFactor = weather.mCloudBlendFactor;
    }

    if (mCloudOpacity != weather.mCloudOpacity)
    {
        mCloudOpacity = weather.mCloudOpacity;
    }

    if (mCloudColour != weather.mSunColor)
    {
        /*
        osg::Vec4f clr( weather.mSunColor.r()*0.7f + weather.mAmbientColor.r()*0.7f,
                        weather.mSunColor.g()*0.7f + weather.mAmbientColor.g()*0.7f,
                        weather.mSunColor.b()*0.7f + weather.mAmbientColor.b()*0.7f, 1.f);
                        */

        mCloudColour = weather.mSunColor;
    }

    if (mSkyColour != weather.mSkyColor)
    {
        mSkyColour = weather.mSkyColor;

        mAtmosphereUpdater->setEmissionColor(mSkyColour);
    }

    if (mFogColour != weather.mFogColor)
    {
        mFogColour = weather.mFogColor;
    }

    mCloudSpeed = weather.mCloudSpeed;

    if (weather.mNight && mStarsOpacity != weather.mNightFade)
    {
        if (weather.mNightFade != 0)
        {
            //sh::Factory::getInstance().setSharedParameter ("nightFade",
            //    sh::makeProperty<sh::FloatValue>(new sh::FloatValue(weather.mNightFade)));

            //mStarsOpacity = weather.mNightFade;
        }
    }

    mAtmosphereNight->setNodeMask((weather.mNight && mEnabled) ? ~0 : 0);


    /*
    float strength;
    float timeofday_angle = std::abs(mSunGlare->getPosition().z/mSunGlare->getPosition().length());
    if (timeofday_angle <= 0.44)
        strength = timeofday_angle/0.44f;
    else
        strength = 1.f;

    mSunGlare->setVisibility(weather.mGlareView * mGlareFade * strength);

    mSun->setVisibility(weather.mGlareView * strength);


    if (mParticle.get())
        setAlpha(mParticle, weather.mEffectFade);
    for (std::map<Ogre::SceneNode*, NifOgre::ObjectScenePtr>::iterator it = mRainModels.begin(); it != mRainModels.end(); ++it)
        setAlpha(it->second, weather.mEffectFade);
        */
}

void SkyManager::setGlare(const float glare)
{
    mGlare = glare;
}

void SkyManager::sunEnable()
{
    mSunEnabled = true;
}

void SkyManager::sunDisable()
{
    mSunEnabled = false;
}

void SkyManager::setStormDirection(const Ogre::Vector3 &direction)
{
    mStormDirection = direction;
}

void SkyManager::setSunDirection(const osg::Vec3f& direction)
{
    if (!mCreated) return;

    mSunTransform->setPosition(direction*1000.f);

    osg::Quat quat;
    quat.makeRotate(osg::Vec3f(0,0,1), direction);
    mSunTransform->setAttitude(quat);

    //mSunGlare->setPosition(direction);
}

void SkyManager::setMasserDirection(const Ogre::Vector3& direction)
{
    if (!mCreated) return;
    //mMasser->setPosition(direction);
}

void SkyManager::setSecundaDirection(const Ogre::Vector3& direction)
{
    if (!mCreated) return;
    //mSecunda->setPosition(direction);
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
    /*
    if (factor > 0.f)
    {
        mLightning->setDiffuseColour (ColourValue(2*factor, 2*factor, 2*factor));
        mLightning->setVisible(true);
    }
    else
        mLightning->setVisible(false);
        */
}

void SkyManager::setMasserFade(const float fade)
{
    if (!mCreated) return;
    //mMasser->setVisibility(fade);
}

void SkyManager::setSecundaFade(const float fade)
{
    if (!mCreated) return;
    //mSecunda->setVisibility(fade);
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

void SkyManager::setGlareEnabled (bool enabled)
{
    if (!mCreated || !mEnabled)
        return;
    //mSunGlare->setVisible (mSunEnabled && enabled);
}

}
