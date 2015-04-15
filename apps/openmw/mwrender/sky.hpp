#ifndef OPENMW_MWRENDER_SKY_H
#define OPENMW_MWRENDER_SKY_H

#include <osg/ref_ptr>

#include "../mwworld/weather.hpp"

namespace osg
{
    class Group;
    class Node;
    class Material;
}

namespace Resource
{
    class SceneManager;
}

namespace MWRender
{
    class AtmosphereUpdater;
    class CloudUpdater;
    class Sun;
    class Moon;

    class SkyManager
    {
    public:
        SkyManager(osg::Group* parentNode, Resource::SceneManager* sceneManager);
        ~SkyManager();

        void update(float duration);

        void setEnabled(bool enabled);

        void setHour (double hour);
        ///< will be called even when sky is disabled.

        void setDate (int day, int month);
        ///< will be called even when sky is disabled.

        int getMasserPhase() const;
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon

        int getSecundaPhase() const;
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon

        void setMoonColour (bool red);
        ///< change Secunda colour to red

        void setWeather(const MWWorld::WeatherResult& weather);

        void sunEnable();

        void sunDisable();

        void setRainSpeed(float speed);

        void setStormDirection(const Ogre::Vector3& direction);

        void setSunDirection(const osg::Vec3f& direction);

        void setMasserDirection(const osg::Vec3f& direction);

        void setSecundaDirection(const osg::Vec3f& direction);

        void setMasserFade(const float fade);

        void setSecundaFade(const float fade);

        void masserEnable();
        void masserDisable();

        void secundaEnable();
        void secundaDisable();

        void setLightningStrength(const float factor);

        void setGlare(const float glare);
        void setGlareEnabled(bool enabled);

    private:
        void create();
        ///< no need to call this, automatically done on first enable()

        void updateRain(float dt);
        void clearRain();

        Resource::SceneManager* mSceneManager;

        osg::ref_ptr<osg::Group> mRootNode;

        osg::ref_ptr<osg::Node> mParticleEffect;

        osg::ref_ptr<osg::Node> mCloudNode;

        osg::ref_ptr<CloudUpdater> mCloudUpdater;

        osg::ref_ptr<osg::Node> mAtmosphereDay;

        osg::ref_ptr<osg::Node> mAtmosphereNight;

        osg::ref_ptr<AtmosphereUpdater> mAtmosphereUpdater;

        std::auto_ptr<Sun> mSun;
        std::auto_ptr<Moon> mMasser;
        std::auto_ptr<Moon> mSecunda;

        bool mCreated;

        bool mMoonRed;

        bool mIsStorm;

        float mHour;
        int mDay;
        int mMonth;

        float mCloudAnimationTimer;

        float mRainTimer;

        Ogre::Vector3 mStormDirection;

        // remember some settings so we don't have to apply them again if they didnt change
        Ogre::String mClouds;
        Ogre::String mNextClouds;
        float mCloudBlendFactor;
        float mCloudOpacity;
        float mCloudSpeed;
        float mStarsOpacity;
        osg::Vec4f mCloudColour;
        osg::Vec4f mSkyColour;
        osg::Vec4f mFogColour;

        std::string mCurrentParticleEffect;

        float mRemainingTransitionTime;

        float mGlare; // target
        float mGlareFade; // actual

        bool mRainEnabled;
        std::string mRainEffect;
        float mRainSpeed;
        float mRainFrequency;

        bool mEnabled;
        bool mSunEnabled;
        bool mMasserEnabled;
        bool mSecundaEnabled;
    };
}

#endif // GAME_RENDER_SKY_H
