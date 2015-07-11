#include "sky.hpp"

#include <osg/Transform>
#include <osg/Geode>
#include <osg/Depth>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/TexEnvCombine>
#include <osg/TexMat>
#include <osg/Version>

#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/ModularEmitter>
#include <osgParticle/BoxPlacer>
#include <osgParticle/ConstantRateCounter>
#include <osgParticle/RadialShooter>

#include <components/misc/rng.hpp>

#include <components/misc/resourcehelpers.hpp>

#include <components/resource/scenemanager.hpp>
#include <components/resource/texturemanager.hpp>

#include <components/vfs/manager.hpp>

#include <components/sceneutil/util.hpp>
#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/controller.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/fallback.hpp"

#include "vismask.hpp"

namespace
{

    osg::ref_ptr<osg::Material> createAlphaTrackingUnlitMaterial()
    {
        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        mat->setColorMode(osg::Material::DIFFUSE);
        return mat;
    }

    osg::ref_ptr<osg::Material> createUnlitMaterial()
    {
        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        mat->setColorMode(osg::Material::OFF);
        return mat;
    }

    osg::ref_ptr<osg::Geometry> createTexturedQuad(int numUvSets=1)
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

        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.f, 1.f, 1.f, 1.f));
        geom->setColorArray(colors, osg::Array::BIND_OVERALL);

        for (int i=0; i<numUvSets; ++i)
            geom->setTexCoordArray(i, texcoords, osg::Array::BIND_PER_VERTEX);

        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4));

        return geom;
    }

}

namespace MWRender
{

class AtmosphereUpdater : public SceneUtil::StateSetUpdater
{
public:
    void setEmissionColor(const osg::Vec4f& emissionColor)
    {
        mEmissionColor = emissionColor;
    }

protected:
    virtual void setDefaults(osg::StateSet* stateset)
    {
        stateset->setAttributeAndModes(createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    }

    virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
    {
        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        mat->setEmission(osg::Material::FRONT_AND_BACK, mEmissionColor);
    }

private:
    osg::Vec4f mEmissionColor;
};

class AtmosphereNightUpdater : public SceneUtil::StateSetUpdater
{
public:
    AtmosphereNightUpdater(Resource::TextureManager* textureManager)
    {
        // we just need a texture, its contents don't really matter
        mTexture = textureManager->getWarningTexture();
    }

    void setFade(const float fade)
    {
        mColor.a() = fade;
    }

protected:
    virtual void setDefaults(osg::StateSet* stateset)
    {
        osg::ref_ptr<osg::TexEnvCombine> texEnv (new osg::TexEnvCombine);
        texEnv->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
        texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
        texEnv->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
        texEnv->setCombine_RGB(osg::TexEnvCombine::REPLACE);
        texEnv->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);

        stateset->setTextureAttributeAndModes(1, mTexture, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        stateset->setTextureAttributeAndModes(1, texEnv, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    }

    virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
    {
        osg::TexEnvCombine* texEnv = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(1, osg::StateAttribute::TEXENV));
        texEnv->setConstantColor(mColor);
    }

    osg::ref_ptr<osg::Texture2D> mTexture;

    osg::Vec4f mColor;
};

class CloudUpdater : public SceneUtil::StateSetUpdater
{
public:
    CloudUpdater()
        : mAnimationTimer(0.f)
        , mOpacity(0.f)
    {
    }

    void setAnimationTimer(float timer)
    {
        mAnimationTimer = timer;
    }

