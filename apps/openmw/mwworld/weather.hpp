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


    struct TimeOfDaySettings
    {
        float mNightStart;
        float mNightEnd;
        float mDayStart;
        float mDayEnd;
        float mSunriseTime;
    };

    /// Interpolates between 4 data points (sunrise, day, sunset, night) based on the time of day.
    /// The template value could be a floating point number, or a color.
    template <typename T>
    class TimeOfDayInterpolator
    {
    public:
        TimeOfDayInterpolator(const T& sunrise, const T& day, const T& sunset, const T& night)
            : mSunriseValue(sunrise), mDayValue(day), mSunsetValue(sunset), mNightValue(night)
        {
        }

        T getValue (const float gameHour, const TimeOfDaySettings& timeSettings) const;

    private:
        T mSunriseValue, mDayValue, mSunsetValue, mNightValue;
    };

    /// Defines a single weather setting (according to INI)
    class Weather
    {
    public:
        Weather(const std::string& name,
                const MWWorld::Fallback& fallback,
                float stormWindSpeed,
                float rainSpeed,
                const std::string& particleEffect);

        std::string mCloudTexture;

        // Sky (atmosphere) color
        TimeOfDayInterpolator<osg::Vec4f> mSkyColor;
        // Fog color
        TimeOfDayInterpolator<osg::Vec4f> mFogColor;
        // Ambient lighting color
        TimeOfDayInterpolator<osg::Vec4f> mAmbientColor;
        // Sun (directional) lighting color
        TimeOfDayInterpolator<osg::Vec4f> mSunColor;

        // Fog depth/density
        TimeOfDayInterpolator<float> mLandFogDepth;

        // Color modulation for the sun itself during sunset
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
        float cloudBlendFactor(const float transitionRatio) const;

        float calculateThunder(const float transitionRatio, const float elapsedSeconds, const bool isPaused);

    private:
        float mTransitionDelta;
        float mCloudsMaximumPercent;

        // Note: In MW, only thunderstorms support these attributes, but in the interest of making weather more
        // flexible, these settings are imported for all weather types. Only thunderstorms will normally have any
        // non-zero values.
        float mThunderFrequency;
        float mThunderThreshold;
        std::string mThunderSoundID[4];
        float mFlashDecrement;

        float mFlashBrightness;

        void flashDecrement(const float elapsedSeconds);
        float thunderChance(const float transitionRatio, const float elapsedSeconds) const;
        void lightningAndThunder(void);
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

        void advanceTime(double hours, bool incremental);

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
        float mSunPreSunsetTime;

        TimeOfDaySettings mTimeSettings;

        // fading of night skydome
        TimeOfDayInterpolator<float> mNightFade;

        float mHoursBetweenWeatherChanges;
        float mRainSpeed;

        // underwater fog not really related to weather, but we handle it here because it's convenient
        TimeOfDayInterpolator<float> mUnderwaterFog;

        std::vector<Weather> mWeatherSettings;
        MoonModel mMasser;
        MoonModel mSecunda;

        float mWindSpeed;
        bool mIsStorm;
        osg::Vec3f mStormDirection;

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
                        const std::string& particleEffect = "");

        void importRegions();

        void regionalWeatherChanged(const std::string& regionID, RegionWeather& region);
        bool updateWeatherTime();
        bool updateWeatherRegion(const std::string& playerRegion);
        void updateWeatherTransitions(const float elapsedRealSeconds);
        void forceWeather(const int weatherID);

        bool inTransition();
        void addWeatherTransition(const int weatherID);

        void calculateWeatherResult(const float gameHour, const float elapsedSeconds, const bool isPaused);
        void calculateResult(const int weatherID, const float gameHour);
        void calculateTransitionResult(const float factor, const float gameHour);
    };
}

#endif // GAME_MWWORLD_WEATHER_H
