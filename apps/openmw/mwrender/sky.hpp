#ifndef OPENMW_MWRENDER_SKY_H
#define OPENMW_MWRENDER_SKY_H

#include <memory>
#include <string>
#include <vector>

#include <osg/Vec4f>
#include <osg/ref_ptr>

#include <components/vfs/pathutil.hpp>

#include "precipitationocclusion.hpp"
#include "skyutil.hpp"

namespace osg
{
    class Group;
    class Node;
    class Material;
    class PositionAttitudeTransform;
    class Camera;
}

namespace osgParticle
{
    class ParticleSystem;
    class BoxPlacer;
}

namespace Resource
{
    class SceneManager;
}

namespace SceneUtil
{
    class RTTNode;
}

namespace MWRender
{
    ///@brief The SkyManager handles rendering of the sky domes, celestial bodies as well as other objects that need to
    /// be rendered
    /// relative to the camera (e.g. weather particle effects)
    class SkyManager
    {
    public:
        SkyManager(osg::Group* parentNode, osg::Group* rootNode, osg::Camera* camera,
            Resource::SceneManager* sceneManager, bool enableSkyRTT);
        ~SkyManager();

        void update(float duration);

        void setEnabled(bool enabled);

        void setHour(double hour);
        ///< will be called even when sky is disabled.

        void setDate(int day, int month);
        ///< will be called even when sky is disabled.

        int getMasserPhase() const;
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon

        int getSecundaPhase() const;
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon

        void setMoonColour(bool red);
        ///< change Secunda colour to red

        void setWeather(const WeatherResult& weather);

        void sunEnable();

        void sunDisable();

        bool isEnabled();

        bool hasRain() const;

        bool getRainRipplesEnabled() const;

        float getPrecipitationAlpha() const;

        void setRainSpeed(float speed);

        void setStormParticleDirection(const osg::Vec3f& direction);

        void setSunDirection(const osg::Vec3f& direction);

        void setMasserState(const MoonState& state);
        void setSecundaState(const MoonState& state);

        void setGlareTimeOfDayFade(float val);

        /// Enable or disable the water plane (used to remove underwater weather particles)
        void setWaterEnabled(bool enabled);

        /// Set height of water plane (used to remove underwater weather particles)
        void setWaterHeight(float height);

        void listAssetsToPreload(
            std::vector<VFS::Path::Normalized>& models, std::vector<VFS::Path::Normalized>& textures);

        float getBaseWindSpeed() const;

        void setSunglare(bool enabled);

        SceneUtil::RTTNode* getSkyRTT() { return mSkyRTT.get(); }

        osg::Vec4f getSkyColor() const { return mSkyColour; }

    private:
        void create();
        ///< no need to call this, automatically done on first enable()

        void createRain();
        void destroyRain();
        void switchUnderwaterRain();
        void updateRainParameters();

        Resource::SceneManager* mSceneManager;

        osg::Camera* mCamera;

        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Group> mEarlyRenderBinRoot;

        osg::ref_ptr<osg::PositionAttitudeTransform> mParticleNode;
        osg::ref_ptr<osg::Node> mParticleEffect;
        osg::ref_ptr<UnderwaterSwitchCallback> mUnderwaterSwitch;

        osg::ref_ptr<osg::Group> mCloudNode;

        osg::ref_ptr<CloudUpdater> mCloudUpdater;
        osg::ref_ptr<CloudUpdater> mNextCloudUpdater;
        osg::ref_ptr<osg::PositionAttitudeTransform> mCloudMesh;
        osg::ref_ptr<osg::PositionAttitudeTransform> mNextCloudMesh;

        osg::ref_ptr<osg::Node> mAtmosphereDay;

        osg::ref_ptr<osg::PositionAttitudeTransform> mAtmosphereNightNode;
        float mAtmosphereNightRoll;
        osg::ref_ptr<AtmosphereNightUpdater> mAtmosphereNightUpdater;

        osg::ref_ptr<AtmosphereUpdater> mAtmosphereUpdater;

        std::unique_ptr<Sun> mSun;
        std::unique_ptr<Moon> mMasser;
        std::unique_ptr<Moon> mSecunda;

        osg::ref_ptr<osg::Group> mRainNode;
        osg::ref_ptr<osgParticle::ParticleSystem> mRainParticleSystem;
        osg::ref_ptr<osgParticle::BoxPlacer> mPlacer;
        osg::ref_ptr<RainCounter> mCounter;
        osg::ref_ptr<RainShooter> mRainShooter;

        bool mPrecipitationOcclusion = false;
        std::unique_ptr<PrecipitationOccluder> mPrecipitationOccluder;

        bool mCreated;

        bool mIsStorm;

        int mDay;
        int mMonth;

        bool mTimescaleClouds;
        float mCloudAnimationTimer;

        float mRainTimer;

        // particle system rotation is independent of cloud rotation internally
        osg::Vec3f mStormParticleDirection;
        osg::Vec3f mStormDirection;
        osg::Vec3f mNextStormDirection;

        // remember some settings so we don't have to apply them again if they didn't change
        std::string mClouds;
        std::string mNextClouds;
        float mCloudBlendFactor;
        float mCloudSpeed;
        float mStarsOpacity;
        osg::Vec4f mCloudColour;
        osg::Vec4f mSkyColour;
        osg::Vec4f mFogColour;

        std::string mCurrentParticleEffect;

        float mRemainingTransitionTime;

        bool mRainEnabled;
        std::string mRainEffect;
        float mRainSpeed;
        float mRainDiameter;
        float mRainMinHeight;
        float mRainMaxHeight;
        float mRainEntranceSpeed;
        int mRainMaxRaindrops;
        bool mRainRipplesEnabled;
        bool mSnowRipplesEnabled;
        float mWindSpeed;
        float mBaseWindSpeed;

        bool mEnabled;
        bool mSunEnabled;
        bool mSunglareEnabled;

        float mPrecipitationAlpha;
        bool mDirtyParticlesEffect;

        osg::Vec4f mMoonScriptColor;

        osg::ref_ptr<SceneUtil::RTTNode> mSkyRTT;
    };
}

#endif