    void setTexture(osg::ref_ptr<osg::Texture2D> texture)
    {
        mTexture = texture;
    }
    void setEmissionColor(const osg::Vec4f& emissionColor)
    {
        mEmissionColor = emissionColor;
    }
    void setOpacity(float opacity)
    {
        mOpacity = opacity;
    }

protected:
    virtual void setDefaults(osg::StateSet *stateset)
    {
        osg::ref_ptr<osg::TexMat> texmat (new osg::TexMat);
        stateset->setTextureAttributeAndModes(0, texmat, osg::StateAttribute::ON);
        stateset->setTextureAttributeAndModes(1, texmat, osg::StateAttribute::ON);
        stateset->setAttribute(createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

        // need to set opacity on a separate texture unit, diffuse alpha is used by the vertex colors already
        osg::ref_ptr<osg::TexEnvCombine> texEnvCombine (new osg::TexEnvCombine);
        texEnvCombine->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
        texEnvCombine->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
        texEnvCombine->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
        texEnvCombine->setConstantColor(osg::Vec4f(1,1,1,1));
        texEnvCombine->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
        texEnvCombine->setCombine_RGB(osg::TexEnvCombine::REPLACE);

        stateset->setTextureAttributeAndModes(1, texEnvCombine, osg::StateAttribute::ON);

        stateset->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        stateset->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    }

    virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
    {
        osg::TexMat* texMat = static_cast<osg::TexMat*>(stateset->getTextureAttribute(0, osg::StateAttribute::TEXMAT));
        texMat->setMatrix(osg::Matrix::translate(osg::Vec3f(0, mAnimationTimer, 0.f)));

        stateset->setTextureAttribute(0, mTexture, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        stateset->setTextureAttribute(1, mTexture, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        mat->setEmission(osg::Material::FRONT_AND_BACK, mEmissionColor);

        osg::TexEnvCombine* texEnvCombine = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(1, osg::StateAttribute::TEXENV));
        texEnvCombine->setConstantColor(osg::Vec4f(1,1,1,mOpacity));
    }

private:
    float mAnimationTimer;
    osg::ref_ptr<osg::Texture2D> mTexture;
    osg::Vec4f mEmissionColor;
    float mOpacity;
};

/// Transform that removes the eyepoint of the modelview matrix,
/// i.e. its children are positioned relative to the camera.
class CameraRelativeTransform : public osg::Transform
{
public:
    CameraRelativeTransform()
    {
        // Culling works in node-local space, not in camera space, so we can't cull this node correctly
        // That's not a problem though, children of this node can be culled just fine
        // Just make sure you do not place a CameraRelativeTransform deep in the scene graph
        setCullingActive(false);
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

                (*colors)[i] = osg::Vec4f(0.f, 0.f, 0.f, alpha);
            }

            geom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
        }
    }

private:
    int mMeshType;
};

class CelestialBody
{
public:
    CelestialBody(osg::Group* parentNode, Resource::SceneManager* sceneManager, float scaleFactor = 1.f, int numUvSets=1)
        : mSceneManager(sceneManager)
    {
        mGeode = new osg::Geode;
        osg::ref_ptr<osg::Geometry> geom = createTexturedQuad(numUvSets);
        mGeode->addDrawable(geom);
        mTransform = new osg::PositionAttitudeTransform;
        mTransform->setScale(osg::Vec3f(450,450,450) * scaleFactor);
        mTransform->addChild(mGeode);

        parentNode->addChild(mTransform);
    }

    void setDirection(const osg::Vec3f& direction)
    {
        osg::Vec3f normalizedDirection = direction / direction.length();
        mTransform->setPosition(normalizedDirection*1000.f);

        osg::Quat quat;
        quat.makeRotate(osg::Vec3f(0,0,1), normalizedDirection);
        mTransform->setAttitude(quat);
    }

    void setVisible(bool visible)
    {
        mTransform->setNodeMask(visible ? ~0 : 0);
    }

protected:
    osg::ref_ptr<osg::PositionAttitudeTransform> mTransform;
    osg::ref_ptr<osg::Geode> mGeode;
    Resource::SceneManager* mSceneManager;

};

class Sun : public CelestialBody
{
public:
    Sun(osg::Group* parentNode, Resource::SceneManager* sceneManager)
        : CelestialBody(parentNode, sceneManager, 1.f, 1)
    {
        osg::ref_ptr<osg::Texture2D> tex = mSceneManager->getTextureManager()->getTexture2D("textures/tx_sun_05.dds",
                                                                                               osg::Texture::CLAMP, osg::Texture::CLAMP);

        mTransform->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON);
        mTransform->getOrCreateStateSet()->setAttributeAndModes(createUnlitMaterial(), osg::StateAttribute::ON);
    }
};

class Moon : public CelestialBody
{
public:
    enum Type
    {
        Type_Masser = 0,
        Type_Secunda
    };

