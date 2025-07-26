#ifndef GAME_MWWORLD_WEATHER_H
#define GAME_MWWORLD_WEATHER_H

#include <cstdint>
#include <map>
#include <string>

#include <osg/Vec4f>

#include <components/esm/refid.hpp>
#include <components/fallback/fallback.hpp>

#include "../mwbase/soundmanager.hpp"

#include "../mwrender/skyutil.hpp"

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

namespace Fallback
{
    class Map;
}

namespace MWWorld
{
    class TimeStamp;

    enum NightDayMode
    {
        Default = 0,
        ExteriorNight = 1,
        InteriorDay = 2
    };

    struct WeatherSetting
    {
        float mPreSunriseTime;
        float mPostSunriseTime;
        float mPreSunsetTime;
        float mPostSunsetTime;
    };

    struct TimeOfDaySettings
    {
        float mNightStart;
        float mNightEnd;
        float mDayStart;
        float mDayEnd;

        std::map<std::string, WeatherSetting> mSunriseTransitions;

        float mStarsPostSunsetStart;
        float mStarsPreSunriseFinish;
        float mStarsFadingDuration;

        WeatherSetting getSetting(const std::string& type) const
        {
            std::map<std::string, WeatherSetting>::const_iterator it = mSunriseTransitions.find(type);
            if (it != mSunriseTransitions.end())
            {
                return it->second;
            }
            else
            {
                return { 1.f, 1.f, 1.f, 1.f };
            }
        }

        void addSetting(const std::string& type)
        {
            WeatherSetting setting = { Fallback::Map::getFloat("Weather_" + type + "_Pre-Sunrise_Time"),
                Fallback::Map::getFloat("Weather_" + type + "_Post-Sunrise_Time"),
                Fallback::Map::getFloat("Weather_" + type + "_Pre-Sunset_Time"),
                Fallback::Map::getFloat("Weather_" + type + "_Post-Sunset_Time") };

            mSunriseTransitions[type] = setting;
        }
    };

    /// Interpolates between 4 data points (sunrise, day, sunset, night) based on the time of day.
    /// The template value could be a floating point number, or a color.
    template <typename T>
    class TimeOfDayInterpolator
    {
    public:
        TimeOfDayInterpolator(const T& sunrise, const T& day, const T& sunset, const T& night)
            : mSunriseValue(sunrise)
            , mDayValue(day)
            , mSunsetValue(sunset)
            , mNightValue(night)
        {
        }

        T getValue(const float gameHour, const TimeOfDaySettings& timeSettings, const std::string& prefix) const;

        const T& getSunriseValue() const { return mSunriseValue; }
        const T& getDayValue() const { return mDayValue; }
        const T& getSunsetValue() const { return mSunsetValue; }
        const T& getNightValue() const { return mNightValue; }

    private:
        T mSunriseValue, mDayValue, mSunsetValue, mNightValue;
    };

    /// Defines a single weather setting (according to INI)
    class Weather
    {
    public:
        static osg::Vec3f defaultDirection();

        Weather(const ESM::RefId id, const int scriptId, const std::string& name, float stormWindSpeed, float rainSpeed,
            float dlFactor, float dlOffset, const std::string& particleEffect);

        ESM::RefId mId;
        int mScriptId;
        std::string mName;
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

        // Fog factor and offset used with distant land rendering.
        struct
        {
            float FogFactor;
            float FogOffset;
        } mDL;

        // Sound effects
        // This is used for Blight, Ashstorm and Blizzard (Bloodmoon)
        ESM::RefId mAmbientLoopSoundID;
        // This is used for Rain and Thunderstorm
        ESM::RefId mRainLoopSoundID;

        std::array<ESM::RefId, 4> mThunderSoundID;

        // Is this an ash storm / blight storm? If so, the following will happen:
        // - The particles and clouds will be oriented so they appear to come from the Red Mountain.
        // - Characters will animate their hand to protect eyes from the storm when looking in its direction (idlestorm
        // animation)
        // - Slower movement when walking against the storm (fStromWalkMult)
        bool mIsStorm;

        // How fast does rain travel down?
        // In Morrowind.ini this is set globally, but we may want to change it per weather later.
        float mRainSpeed;

        // How often does a new rain mesh spawn?
        float mRainEntranceSpeed;

        // Maximum count of rain particles
        int mRainMaxRaindrops;

        // Radius of rain effect
        float mRainDiameter;

        // Transition threshold to spawn rain
        float mRainThreshold;

        // Height of rain particles spawn
        float mRainMinHeight;
        float mRainMaxHeight;

        std::string mParticleEffect;

        std::string mRainEffect;

        osg::Vec3f mStormDirection;

        float mCloudsMaximumPercent;

        // Note: For Weather Blight, there is a "Disease Chance" (=0.1) setting. But according to MWSFD this feature
        // is broken in the vanilla game and was disabled.

        float transitionDelta() const;
        float cloudBlendFactor(const float transitionRatio) const;

        float calculateThunder(const float transitionRatio, const float elapsedSeconds, const bool isPaused);

