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
    /// Global weather manager properties (according to INI)
    struct WeatherGlobals
    {
        /*
        EnvReduceColor=255,255,255,255
        LerpCloseColor=037,046,048,255
        BumpFadeColor=230,239,255,255
        AlphaReduce=0.35
        Minimum Time Between Environmental Sounds=1.0
        Maximum Time Between Environmental Sounds=5.0
        Sun Glare Fader Max=0.5
        Sun Glare Fader Angle Max=30.0
        Sun Glare Fader Color=222,095,039
        Timescale Clouds=0
        Precip Gravity=575
        Hours Between Weather Changes=20
        Rain Ripples=1
        Rain Ripple Radius=1024
        Rain Ripples Per Drop=1
        Rain Ripple Scale=0.3
        Rain Ripple Speed=1.0
        Fog Depth Change Speed=3
        Sunrise Time=6
        Sunset Time=18
        Sunrise Duration=2
        Sunset Duration=2
        Sky Pre-Sunrise Time=.5
        Sky Post-Sunrise Time=1
        Sky Pre-Sunset Time=1.5
        Sky Post-Sunset Time=.5
        Ambient Pre-Sunrise Time=.5
        Ambient Post-Sunrise Time=2
        Ambient Pre-Sunset Time=1
        Ambient Post-Sunset Time=1.25
        Fog Pre-Sunrise Time=.5
        Fog Post-Sunrise Time=1
        Fog Pre-Sunset Time=2
        Fog Post-Sunset Time=1
        Sun Pre-Sunrise Time=0
        Sun Post-Sunrise Time=0
        Sun Pre-Sunset Time=1
        Sun Post-Sunset Time=1.25
        Stars Post-Sunset Start=1
        Stars Pre-Sunrise Finish=2
        Stars Fading Duration=2
        Snow Ripples=0
        Snow Ripple Radius=1024
        Snow Ripples Per Flake=1
        Snow Ripple Scale=0.3
        Snow Ripple Speed=1.0
        Snow Gravity Scale=0.1
        Snow High Kill=700
        Snow Low Kill=150
        */
        
        float   mSunriseTime,
                mSunsetTime,
                mSunriseDuration,
                mSunsetDuration;
        
        WeatherGlobals() :
            mSunriseTime(8),
            mSunsetTime(18),
            mSunriseDuration(2),
            mSunsetDuration(2)
        {};
    };
    
    /// Defines the actual weather that results from weather setting (see below), time of day and weather transition
    struct WeatherResult
    {
        Ogre::String mCloudTexture;
        Ogre::String mNextCloudTexture;
        float mCloudBlendFactor;
                
        Ogre::ColourValue mFogColor;
        
        Ogre::ColourValue mAmbientColor;
        
        Ogre::ColourValue mSunColor;
        
        Ogre::ColourValue mSunDiscColor;
        
        float mFogDepth;
        
        float mWindSpeed;
        
        float mCloudSpeed;
        
        float mCloudOpacity;
        
        float mGlareView;
        
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
        Ogre::ColourValue mSunDiscSunsetColour;
        
        // Duration of weather transition
        // the INI value is 0.015, so I suppose this is measured in Morrowind-days? (0.015 days = 36 minutes in Morrowind)
        float mTransitionDelta;
        
        // No idea what this one is used for?
        float mWindSpeed;
        
        // Cloud animation speed multiplier
        float mCloudSpeed;
        
        // Multiplier for clouds transparency?
        float mCloudsMaximumPercent;
        
        // Value between 0 and 1, defines the strength of the sun glare effect
        float mGlareView;
        
        // Sound effect
        // This is used for Blight, Ashstorm and Blizzard (Bloodmoon)
        Ogre::String mAmbientLoopSoundID;
        
        /// \todo rain, thunder, ashstorm...
        
        /// \todo disease chance
    };
    
    ///
    /// Interface for weather settings
    ///
    class WeatherManager
    {
    public:
        WeatherManager(MWRender::RenderingManager*);
        
        /**
         * Change the weather setting
         * @param weather
         *      new weather setting to use
         * @param instant
         *      if true, the weather changes instantly. if false, it slowly starts transitioning.
         */
        void setWeather(const Ogre::String& weather, bool instant=false);
        
        /**
         * Per-frame update
         * @param duration
         */
        void update(float duration);
        
        void setHour(const float hour);
        
        void setDate(const int day, const int month);
        
    private:
        float mHour;
        int mDay, mMonth;
    
        MWRender::RenderingManager* mRendering;
        
        std::map<Ogre::String, Weather> mWeatherSettings;
        
        Ogre::String mCurrentWeather;
        Ogre::String mNextWeather;
        
        float mRemainingTransitionTime;
        
        WeatherResult transition(const float factor);
        WeatherResult getResult(const Ogre::String& weather);
    };
}

#endif // GAME_MWWORLD_WEATHER_H