    Moon(osg::Group* parentNode, Resource::SceneManager* sceneManager, float scaleFactor, Type type)
        : CelestialBody(parentNode, sceneManager, scaleFactor, 2)
        , mType(type)
        , mPhase(Phase_Unspecified)
    {
        mUpdater = new MoonUpdater;
        mGeode->addUpdateCallback(mUpdater);

        setPhase(Phase_WaxingCrescent);
    }

    enum Phase
    {
        Phase_New = 0,
        Phase_WaxingCrescent,
        Phase_WaxingHalf,
        Phase_WaxingGibbous,
        Phase_Full,
        Phase_WaningGibbous,
        Phase_WaningHalf,
        Phase_WaningCrescent,
        Phase_Unspecified
    };

    void setTextures(const std::string& phaseTex, const std::string& circleTex)
    {
        osg::ref_ptr<osg::Texture2D> phaseTexPtr = mSceneManager->getTextureManager()->getTexture2D(phaseTex,
                                                                                               osg::Texture::CLAMP, osg::Texture::CLAMP);

        osg::ref_ptr<osg::Texture2D> circleTexPtr = mSceneManager->getTextureManager()->getTexture2D(circleTex,
                                                                                               osg::Texture::CLAMP, osg::Texture::CLAMP);

        mUpdater->setTextures(phaseTexPtr, circleTexPtr);
    }

    void setPhase(const Phase& phase)
    {
        if (mPhase == phase)
            return;
        mPhase = phase;

        std::string textureName = "textures/tx_";

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
            setTextures(textureName, "textures/tx_mooncircle_full_s.dds");
        else
            setTextures(textureName, "textures/tx_mooncircle_full_m.dds");
    }

    void setType(const Type& type)
    {
        mType = type;
    }

    class MoonUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        MoonUpdater()
            : mFade(0.f)
            , mMoonColor(1,1,1,1)
        {
        }

        virtual void setDefaults(osg::StateSet *stateset)
        {
            stateset->setTextureAttributeAndModes(0, mPhaseTex, osg::StateAttribute::ON);
            osg::ref_ptr<osg::TexEnvCombine> texEnv = new osg::TexEnvCombine;
            texEnv->setCombine_RGB(osg::TexEnvCombine::MODULATE);
            texEnv->setSource0_RGB(osg::TexEnvCombine::CONSTANT);
            texEnv->setSource1_RGB(osg::TexEnvCombine::TEXTURE);
            texEnv->setConstantColor(osg::Vec4f(1.f, 0.f, 0.f, 1.f)); // fade * MoonRedColor
            stateset->setTextureAttributeAndModes(0, texEnv, osg::StateAttribute::ON);

            stateset->setTextureAttributeAndModes(1, mCircleTex, osg::StateAttribute::ON);
            osg::ref_ptr<osg::TexEnvCombine> texEnv2 = new osg::TexEnvCombine;
            texEnv2->setCombine_RGB(osg::TexEnvCombine::ADD);
            texEnv2->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
            texEnv2->setSource0_Alpha(osg::TexEnvCombine::TEXTURE);
            texEnv2->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
            texEnv2->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
            texEnv2->setSource1_RGB(osg::TexEnvCombine::CONSTANT);
            texEnv2->setConstantColor(osg::Vec4f(0.f, 0.f, 0.f, 1.f)); // atmospherecolor
            stateset->setTextureAttributeAndModes(1, texEnv2, osg::StateAttribute::ON);

            stateset->setAttributeAndModes(createUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        }

        virtual void apply(osg::StateSet *stateset, osg::NodeVisitor*)
        {
            osg::TexEnvCombine* texEnv = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(0, osg::StateAttribute::TEXENV));
            texEnv->setConstantColor(mMoonColor * mFade);

            osg::TexEnvCombine* texEnv2 = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(1, osg::StateAttribute::TEXENV));
            const float backdropFadeThreshold = 0.03;
            if (mFade <= backdropFadeThreshold)
            {
                texEnv2->setConstantColor(osg::Vec4f(mAtmosphereColor.x(), mAtmosphereColor.y(), mAtmosphereColor.z(), mFade / backdropFadeThreshold));
            }
            else
                texEnv2->setConstantColor(mAtmosphereColor);
        }