    private:
        float mTransitionDelta;

        // Note: In MW, only thunderstorms support these attributes, but in the interest of making weather more
        // flexible, these settings are imported for all weather types. Only thunderstorms will normally have any
        // non-zero values.
        float mThunderFrequency;
        float mThunderThreshold;
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

        void setChances(const std::vector<uint8_t>& chances);

        void setWeather(int weatherID);

        int getWeather();

    private:
        int mWeather;
        std::vector<uint8_t> mChances;

        void chooseNewWeather();
    };

    /// A class that acts as a model for the moons.
    class MoonModel
    {
    public:
        MoonModel(const std::string& name);
        MoonModel(float fadeInStart, float fadeInFinish, float fadeOutStart, float fadeOutFinish, float axisOffset,
            float speed, float dailyIncrement, float fadeStartAngle, float fadeEndAngle,
            float moonShadowEarlyFadeAngle);

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

        float angle(int gameDay, float gameHour) const;
        float moonPhaseHour(int gameDay) const;
        float moonRiseHour(int gameDay) const;
        float rotation(float hours) const;
        MWRender::MoonState::Phase phase(const TimeStamp& gameTime) const;
        bool isVisible(int gameDay, float gameHour) const;
        float shadowBlend(float angle) const;
        float hourlyAlpha(float gameHour) const;
        float earlyMoonShadowAlpha(float angle) const;
    };

    /// Interface for weather settings
    class WeatherManager
    {
    public:
        // Have to pass fallback and Store, can't use singleton since World isn't fully constructed yet at the time
        WeatherManager(MWRender::RenderingManager& rendering, MWWorld::ESMStore& store);
        ~WeatherManager();

        /**
         * Change the weather in the specified region by id of the weather
         * @param region that should be changed
         * @param ID of the weather setting to shift to
         */
        void changeWeather(const ESM::RefId& regionID, const unsigned int weatherID);
        void changeWeather(const ESM::RefId& regionID, const ESM::RefId& weatherID);
        void modRegion(const ESM::RefId& regionID, const std::vector<uint8_t>& chances);
        void playerTeleported(const ESM::RefId& playerRegion, bool isExterior);

        /**
         * Per-frame update
         * @param duration
         * @param paused
         */
        void update(float duration, bool paused, const TimeStamp& time, bool isExterior);

        void stopSounds();

        float getWindSpeed() const;
        NightDayMode getNightDayMode() const;

        /// Are we in an ash or blight storm?
        bool isInStorm() const;

        osg::Vec3f getStormDirection() const;

        void advanceTime(double hours, bool incremental);

        const std::vector<Weather>& getAllWeather() { return mWeatherSettings; }

        const Weather& getWeather() { return mWeatherSettings[mCurrentWeather]; }

        const Weather* getWeather(size_t index) const;

        const Weather* getWeather(const ESM::RefId& id) const;

        int getWeatherID() const { return mCurrentWeather; }

        const Weather* getNextWeather()
        {
            if (mNextWeather > -1)
                return &mWeatherSettings[mNextWeather];
            return nullptr;
        }

        int getNextWeatherID() const { return mNextWeather; }

        float getTransitionFactor() const { return mTransitionFactor; }

        bool useTorches(float hour) const;

        float getSunPercentage(float hour) const;

        float getSunVisibility() const;

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
        float mCurrentWindSpeed;
        float mNextWindSpeed;
        bool mIsStorm;
        bool mPrecipitation;
        osg::Vec3f mStormDirection;

        ESM::RefId mCurrentRegion;
        float mTimePassed;
        bool mFastForward;
        float mWeatherUpdateTime;
        float mTransitionFactor;
        NightDayMode mNightDayMode;
        int mCurrentWeather;
        int mNextWeather;
        int mQueuedWeather;
        std::map<ESM::RefId, RegionWeather> mRegions;
        MWRender::WeatherResult mResult;

        MWBase::Sound* mAmbientSound{ nullptr };
        ESM::RefId mPlayingAmbientSoundID;
        MWBase::Sound* mRainSound{ nullptr };
        ESM::RefId mPlayingRainSoundID;

        void addWeather(
            const std::string& name, float dlFactor, float dlOffset, const std::string& particleEffect = "");

        void importRegions();

        void regionalWeatherChanged(const ESM::RefId& regionID, RegionWeather& region);
        bool updateWeatherTime();
        bool updateWeatherRegion(const ESM::RefId& playerRegion);
        void updateWeatherTransitions(const float elapsedRealSeconds);
        void forceWeather(const int weatherID);

        bool inTransition() const;
        void addWeatherTransition(const int weatherID);

        void calculateWeatherResult(const float gameHour, const float elapsedSeconds, const bool isPaused);
        void calculateResult(const int weatherID, const float gameHour);
        void calculateTransitionResult(const float factor, const float gameHour);
        float calculateWindSpeed(int weatherId, float currentSpeed);
    };
}

#endif // GAME_MWWORLD_WEATHER_H
