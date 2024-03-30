#ifndef OPENMW_MWRENDER_SKYUTIL_H
#define OPENMW_MWRENDER_SKYUTIL_H

#include <osg/Material>
#include <osg/Matrixf>
#include <osg/Texture2D>
#include <osg/Transform>
#include <osg/Vec4f>

#include <osgParticle/ConstantRateCounter>
#include <osgParticle/Shooter>

#include <components/esm/refid.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/sceneutil/statesetupdater.hpp>

namespace Resource
{
    class ImageManager;
    class SceneManager;
}

namespace MWRender
{
    struct MoonUpdater;
    class SunUpdater;
    class SunFlashCallback;
    class SunGlareCallback;

    struct WeatherResult
    {
        std::string mCloudTexture;
        std::string mNextCloudTexture;
        float mCloudBlendFactor;

        osg::Vec4f mFogColor;

        osg::Vec4f mAmbientColor;

        osg::Vec4f mSkyColor;

        // sun light color
        osg::Vec4f mSunColor;

        // alpha is the sun transparency
        osg::Vec4f mSunDiscColor;

        float mFogDepth;

        float mDLFogFactor;
        float mDLFogOffset;

        float mWindSpeed;
        float mBaseWindSpeed;
        float mCurrentWindSpeed;
        float mNextWindSpeed;

        float mCloudSpeed;

        float mGlareView;

        bool mNight; // use night skybox
        float mNightFade; // fading factor for night skybox

        bool mIsStorm;

        ESM::RefId mAmbientLoopSoundID;
        ESM::RefId mRainLoopSoundID;
        float mAmbientSoundVolume;

        std::string mParticleEffect;
        std::string mRainEffect;
        float mPrecipitationAlpha;

        float mRainDiameter;
        float mRainMinHeight;
        float mRainMaxHeight;
        float mRainSpeed;
        float mRainEntranceSpeed;
        int mRainMaxRaindrops;

        osg::Vec3f mStormDirection;
        osg::Vec3f mNextStormDirection;
    };

    struct MoonState
    {
        enum class Phase
        {
            Full,
            WaningGibbous,
            ThirdQuarter,
            WaningCrescent,
            New,
            WaxingCrescent,
            FirstQuarter,
            WaxingGibbous,
            Unspecified
        };

        float mRotationFromHorizon;
        float mRotationFromNorth;
        Phase mPhase;
        float mShadowBlend;
        float mMoonAlpha;
    };

    osg::ref_ptr<osg::Material> createAlphaTrackingUnlitMaterial();
    osg::ref_ptr<osg::Material> createUnlitMaterial(osg::Material::ColorMode colorMode = osg::Material::OFF);

    class OcclusionCallback
    {
    public:
        OcclusionCallback(
            osg::ref_ptr<osg::OcclusionQueryNode> oqnVisible, osg::ref_ptr<osg::OcclusionQueryNode> oqnTotal);

    protected:
        float getVisibleRatio(osg::Camera* camera);

    private:
        osg::ref_ptr<osg::OcclusionQueryNode> mOcclusionQueryVisiblePixels;
        osg::ref_ptr<osg::OcclusionQueryNode> mOcclusionQueryTotalPixels;

        std::map<osg::observer_ptr<osg::Camera>, float> mLastRatio;
    };

    class AtmosphereUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        void setEmissionColor(const osg::Vec4f& emissionColor);