        void setFade (const float fade)
        {
            mFade = fade;
        }

        void setAtmosphereColor(const osg::Vec4f& color)
        {
            mAtmosphereColor = color;
        }

        void setMoonColor(const osg::Vec4f& color)
        {
            mMoonColor = color;
        }

        void setTextures(osg::ref_ptr<osg::Texture2D> phaseTex, osg::ref_ptr<osg::Texture2D> circleTex)
        {
            mPhaseTex = phaseTex;
            mCircleTex = circleTex;
            reset();
        }

    private:
        float mFade;
        osg::Vec4f mAtmosphereColor;
        osg::Vec4f mMoonColor;
        osg::ref_ptr<osg::Texture2D> mPhaseTex;
        osg::ref_ptr<osg::Texture2D> mCircleTex;
    };


    void setAtmosphereColor(const osg::Vec4f& color)
    {
        mUpdater->setAtmosphereColor(color);
    }

    void setColor(const osg::Vec4f& color)
    {
        mUpdater->setMoonColor(color);
    }

    void setFade(const float fade)
    {
        mUpdater->setFade(fade);
    }

    unsigned int getPhaseInt() const
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

private:
    Type mType;
    Phase mPhase;
    osg::ref_ptr<MoonUpdater> mUpdater;
};

SkyManager::SkyManager(osg::Group* parentNode, Resource::SceneManager* sceneManager)
    : mSceneManager(sceneManager)
    , mAtmosphereNightRoll(0.f)
    , mCreated(false)
    , mIsStorm(false)
    , mDay(0)
    , mMonth(0)
    , mCloudAnimationTimer(0.f)
    , mRainTimer(0.f)
    , mStormDirection(0,-1,0)
    , mClouds()
    , mNextClouds()
    , mCloudBlendFactor(0.0f)
    , mCloudOpacity(0.0f)
    , mCloudSpeed(0.0f)
    , mStarsOpacity(0.0f)
    , mRemainingTransitionTime(0.0f)
    , mGlare(0.0f)
    , mGlareFade(0.0f)
    , mRainEnabled(false)
    , mRainSpeed(0)
    , mRainFrequency(1)
    , mWindSpeed(0.f)
    , mEnabled(true)
    , mSunEnabled(true)
{
    osg::ref_ptr<CameraRelativeTransform> skyroot (new CameraRelativeTransform);
    skyroot->setNodeMask(Mask_Sky);
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

    mAtmosphereUpdater = new AtmosphereUpdater;
    mAtmosphereDay->addUpdateCallback(mAtmosphereUpdater);

    mAtmosphereNightNode = new osg::PositionAttitudeTransform;
    mAtmosphereNightNode->setNodeMask(0);
    mRootNode->addChild(mAtmosphereNightNode);

    osg::ref_ptr<osg::Node> atmosphereNight;
    if (mSceneManager->getVFS()->exists("meshes/sky_night_02.nif"))
        atmosphereNight = mSceneManager->createInstance("meshes/sky_night_02.nif", mAtmosphereNightNode);
    else
        atmosphereNight = mSceneManager->createInstance("meshes/sky_night_01.nif", mAtmosphereNightNode);
    atmosphereNight->getOrCreateStateSet()->setAttributeAndModes(createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    ModVertexAlphaVisitor modStars(2);
    atmosphereNight->accept(modStars);
    mAtmosphereNightUpdater = new AtmosphereNightUpdater(mSceneManager->getTextureManager());
    atmosphereNight->addUpdateCallback(mAtmosphereNightUpdater);

    mSun.reset(new Sun(mRootNode, mSceneManager));

    const MWWorld::Fallback* fallback=MWBase::Environment::get().getWorld()->getFallback();
    mMasser.reset(new Moon(mRootNode, mSceneManager, fallback->getFallbackFloat("Moons_Masser_Size")/125, Moon::Type_Masser));
    mSecunda.reset(new Moon(mRootNode, mSceneManager, fallback->getFallbackFloat("Moons_Secunda_Size")/125, Moon::Type_Secunda));

    mCloudNode = new osg::PositionAttitudeTransform;
    mRootNode->addChild(mCloudNode);
    mCloudMesh = mSceneManager->createInstance("meshes/sky_clouds_01.nif", mCloudNode);
    ModVertexAlphaVisitor modClouds(1);
    mCloudMesh->accept(modClouds);
    mCloudUpdater = new CloudUpdater;
    mCloudMesh->addUpdateCallback(mCloudUpdater);

    mCloudMesh2 = mSceneManager->createInstance("meshes/sky_clouds_01.nif", mCloudNode);
    mCloudMesh2->accept(modClouds);
    mCloudUpdater2 = new CloudUpdater;
    mCloudMesh2->addUpdateCallback(mCloudUpdater2);

    osg::ref_ptr<osg::Depth> depth = new osg::Depth;
    depth->setWriteMask(false);
    mRootNode->getOrCreateStateSet()->setAttributeAndModes(depth, osg::StateAttribute::ON);
    mRootNode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    mRootNode->getOrCreateStateSet()->setMode(GL_FOG, osg::StateAttribute::OFF);

    mMoonScriptColor = fallback->getFallbackColour("Moons_Script_Color");

    mCreated = true;
}

class RainShooter : public osgParticle::Shooter
{
public:
    RainShooter()
        : mAngle(0.f)
    {
    }

    virtual void shoot(osgParticle::Particle* particle) const
    {
        particle->setVelocity(mVelocity);
        particle->setAngle(osg::Vec3f(-mAngle, 0, (Misc::Rng::rollProbability() * 2 - 1) * osg::PI));
    }

    void setVelocity(const osg::Vec3f& velocity)
    {
        mVelocity = velocity;
    }

    void setAngle(float angle)
    {
        mAngle = angle;
    }

    virtual osg::Object* cloneType() const
    {
        return new RainShooter;
    }
    virtual osg::Object* clone(const osg::CopyOp &) const
    {
        return new RainShooter(*this);
    }

private:
    osg::Vec3f mVelocity;
    float mAngle;
};

// Updater for alpha value on a node's StateSet. Assumes the node has an existing Material StateAttribute.
class AlphaFader : public SceneUtil::StateSetUpdater
{
public:
    AlphaFader()
        : mAlpha(1.f)
    {
    }

    void setAlpha(float alpha)
    {
        mAlpha = alpha;
    }

    virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
    {
        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,0,mAlpha));
    }

    // Helper for adding AlphaFader to a subgraph
    class SetupVisitor : public osg::NodeVisitor
    {
    public:
        SetupVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
            mAlphaFader = new AlphaFader;
        }

        virtual void apply(osg::Node &node)
        {
            if (osg::StateSet* stateset = node.getStateSet())
            {
                if (stateset->getAttribute(osg::StateAttribute::MATERIAL))
                {
                    SceneUtil::CompositeStateSetUpdater* composite = nullptr;
#if OSG_MIN_VERSION_REQUIRED(3,3,3)
                    osg::Callback* callback = node.getUpdateCallback();
#else
                    osg::NodeCallback* callback = node.getUpdateCallback();
#endif
                    while (callback)
                    {
                        if ((composite = dynamic_cast<SceneUtil::CompositeStateSetUpdater*>(callback)))
                            break;
                        callback = callback->getNestedCallback();
                    }

                    if (composite)
                        composite->addController(mAlphaFader);
                    else
                        node.addUpdateCallback(mAlphaFader);
                }
            }
            traverse(node);
        }

        osg::ref_ptr<AlphaFader> getAlphaFader()
        {
            return mAlphaFader;
        }

    private:
        osg::ref_ptr<AlphaFader> mAlphaFader;
    };

