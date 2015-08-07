#ifndef GAME_MWWORLD_WEATHER_H
#define GAME_MWWORLD_WEATHER_H

#include <stdint.h>
#include <string>
#include <map>

#include <osg/Vec4f>

#include "../mwbase/soundmanager.hpp"

#include "../mwrender/sky.hpp"

namespace ESM
{
    struct Region;
    class ESMWriter;
    class ESMReader;
}

namespace MWRender
{
    class RenderingManager;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class Fallback;

    /// Defines a single weather setting (according to INI)
    struct Weather
    {
        std::string mCloudTexture;

        // Sky (atmosphere) colors
        osg::Vec4f   mSkySunriseColor,
                            mSkyDayColor,
                            mSkySunsetColor,
                            mSkyNightColor;

        // Fog colors
        osg::Vec4f   mFogSunriseColor,
                            mFogDayColor,
                            mFogSunsetColor,
                            mFogNightColor;

        // Ambient lighting colors
        osg::Vec4f   mAmbientSunriseColor,
                            mAmbientDayColor,
                            mAmbientSunsetColor,
                            mAmbientNightColor;

        // Sun (directional) lighting colors
        osg::Vec4f   mSunSunriseColor,
                            mSunDayColor,
                            mSunSunsetColor,
                            mSunNightColor;

        // Fog depth/density
        float   mLandFogDayDepth,
                mLandFogNightDepth;

        // Color modulation for the sun itself during sunset (not completely sure)
        osg::Vec4f mSunDiscSunsetColor;

        // Duration of weather transition (in days)
        float mTransitionDelta;

        // Used by scripts to animate signs, etc based on the wind (GetWindSpeed)
        float mWindSpeed;

        // Cloud animation speed multiplier
        float mCloudSpeed;

        // Multiplier for clouds transparency
        float mCloudsMaximumPercent;

        // Value between 0 and 1, defines the strength of the sun glare effect.
        // Also appears to modify how visible the sun, moons, and stars are for various weather effects.
        float mGlareView;

        // Sound effect
        // This is used for Blight, Ashstorm and Blizzard (Bloodmoon)
        std::string mAmbientLoopSoundID;

        // Is this an ash storm / blight storm? If so, the following will happen:
        // - The particles and clouds will be oriented so they appear to come from the Red Mountain.
        // - Characters will animate their hand to protect eyes from the storm when looking in its direction (idlestorm animation)
        // - Slower movement when walking against the storm (fStromWalkMult)
        bool mIsStorm;

        // How fast does rain travel down?
        // In Morrowind.ini this is set globally, but we may want to change it per weather later.
        float mRainSpeed;

        // How often does a new rain mesh spawn?
        float mRainFrequency;

        std::string mParticleEffect;

        std::string mRainEffect;

        // Note: For Weather Blight, there is a "Disease Chance" (=0.1) setting. But according to MWSFD this feature
        // is broken in the vanilla game and was disabled.
    };

    class MoonModel
    {
    public:
        MoonModel(const std::string& name, const MWWorld::Fallback& fallback);

        MWRender::MoonState calculateState(unsigned int daysPassed, float gameHour) const;

    private:
        float mFadeInStart;
        float mFadeInFinish;
        float mFadeOutStart;
        float mFadeOutFinish;
        float mAxisOffset;
        float mSpeed;
        float mDailyIncrement;
        float mFadeStartAngle;
        float mFadeEndAngle;
        float mMoonShadowEarlyFadeAngle;

        float angle(unsigned int daysPassed, float gameHour) const;
        float moonRiseHour(unsigned int daysPassed) const;
        float rotation(float hours) const;
        unsigned int phase(unsigned int daysPassed, float gameHour) const;
        float shadowBlend(float angle) const;
        float hourlyAlpha(float gameHour) const;
        float earlyMoonShadowAlpha(float angle) const;
    };

    ///
    /// Interface for weather settings
    ///
    class WeatherManager
    {
    public:
        // Have to pass fallback and Store, can't use singleton since World isn't fully constructed yet at the time
        WeatherManager(MWRender::RenderingManager*, MWWorld::Fallback* fallback, MWWorld::ESMStore* store);
        ~WeatherManager();

        /**
         * Change the weather in the specified region
         * @param region that should be changed
         * @param ID of the weather setting to shift to
         */
        void changeWeather(const std::string& region, const unsigned int id);
        void switchToNextWeather(bool instantly = true);

        /**
         * Per-frame update
         * @param duration
         * @param paused
         */
        void update(float duration, bool paused = false);

        void stopSounds();

        void setHour(const float hour);

        float getWindSpeed() const;

        /// Are we in an ash or blight storm?
        bool isInStorm() const;

        osg::Vec3f getStormDirection() const;

        void advanceTime(double hours);

        unsigned int getWeatherID() const;

        void modRegion(const std::string &regionid, const std::vector<char> &chances);

        /// @see World::isDark
        bool isDark() const;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress);

        bool readRecord(ESM::ESMReader& reader, uint32_t type);

        void clear();

    private:
        float mHour;
        float mWindSpeed;
        bool mIsStorm;
        osg::Vec3f mStormDirection;

        MWBase::SoundPtr mAmbientSound;
        std::string mPlayingSoundID;

        MWWorld::Fallback* mFallback;
        MWWorld::ESMStore* mStore;
        void setFallbackWeather(Weather& weather,const std::string& name);
        MWRender::RenderingManager* mRendering;

        std::map<std::string, Weather> mWeatherSettings;

        std::map<std::string, std::string> mRegionOverrides;

        std::string mCurrentWeather;
        std::string mNextWeather;

        std::string mCurrentRegion;

        bool mFirstUpdate;

        float mRemainingTransitionTime;

        float mThunderFlash;
        float mThunderChance;
        float mThunderChanceNeeded;

        double mTimePassed; // time passed since last update

        void transition(const float factor);
        void setResult(const std::string& weatherType);

        void setWeather(const std::string& weatherType, bool instant=false);
        std::string nextWeather(const ESM::Region* region) const;
        MWRender::WeatherResult mResult;

        typedef std::map<std::string,std::vector<char> > RegionModMap;
        RegionModMap mRegionMods;

        float mRainSpeed;
        float mSunriseTime;
        float mSunsetTime;
        float mSunriseDuration;
        float mSunsetDuration;
        float mWeatherUpdateTime;
        float mHoursBetweenWeatherChanges;
        float mThunderFrequency;
        float mThunderThreshold;
        float mThunderSoundDelay;
        float mNightStart;
        float mNightEnd;
        float mDayStart;
        float mDayEnd;
        std::string mThunderSoundID0;
        std::string mThunderSoundID1;
        std::string mThunderSoundID2;
        std::string mThunderSoundID3;
        MoonModel mMasser;
        MoonModel mSecunda;
    };
}

#endif // GAME_MWWORLD_WEATHER_H
