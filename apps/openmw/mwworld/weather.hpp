#ifndef GAME_MWWORLD_WEATHER_H
#define GAME_MWWORLD_WEATHER_H

#include <OgreString.h>
#include <OgreColourValue.h>

namespace MWRender
{
    class RenderingManager;
}

namespace MWWorld
{
    class Fallback;

    /// Defines the actual weather that results from weather setting (see below), time of day and weather transition
    struct WeatherResult
    {
        Ogre::String mCloudTexture;
        Ogre::String mNextCloudTexture;
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

        Ogre::String mAmbientLoopSoundID;
    };


    /// Defines a single weather setting (according to INI)
    struct Weather
    {
        Ogre::String mCloudTexture;

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
        Ogre::String mAmbientLoopSoundID;

        // Rain sound effect
        Ogre::String mRainLoopSoundID;

        /// \todo disease chance
    };

    ///
    /// Interface for weather settings
    ///
    class WeatherManager
    {
    public:
        WeatherManager(MWRender::RenderingManager*,MWWorld::Fallback* fallback);

        /**
         * Change the weather in the specified region
         * @param region that should be changed
         * @param ID of the weather setting to shift to
         */
        void changeWeather(const std::string& region, const unsigned int id);

        /**
         * Per-frame update
         * @param duration
         */
        void update(float duration);

        void setHour(const float hour);

        void setDate(const int day, const int month);

        void advanceTime(double hours)
        {
            mTimePassed += hours*3600;
        }

        unsigned int getWeatherID() const;

    private:
        float mHour;
        int mDay, mMonth;
        MWWorld::Fallback* mFallback;
        void setFallbackWeather(Weather& weather,const std::string& name);
        MWRender::RenderingManager* mRendering;

        std::map<Ogre::String, Weather> mWeatherSettings;

        std::map<std::string, std::string> mRegionOverrides;

        std::vector<std::string> mSoundsPlaying;

        Ogre::String mCurrentWeather;
        Ogre::String mNextWeather;

        std::string mCurrentRegion;

        bool mFirstUpdate;

        float mRemainingTransitionTime;

        float mThunderFlash;
        float mThunderChance;
        float mThunderChanceNeeded;

        double mTimePassed; // time passed since last update

        WeatherResult transition(const float factor);
        WeatherResult getResult(const Ogre::String& weather);

        void setWeather(const Ogre::String& weather, bool instant=false);
        float mSunriseTime;
        float mSunsetTime;
        float mSunriseDuration;
        float mSunsetDuration;
        float mWeatherUpdateTime;
        float mHoursBetweenWeatherChanges;
        float mThunderFrequency;
        float mThunderThreshold;
        float mThunderSoundDelay;
        std::string mThunderSoundID0;
        std::string mThunderSoundID1;
        std::string mThunderSoundID2;
        std::string mThunderSoundID3;
    };
}

#endif // GAME_MWWORLD_WEATHER_H
