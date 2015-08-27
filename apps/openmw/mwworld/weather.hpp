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
    struct RegionWeatherState;
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
    class TimeStamp;

    /// Defines a single weather setting (according to INI)
    class Weather
    {
    public:
        Weather(const std::string& name,
                const MWWorld::Fallback& fallback,
                float stormWindSpeed,
                float rainSpeed,
                const std::string& ambientLoopSoundID,
                const std::string& particleEffect);

        std::string mCloudTexture;

        // Sky (atmosphere) colors
        osg::Vec4f mSkySunriseColor;
        osg::Vec4f mSkyDayColor;
        osg::Vec4f mSkySunsetColor;
        osg::Vec4f mSkyNightColor;

        // Fog colors
        osg::Vec4f mFogSunriseColor;
        osg::Vec4f mFogDayColor;
        osg::Vec4f mFogSunsetColor;
        osg::Vec4f mFogNightColor;

        // Ambient lighting colors
        osg::Vec4f mAmbientSunriseColor;
        osg::Vec4f mAmbientDayColor;
        osg::Vec4f mAmbientSunsetColor;
        osg::Vec4f mAmbientNightColor;

        // Sun (directional) lighting colors
        osg::Vec4f mSunSunriseColor;
        osg::Vec4f mSunDayColor;
        osg::Vec4f mSunSunsetColor;
        osg::Vec4f mSunNightColor;

        // Fog depth/density
        float mLandFogDayDepth;
        float mLandFogNightDepth;

        // Color modulation for the sun itself during sunset (not completely sure)
        osg::Vec4f mSunDiscSunsetColor;

        // Used by scripts to animate signs, etc based on the wind (GetWindSpeed)
        float mWindSpeed;

        // Cloud animation speed multiplier
        float mCloudSpeed;

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

        float transitionDelta() const;
        float cloudBlendFactor(float transitionRatio) const;

    private:
        float mTransitionDelta;
        float mCloudsMaximumPercent;
    };

    /// A class for storing a region's weather.
    class RegionWeather
    {
    public:
        explicit RegionWeather(const ESM::Region& region);
        explicit RegionWeather(const ESM::RegionWeatherState& state);

        operator ESM::RegionWeatherState() const;

        void setChances(const std::vector<char>& chances);

        void setWeather(int weatherID);

        int getWeather();

    private:
        int mWeather;
        std::vector<char> mChances;

        void chooseNewWeather();
    };

    /// A class that acts as a model for the moons.
    class MoonModel
    {
    public:
        MoonModel(const std::string& name, const MWWorld::Fallback& fallback);

        MWRender::MoonState calculateState(const TimeStamp& gameTime) const;

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

        float angle(const TimeStamp& gameTime) const;
        float moonRiseHour(unsigned int daysPassed) const;
        float rotation(float hours) const;
        unsigned int phase(const TimeStamp& gameTime) const;
        float shadowBlend(float angle) const;
        float hourlyAlpha(float gameHour) const;
        float earlyMoonShadowAlpha(float angle) const;
    };

    /// Interface for weather settings
    class WeatherManager
    {
    public:
        // Have to pass fallback and Store, can't use singleton since World isn't fully constructed yet at the time
        WeatherManager(MWRender::RenderingManager& rendering,
                       const MWWorld::Fallback& fallback,
                       MWWorld::ESMStore& store);
        ~WeatherManager();

        /**
         * Change the weather in the specified region
         * @param region that should be changed
         * @param ID of the weather setting to shift to
         */
        void changeWeather(const std::string& regionID, const unsigned int weatherID);
        void modRegion(const std::string& regionID, const std::vector<char>& chances);
        void playerTeleported();

        /**
         * Per-frame update
         * @param duration
         * @param paused
         */
        void update(float duration, bool paused = false);

        void stopSounds();

        float getWindSpeed() const;

        /// Are we in an ash or blight storm?
        bool isInStorm() const;

        osg::Vec3f getStormDirection() const;

        void advanceTime(double hours);
        void advanceTimeByFrame(double hours);

        unsigned int getWeatherID() const;

        /// @see World::isDark
        bool isDark() const;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress);

        bool readRecord(ESM::ESMReader& reader, uint32_t type);

        void clear();

    private:
        MWWorld::ESMStore& mStore;
        MWRender::RenderingManager& mRendering;
        float mSunriseTime;
        float mSunsetTime;
        float mSunriseDuration;
        float mSunsetDuration;
        // Some useful values
        /* TODO: Use pre-sunrise_time, pre-sunset_time,
         * post-sunrise_time, and post-sunset_time to better
         * describe sunrise/sunset time.
         * These values are fallbacks attached to weather.
         */
        float mNightStart;
        float mNightEnd;
        float mDayStart;
        float mDayEnd;
        float mHoursBetweenWeatherChanges;
        float mRainSpeed;
        std::vector<Weather> mWeatherSettings;
        MoonModel mMasser;
        MoonModel mSecunda;

        float mThunderFrequency;
        float mThunderThreshold;
        std::string mThunderSoundID0;
        std::string mThunderSoundID1;
        std::string mThunderSoundID2;
        std::string mThunderSoundID3;

        float mWindSpeed;
        bool mIsStorm;
        osg::Vec3f mStormDirection;

        float mThunderSoundDelay;
        float mThunderFlash;
        float mThunderChance;
        float mThunderChanceNeeded;

        std::string mCurrentRegion;
        float mTimePassed;
        bool mFastForward;
        float mWeatherUpdateTime;
        float mTransitionFactor;
        int mCurrentWeather;
        int mNextWeather;
        int mQueuedWeather;
        std::map<std::string, RegionWeather> mRegions;
        MWRender::WeatherResult mResult;

        MWBase::SoundPtr mAmbientSound;
        std::string mPlayingSoundID;

        void addWeather(const std::string& name,
                        const MWWorld::Fallback& fallback,
                        const std::string& ambientLoopSoundID = "",
                        const std::string& particleEffect = "");

        void importRegions();

        void regionalWeatherChanged(const std::string& regionID, RegionWeather& region);
        bool updateWeatherTime();
        bool updateWeatherRegion(const std::string& playerRegion);
        void updateWeatherTransitions(const float elapsedRealSeconds);
        void forceWeather(const int weatherID);

        bool inTransition();
        void addWeatherTransition(const int weatherID);

        void calculateWeatherResult(const float gameHour);
        void calculateResult(const int weatherID, const float gameHour);
        void calculateTransitionResult(const float factor, const float gameHour);
    };
}

#endif // GAME_MWWORLD_WEATHER_H