private:
    float mAlpha;
};

class RainFader : public AlphaFader
{
public:
    virtual void setDefaults(osg::StateSet* stateset)
    {
        osg::ref_ptr<osg::Material> mat (new osg::Material);
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,0,1));
        mat->setColorMode(osg::Material::OFF);
        stateset->setAttributeAndModes(mat, osg::StateAttribute::ON);
    }
};

void SkyManager::createRain()
{
    if (mRainNode)
        return;

    mRainNode = new osg::Group;

    mRainParticleSystem = new osgParticle::ParticleSystem;
    mRainParticleSystem->setParticleAlignment(osgParticle::ParticleSystem::FIXED);
    mRainParticleSystem->setAlignVectorX(osg::Vec3f(0.1,0,0));
    mRainParticleSystem->setAlignVectorY(osg::Vec3f(0,0,-1));

    osg::ref_ptr<osg::StateSet> stateset (mRainParticleSystem->getOrCreateStateSet());
    stateset->setTextureAttributeAndModes(0, mSceneManager->getTextureManager()->getTexture2D("textures/tx_raindrop_01.dds",
        osg::Texture::CLAMP, osg::Texture::CLAMP), osg::StateAttribute::ON);
    stateset->setNestRenderBins(false);
    stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    osgParticle::Particle& particleTemplate = mRainParticleSystem->getDefaultParticleTemplate();
    particleTemplate.setSizeRange(osgParticle::rangef(5.f, 15.f));
    particleTemplate.setAlphaRange(osgParticle::rangef(1.f, 1.f));
    particleTemplate.setLifeTime(1);

    osg::ref_ptr<osgParticle::ModularEmitter> emitter (new osgParticle::ModularEmitter);
    emitter->setParticleSystem(mRainParticleSystem);

    osg::ref_ptr<osgParticle::BoxPlacer> placer (new osgParticle::BoxPlacer);
    placer->setXRange(-300, 300); // Rain_Diameter
    placer->setYRange(-300, 300);
    placer->setZRange(300, 300);
    emitter->setPlacer(placer);

    osg::ref_ptr<osgParticle::ConstantRateCounter> counter (new osgParticle::ConstantRateCounter);
    counter->setNumberOfParticlesPerSecondToCreate(600.0);
    emitter->setCounter(counter);

    osg::ref_ptr<RainShooter> shooter (new RainShooter);
    mRainShooter = shooter;
    emitter->setShooter(shooter);

    osg::ref_ptr<osgParticle::ParticleSystemUpdater> updater (new osgParticle::ParticleSystemUpdater);
    updater->addParticleSystem(mRainParticleSystem);

    osg::ref_ptr<osg::Geode> geode (new osg::Geode);
    geode->addDrawable(mRainParticleSystem);

    mRainNode->addChild(emitter);
    mRainNode->addChild(geode);
    mRainNode->addChild(updater);

    mRainFader = new RainFader;
    mRainNode->addUpdateCallback(mRainFader);

    mRootNode->addChild(mRainNode);
}