    protected:
        void setDefaults(osg::StateSet* stateset) override;
        void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/) override;

    private:
        osg::Vec4f mEmissionColor;
    };

    class AtmosphereNightUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        AtmosphereNightUpdater(Resource::ImageManager* imageManager, bool forceShaders);

        void setFade(float fade);

    protected:
        void setDefaults(osg::StateSet* stateset) override;

        void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/) override;

    private:
        osg::Vec4f mColor;
        osg::ref_ptr<osg::Texture2D> mTexture;
        bool mForceShaders;
    };

    class CloudUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        CloudUpdater(bool forceShaders);

        void setTexture(osg::ref_ptr<osg::Texture2D> texture);

        void setEmissionColor(const osg::Vec4f& emissionColor);
        void setOpacity(float opacity);
        void setTextureCoord(float timer);

    protected:
        void setDefaults(osg::StateSet* stateset) override;
        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override;

    private:
        osg::ref_ptr<osg::Texture2D> mTexture;
        osg::Vec4f mEmissionColor;
        float mOpacity;
        bool mForceShaders;
        osg::Matrixf mTexMat;
    };

    /// Transform that removes the eyepoint of the modelview matrix,
    /// i.e. its children are positioned relative to the camera.
    class CameraRelativeTransform : public osg::Transform
    {
    public:
        CameraRelativeTransform();

        CameraRelativeTransform(const CameraRelativeTransform& copy, const osg::CopyOp& copyop);

        META_Node(MWRender, CameraRelativeTransform)

        const osg::Vec3f& getLastViewPoint() const;

        bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const override;

        osg::BoundingSphere computeBound() const override;

    private:
        // viewPoint for the current frame
        mutable osg::Vec3f mViewPoint;
    };

    /// @brief Hides the node subgraph if the eye point is below water.
    /// @note Must be added as cull callback.
    /// @note Meant to be used on a node that is child of a CameraRelativeTransform.
    /// The current view point must be retrieved by the CameraRelativeTransform since we can't get it anymore once we
    /// are in camera-relative space.
    class UnderwaterSwitchCallback : public SceneUtil::NodeCallback<UnderwaterSwitchCallback>
    {
    public:
        UnderwaterSwitchCallback(CameraRelativeTransform* cameraRelativeTransform);
        bool isUnderwater();

        void operator()(osg::Node* node, osg::NodeVisitor* nv);
        void setEnabled(bool enabled);
        void setWaterLevel(float waterLevel);

    private:
        osg::ref_ptr<CameraRelativeTransform> mCameraRelativeTransform;
        bool mEnabled;
        float mWaterLevel;
    };

    /// A base class for the sun and moons.
    class CelestialBody
    {
    public:
        CelestialBody(osg::Group* parentNode, float scaleFactor, int numUvSets, unsigned int visibleMask = ~0u);

        virtual ~CelestialBody() = default;

        virtual void adjustTransparency(const float ratio) = 0;

        void setVisible(bool visible);

    protected:
        unsigned int mVisibleMask;
        static const float mDistance;
        osg::ref_ptr<osg::PositionAttitudeTransform> mTransform;
        osg::ref_ptr<osg::Geometry> mGeom;
    };

    class Sun : public CelestialBody
    {
    public:
        Sun(osg::Group* parentNode, Resource::SceneManager& sceneManager);

        ~Sun();

        void setColor(const osg::Vec4f& color);
        void adjustTransparency(const float ratio) override;

        void setDirection(const osg::Vec3f& direction);
        void setGlareTimeOfDayFade(float val);
        void setSunglare(bool enabled);

    private:
        /// @param queryVisible If true, queries the amount of visible pixels. If false, queries the total amount of
        /// pixels.
        osg::ref_ptr<osg::OcclusionQueryNode> createOcclusionQueryNode(osg::Group* parent, bool queryVisible);

        void createSunFlash(Resource::ImageManager& imageManager);
        void destroySunFlash();

        void createSunGlare();
        void destroySunGlare();

        osg::ref_ptr<SunUpdater> mUpdater;
        osg::ref_ptr<osg::Node> mSunFlashNode;
        osg::ref_ptr<osg::Node> mSunGlareNode;
        osg::ref_ptr<SunFlashCallback> mSunFlashCallback;
        osg::ref_ptr<SunGlareCallback> mSunGlareCallback;
        osg::ref_ptr<osg::OcclusionQueryNode> mOcclusionQueryVisiblePixels;
        osg::ref_ptr<osg::OcclusionQueryNode> mOcclusionQueryTotalPixels;
    };

    class Moon : public CelestialBody
    {
    public:
        enum Type
        {
            Type_Masser = 0,
            Type_Secunda
        };

        Moon(osg::Group* parentNode, Resource::SceneManager& sceneManager, float scaleFactor, Type type);

        ~Moon();

        void adjustTransparency(const float ratio) override;
        void setState(const MoonState state);
        void setAtmosphereColor(const osg::Vec4f& color);
        void setColor(const osg::Vec4f& color);

        unsigned int getPhaseInt() const;

    private:
        Type mType;
        MoonState::Phase mPhase;
        osg::ref_ptr<MoonUpdater> mUpdater;

        void setPhase(const MoonState::Phase& phase);
    };

    class RainCounter : public osgParticle::ConstantRateCounter
    {
    public:
        int numParticlesToCreate(double dt) const override;
    };

    class RainShooter : public osgParticle::Shooter
    {
    public:
        RainShooter();

        osg::Object* cloneType() const override;

        osg::Object* clone(const osg::CopyOp&) const override;

        void shoot(osgParticle::Particle* particle) const override;

        void setVelocity(const osg::Vec3f& velocity);
        void setAngle(float angle);

    private:
        osg::Vec3f mVelocity;
        float mAngle;
    };

    class ModVertexAlphaVisitor : public osg::NodeVisitor
    {
    public:
        enum MeshType
        {
            Atmosphere,
            Stars,
            Clouds
        };

        ModVertexAlphaVisitor(MeshType type);

        void apply(osg::Geometry& geometry) override;

    private:
        MeshType mType;
    };
}

#endif
