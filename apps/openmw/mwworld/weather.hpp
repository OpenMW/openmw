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
    /// Defines a single weather setting
    struct Weather
    {
        Ogre::String mCloudTexture;
        
        // Sky (atmosphere) colors 
        Ogre::ColourValue mSkySunriseColor;
        Ogre::ColourValue mSkyDayColor;
        Ogre::ColourValue mSkySunsetColor;
        Ogre::ColourValue mSkyNightColor;
        
        // Fog colors
        Ogre::ColourValue mFogSunriseColor;
        Ogre::ColourValue mFogDayColor;
        Ogre::ColourValue mFogSunsetColor;
        Ogre::ColourValue mFogNightColor;
        
        // Ambient lighting colors
        Ogre::ColourValue mAmbientSunriseColor;
        Ogre::ColourValue mAmbientDayColor;
        Ogre::ColourValue mAmbientSunsetColor;
        Ogre::ColourValue mAmbientNightColor;
        
        // Sun (directional) lighting colors
        Ogre::ColourValue mSunSunriseColor;
        Ogre::ColourValue mSunDayColor;
        Ogre::ColourValue mSunSunsetColor;
        Ogre::ColourValue mSunNightColor;
        
        // Fog depth/density
        float mLandFogDayDepth;
        float mLandFogNightDepth;
        
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
        void setWeather(const Weather& weather, bool instant=false);
        
        /**
         * Per-frame update
         * @param duration
         */
        void update(float duration);
        
    private:
        MWRender::RenderingManager* mRendering;
    };
}

#endif // GAME_MWWORLD_WEATHER_H