void SkyManager::destroyRain()
{
    if (!mRainNode)
        return;

    mRootNode->removeChild(mRainNode);
    mRainNode = nullptr;
    mRainParticleSystem = nullptr;
    mRainShooter = nullptr;
    mRainFader = nullptr;
}

SkyManager::~SkyManager()
{
    if (mRootNode)
    {
        mRootNode->getParent(0)->removeChild(mRootNode);
        mRootNode = nullptr;
    }
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

    if (mIsStorm)
    {
        osg::Quat quat;
        quat.makeRotate(osg::Vec3f(0,1,0), mStormDirection);

        if (mParticleNode)
            mParticleNode->setAttitude(quat);
        mCloudNode->setAttitude(quat);
    }
    else
        mCloudNode->setAttitude(osg::Quat());

    // UV Scroll the clouds
    mCloudAnimationTimer += duration * mCloudSpeed * 0.003;
    mCloudUpdater->setAnimationTimer(mCloudAnimationTimer);
    mCloudUpdater2->setAnimationTimer(mCloudAnimationTimer);

    /// \todo improve this
    mMasser->setPhase( static_cast<Moon::Phase>( (int) ((mDay % 32)/4.f)) );
    mSecunda->setPhase ( static_cast<Moon::Phase>( (int) ((mDay % 32)/4.f)) );

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

    // rotate the stars by 360 degrees every 4 days
    mAtmosphereNightRoll += MWBase::Environment::get().getWorld()->getTimeScaleFactor()*duration*osg::DegreesToRadians(360.f) / (3600*96.f);
    if (mAtmosphereNightNode->getNodeMask() != 0)
        mAtmosphereNightNode->setAttitude(osg::Quat(mAtmosphereNightRoll, osg::Vec3f(0,0,1)));
}

