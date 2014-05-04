#ifndef GAME_MWWORLD_WEATHER_H
#define GAME_MWWORLD_WEATHER_H

#include <stdint.h>
#include <string>

#include <OgreColourValue.h>

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

    /// Defines the actual weather that results from weather setting (see below), time of day and weather transition
    struct WeatherResult
    {
        std::string mCloudTexture;
        std::string mNextCloudTexture;
        float mCloudBlendFactor;

        Ogre::ColourValue mFogColor;

        Ogre::ColourValue mAmbientColor;

        Ogre::ColourValue mSkyColor;

        Ogre::ColourValue mSunColor;

        Ogre::ColourValue mSunDiscColor;

        float mFogDepth;

        float mWindSpeed;

        float mCloudSpeed;

        float mCloudOpacity;

        float mGlareView;

        bool mNight; // use night skybox
        float mNightFade; // fading factor for night skybox

        std::string mAmbientLoopSoundID;
    };


    /// Defines a single weather setting (according to INI)
    struct Weather
    {
        std::string mCloudTexture;

        // Sky (atmosphere) colors
        Ogre::ColourValue   mSkySunriseColor,
                            mSkyDayColor,
                            mSkySunsetColor,
                            mSkyNightColor;

        // Fog colors
        Ogre::ColourValue   mFogSunriseColor,
                            mFogDayColor,
                            mFogSunsetColor,
                            mFogNightColor;

        // Ambient lighting colors
        Ogre::ColourValue   mAmbientSunriseColor,
                            mAmbientDayColor,
                            mAmbientSunsetColor,
                            mAmbientNightColor;

        // Sun (directional) lighting colors
        Ogre::ColourValue   mSunSunriseColor,
                            mSunDayColor,
                            mSunSunsetColor,
                            mSunNightColor;

        // Fog depth/density
        float   mLandFogDayDepth,
                mLandFogNightDepth;

        // Color modulation for the sun itself during sunset (not completely sure)
        Ogre::ColourValue mSunDiscSunsetColor;

        // Duration of weather transition (in days)
        float mTransitionDelta;

        // No idea what this one is used for?
        float mWindSpeed;

        // Cloud animation speed multiplier
        float mCloudSpeed;

        // Multiplier for clouds transparency
        float mCloudsMaximumPercent;

        // Value between 0 and 1, defines the strength of the sun glare effect
        float mGlareView;

        // Sound effect
        // This is used for Blight, Ashstorm and Blizzard (Bloodmoon)
        std::string mAmbientLoopSoundID;

        // Rain sound effect
        std::string mRainLoopSoundID;

        /// \todo disease chance
    };

    ///
    /// Interface for weather settings
    ///
    class WeatherManager
    {
    public:
        WeatherManager(MWRender::RenderingManager*,MWWorld::Fallback* fallback);
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
         */
        void update(float duration);

        void stopSounds(bool stopAll);

        void setHour(const float hour);

        float getWindSpeed() const;

        void advanceTime(double hours)
        {
            mTimePassed += hours*3600;
        }

        unsigned int getWeatherID() const;

        void modRegion(const std::string &regionid, const std::vector<char> &chances);

        /// @see World::isDark
        bool isDark() const;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress);

        bool readRecord(ESM::ESMReader& reader, int32_t type);

    private:
        float mHour;
        float mWindSpeed;
        MWWorld::Fallback* mFallback;
        void setFallbackWeather(Weather& weather,const std::string& name);
        MWRender::RenderingManager* mRendering;

        std::map<std::string, Weather> mWeatherSettings;

        std::map<std::string, std::string> mRegionOverrides;

        std::vector<std::string> mSoundsPlaying;

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

        float calculateHourFade (const std::string& moonName) const;
        float calculateAngleFade (const std::string& moonName, float angle) const;

        void setWeather(const std::string& weatherType, bool instant=false);
        std::string nextWeather(const ESM::Region* region) const;
        WeatherResult mResult;

        typedef std::map<std::string,std::vector<char> > RegionModMap;
        RegionModMap mRegionMods;

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
    };
}

#endif // GAME_MWWORLD_WEATHER_H