void SkyManager::setEnabled(bool enabled)
{
    if (enabled && !mCreated)
        create();

    mRootNode->setNodeMask(enabled ? Mask_Sky : 0);

    mEnabled = enabled;
}

void SkyManager::setMoonColour (bool red)
{
    if (!mCreated) return;
    mSecunda->setColor(red ? mMoonScriptColor : osg::Vec4f(1,1,1,1));
}

void SkyManager::updateRainParameters()
{
    if (mRainShooter)
    {
        float windFactor = mWindSpeed/3.f;
        float angle = windFactor * osg::PI/4;
        mRainShooter->setVelocity(osg::Vec3f(0, mRainSpeed * windFactor, -mRainSpeed));
        mRainShooter->setAngle(angle);
    }
}

void SkyManager::setWeather(const MWWorld::WeatherResult& weather)
{
    if (!mCreated) return;

    if (mRainEffect != weather.mRainEffect)
    {
        mRainEffect = weather.mRainEffect;
        if (!mRainEffect.empty())
        {
            createRain();
        }
        else
        {
            destroyRain();
        }
    }

    mRainFrequency = weather.mRainFrequency;
    mRainSpeed = weather.mRainSpeed;
    mWindSpeed = weather.mWindSpeed;
    updateRainParameters();

    mIsStorm = weather.mIsStorm;

    if (mCurrentParticleEffect != weather.mParticleEffect)
    {
        mCurrentParticleEffect = weather.mParticleEffect;

        if (mCurrentParticleEffect.empty())
        {
            if (mParticleNode)
            {
                mRootNode->removeChild(mParticleNode);
                mParticleNode = nullptr;
            }
            mParticleEffect = nullptr;
            mParticleFader = nullptr;
        }
        else
        {
            if (!mParticleNode)
            {
                mParticleNode = new osg::PositionAttitudeTransform;
                mRootNode->addChild(mParticleNode);
            }
            mParticleEffect = mSceneManager->createInstance(mCurrentParticleEffect, mParticleNode);

            SceneUtil::AssignControllerSourcesVisitor assignVisitor(std::shared_ptr<SceneUtil::ControllerSource>(new SceneUtil::FrameTimeSource));
            mParticleEffect->accept(assignVisitor);

            AlphaFader::SetupVisitor alphaFaderSetupVisitor;
            mParticleEffect->accept(alphaFaderSetupVisitor);
            mParticleFader = alphaFaderSetupVisitor.getAlphaFader();
        }
    }

    if (mClouds != weather.mCloudTexture)
    {
        mClouds = weather.mCloudTexture;

        std::string texture = Misc::ResourceHelpers::correctTexturePath(mClouds, mSceneManager->getVFS());

        mCloudUpdater->setTexture(mSceneManager->getTextureManager()->getTexture2D(texture,
                                                                                   osg::Texture::REPEAT, osg::Texture::REPEAT));
    }

    if (mNextClouds != weather.mNextCloudTexture)
    {
        mNextClouds = weather.mNextCloudTexture;

        std::string texture = Misc::ResourceHelpers::correctTexturePath(mNextClouds, mSceneManager->getVFS());

        if (!texture.empty())
            mCloudUpdater2->setTexture(mSceneManager->getTextureManager()->getTexture2D(texture,
                                                                                       osg::Texture::REPEAT, osg::Texture::REPEAT));
    }

    if (mCloudBlendFactor != weather.mCloudBlendFactor
            || mCloudOpacity != weather.mCloudOpacity)
    {
        mCloudBlendFactor = weather.mCloudBlendFactor;
        mCloudOpacity = weather.mCloudOpacity;

        mCloudUpdater->setOpacity(mCloudOpacity * (1.f-mCloudBlendFactor));
        mCloudUpdater2->setOpacity(mCloudOpacity * mCloudBlendFactor);
        mCloudMesh2->setNodeMask(mCloudBlendFactor > 0.f ? ~0 : 0);
    }

    if (mCloudColour != weather.mSunColor)
    {
        // FIXME: this doesn't look correct
        osg::Vec4f clr( weather.mSunColor.r()*0.7f + weather.mAmbientColor.r()*0.7f,
                        weather.mSunColor.g()*0.7f + weather.mAmbientColor.g()*0.7f,
                        weather.mSunColor.b()*0.7f + weather.mAmbientColor.b()*0.7f, 1.f);

        mCloudUpdater->setEmissionColor(clr);
        mCloudUpdater2->setEmissionColor(clr);

        mCloudColour = weather.mSunColor;
    }

    if (mSkyColour != weather.mSkyColor)
    {
        mSkyColour = weather.mSkyColor;

        mAtmosphereUpdater->setEmissionColor(mSkyColour);
        mMasser->setAtmosphereColor(mSkyColour);
        mSecunda->setAtmosphereColor(mSkyColour);
    }

    if (mFogColour != weather.mFogColor)
    {
        mFogColour = weather.mFogColor;
    }

    mCloudSpeed = weather.mCloudSpeed;

    if (weather.mNight && mStarsOpacity != weather.mNightFade)
    {
        mStarsOpacity = weather.mNightFade;

        mAtmosphereNightUpdater->setFade(mStarsOpacity);
    }

    mAtmosphereNightNode->setNodeMask(weather.mNight ? ~0 : 0);


    /*
    float strength;
    float timeofday_angle = std::abs(mSunGlare->getPosition().z/mSunGlare->getPosition().length());
    if (timeofday_angle <= 0.44)
        strength = timeofday_angle/0.44f;
    else
        strength = 1.f;

    mSunGlare->setVisibility(weather.mGlareView * mGlareFade * strength);

    mSun->setVisibility(weather.mGlareView * strength);
    */

    if (mRainFader)
        mRainFader->setAlpha(weather.mEffectFade * 0.6); // * Rain_Threshold?
    if (mParticleFader)
        mParticleFader->setAlpha(weather.mEffectFade);
}

void SkyManager::setGlare(const float glare)
{
    mGlare = glare;
}

void SkyManager::sunEnable()
{
    if (!mCreated) return;

    mSun->setVisible(true);
}

void SkyManager::sunDisable()
{
    if (!mCreated) return;

    mSun->setVisible(false);
}

void SkyManager::setStormDirection(const osg::Vec3f &direction)
{
    mStormDirection = direction;
}

void SkyManager::setSunDirection(const osg::Vec3f& direction)
{
    if (!mCreated) return;

    mSun->setDirection(direction);

    //mSunGlare->setPosition(direction);
}

void SkyManager::setMasserDirection(const osg::Vec3f& direction)
{
    if (!mCreated) return;

    mMasser->setDirection(direction);
}

void SkyManager::setSecundaDirection(const osg::Vec3f& direction)
{
    if (!mCreated) return;

    mSecunda->setDirection(direction);
}

void SkyManager::masserEnable()
{
    if (!mCreated) return;

    mMasser->setVisible(true);
}

void SkyManager::secundaEnable()
{
    if (!mCreated) return;

    mSecunda->setVisible(true);
}

void SkyManager::masserDisable()
{
    if (!mCreated) return;

    mMasser->setVisible(false);
}

void SkyManager::secundaDisable()
{
    if (!mCreated) return;

    mSecunda->setVisible(false);
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
    mMasser->setFade(fade);
}

void SkyManager::setSecundaFade(const float fade)
{
    if (!mCreated) return;
    mSecunda->setFade(fade);
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
