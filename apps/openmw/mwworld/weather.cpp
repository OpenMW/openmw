#define _USE_MATH_DEFINES
#include <cmath>

#include "weather.hpp"

#include <openengine/misc/rng.hpp>

#include <components/esm/weatherstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwsound/sound.hpp"

#include "../mwrender/renderingmanager.hpp"

#include "player.hpp"
#include "esmstore.hpp"
#include "fallback.hpp"
#include "cellstore.hpp"

using namespace Ogre;
using namespace MWWorld;
using namespace MWSound;

namespace
{
    float lerp (float x, float y, float factor)
    {
        return x * (1-factor) + y * factor;
    }

    Ogre::ColourValue lerp (const Ogre::ColourValue& x, const Ogre::ColourValue& y, float factor)
    {
        return x * (1-factor) + y * factor;
    }
}

void WeatherManager::setFallbackWeather(Weather& weather,const std::string& name)
{
    std::string upper=name;
    upper[0]=toupper(name[0]);
    weather.mCloudsMaximumPercent = mFallback->getFallbackFloat("Weather_"+upper+"_Clouds_Maximum_Percent");
    weather.mTransitionDelta = mFallback->getFallbackFloat("Weather_"+upper+"_Transition_Delta");
    weather.mSkySunriseColor=mFallback->getFallbackColour("Weather_"+upper+"_Sky_Sunrise_Color");
    weather.mSkyDayColor = mFallback->getFallbackColour("Weather_"+upper+"_Sky_Day_Color");
    weather.mSkySunsetColor = mFallback->getFallbackColour("Weather_"+upper+"_Sky_Sunset_Color");
    weather.mSkyNightColor = mFallback->getFallbackColour("Weather_"+upper+"_Sky_Night_Color");
    weather.mFogSunriseColor = mFallback->getFallbackColour("Weather_"+upper+"_Fog_Sunrise_Color");
    weather.mFogDayColor = mFallback->getFallbackColour("Weather_"+upper+"_Fog_Day_Color");
    weather.mFogSunsetColor = mFallback->getFallbackColour("Weather_"+upper+"_Fog_Sunset_Color");
    weather.mFogNightColor = mFallback->getFallbackColour("Weather_"+upper+"_Fog_Night_Color");
    weather.mAmbientSunriseColor = mFallback->getFallbackColour("Weather_"+upper+"_Ambient_Sunrise_Color");
    weather.mAmbientDayColor = mFallback->getFallbackColour("Weather_"+upper+"_Ambient_Day_Color");
    weather.mAmbientSunsetColor = mFallback->getFallbackColour("Weather_"+upper+"_Ambient_Sunset_Color");
    weather.mAmbientNightColor = mFallback->getFallbackColour("Weather_"+upper+"_Ambient_Night_Color");
    weather.mSunSunriseColor = mFallback->getFallbackColour("Weather_"+upper+"_Sun_Sunrise_Color");
    weather.mSunDayColor = mFallback->getFallbackColour("Weather_"+upper+"_Sun_Day_Color");
    weather.mSunSunsetColor = mFallback->getFallbackColour("Weather_"+upper+"_Sun_Sunset_Color");
    weather.mSunNightColor = mFallback->getFallbackColour("Weather_"+upper+"_Sun_Night_Color");
    weather.mSunDiscSunsetColor = mFallback->getFallbackColour("Weather_"+upper+"_Sun_Disc_Sunset_Color");
    weather.mLandFogDayDepth = mFallback->getFallbackFloat("Weather_"+upper+"_Land_Fog_Day_Depth");
    weather.mLandFogNightDepth = mFallback->getFallbackFloat("Weather_"+upper+"_Land_Fog_Night_Depth");
    weather.mWindSpeed = mFallback->getFallbackFloat("Weather_"+upper+"_Wind_Speed");
    weather.mCloudSpeed = mFallback->getFallbackFloat("Weather_"+upper+"_Cloud_Speed");
    weather.mGlareView = mFallback->getFallbackFloat("Weather_"+upper+"_Glare_View");
    weather.mCloudTexture = mFallback->getFallbackString("Weather_"+upper+"_Cloud_Texture");

    bool usesPrecip = mFallback->getFallbackBool("Weather_"+upper+"_Using_Precip");
    if (usesPrecip)
        weather.mRainEffect = "meshes\\raindrop.nif";
    weather.mRainSpeed = mRainSpeed;
    weather.mRainFrequency = mFallback->getFallbackFloat("Weather_"+upper+"_Rain_Entrance_Speed");
    /*
Unhandled:
Rain Diameter=600 ?
Rain Height Min=200 ?
Rain Height Max=700 ?
Rain Threshold=0.6 ?
Max Raindrops=650 ?
*/
    weather.mIsStorm = (name == "ashstorm" || name == "blight");

    mWeatherSettings[name] = weather;
}


float WeatherManager::calculateHourFade (const std::string& moonName) const
{
    float fadeOutStart=mFallback->getFallbackFloat("Moons_"+moonName+"_Fade_Out_Start");
    float fadeOutFinish=mFallback->getFallbackFloat("Moons_"+moonName+"_Fade_Out_Finish");
    float fadeInStart=mFallback->getFallbackFloat("Moons_"+moonName+"_Fade_In_Start");
    float fadeInFinish=mFallback->getFallbackFloat("Moons_"+moonName+"_Fade_In_Finish");

    if (mHour >= fadeOutStart && mHour <= fadeOutFinish)
        return (1 - ((mHour - fadeOutStart) / (fadeOutFinish - fadeOutStart)));
    if (mHour >= fadeInStart && mHour <= fadeInFinish)
        return (1 - ((mHour - fadeInStart) / (fadeInFinish - fadeInStart)));
    else
        return 1;
}

float WeatherManager::calculateAngleFade (const std::string& moonName, float angle) const
{
    float endAngle=mFallback->getFallbackFloat("Moons_"+moonName+"_Fade_End_Angle");
    float startAngle=mFallback->getFallbackFloat("Moons_"+moonName+"_Fade_Start_Angle");
    if (angle <= startAngle && angle >= endAngle)
        return (1 - ((angle - endAngle)/(startAngle-endAngle)));
    else if (angle > startAngle)
        return 0.f;
    else
        return 1.f;
}

WeatherManager::WeatherManager(MWRender::RenderingManager* rendering,MWWorld::Fallback* fallback) :
     mHour(14), mCurrentWeather("clear"), mNextWeather(""), mFirstUpdate(true),
     mWeatherUpdateTime(0), mThunderFlash(0), mThunderChance(0),
     mThunderChanceNeeded(50), mThunderSoundDelay(0), mRemainingTransitionTime(0),
     mTimePassed(0), mFallback(fallback), mWindSpeed(0.f), mRendering(rendering), mIsStorm(false),
     mStormDirection(0,1,0)
{
    //Globals
    mThunderSoundID0 = mFallback->getFallbackString("Weather_Thunderstorm_Thunder_Sound_ID_0");
    mThunderSoundID1 = mFallback->getFallbackString("Weather_Thunderstorm_Thunder_Sound_ID_1");
    mThunderSoundID2 = mFallback->getFallbackString("Weather_Thunderstorm_Thunder_Sound_ID_2");
    mThunderSoundID3 = mFallback->getFallbackString("Weather_Thunderstorm_Thunder_Sound_ID_3");
    mSunriseTime = mFallback->getFallbackFloat("Weather_Sunrise_Time");
    mSunsetTime = mFallback->getFallbackFloat("Weather_Sunset_Time");
    mSunriseDuration = mFallback->getFallbackFloat("Weather_Sunrise_Duration");
    mSunsetDuration = mFallback->getFallbackFloat("Weather_Sunset_Duration");
    mHoursBetweenWeatherChanges = mFallback->getFallbackFloat("Weather_Hours_Between_Weather_Changes");
    mWeatherUpdateTime = mHoursBetweenWeatherChanges * 3600;
    mThunderFrequency = mFallback->getFallbackFloat("Weather_Thunderstorm_Thunder_Frequency");
    mThunderThreshold = mFallback->getFallbackFloat("Weather_Thunderstorm_Thunder_Threshold");
    mThunderSoundDelay = 0.25;

    mRainSpeed = mFallback->getFallbackFloat("Weather_Precip_Gravity");

    //Some useful values
    /* TODO: Use pre-sunrise_time, pre-sunset_time,
     * post-sunrise_time, and post-sunset_time to better
     * describe sunrise/sunset time.
     * These values are fallbacks attached to weather.
     */
    mNightStart = mSunsetTime + mSunsetDuration;
    mNightEnd = mSunriseTime - 0.5f;
    mDayStart = mSunriseTime + mSunriseDuration;
    mDayEnd = mSunsetTime;

    //Weather
    Weather clear;
    setFallbackWeather(clear,"clear");

    Weather cloudy;
    setFallbackWeather(cloudy,"cloudy");

    Weather foggy;
    setFallbackWeather(foggy,"foggy");

    Weather thunderstorm;
    thunderstorm.mAmbientLoopSoundID = "rain heavy";
    thunderstorm.mRainEffect = "meshes\\raindrop.nif";
    setFallbackWeather(thunderstorm,"thunderstorm");

    Weather rain;
    rain.mAmbientLoopSoundID = "rain";
    rain.mRainEffect = "meshes\\raindrop.nif";
    setFallbackWeather(rain,"rain");

    Weather overcast;
    setFallbackWeather(overcast,"overcast");

    Weather ashstorm;
    ashstorm.mAmbientLoopSoundID = "ashstorm";
    ashstorm.mParticleEffect = "meshes\\ashcloud.nif";
    setFallbackWeather(ashstorm,"ashstorm");

    Weather blight;
    blight.mAmbientLoopSoundID = "blight";
    blight.mParticleEffect = "meshes\\blightcloud.nif";
    setFallbackWeather(blight,"blight");

    Weather snow;
    snow.mParticleEffect = "meshes\\snow.nif";
    setFallbackWeather(snow, "snow");

    Weather blizzard;
    blizzard.mAmbientLoopSoundID = "BM Blizzard";
    blizzard.mParticleEffect = "meshes\\blizzard.nif";
    setFallbackWeather(blizzard,"blizzard");
}

WeatherManager::~WeatherManager()
{
    stopSounds();
}

void WeatherManager::setWeather(const String& weather, bool instant)
{
    if (weather == mCurrentWeather && mNextWeather == "")
    {
        mFirstUpdate = false;
        return;
    }

    if (instant || mFirstUpdate)
    {
        mNextWeather = "";
        mCurrentWeather = weather;
    }
    else
    {
        if (mNextWeather != "")
        {
            // transition more than 50% finished?
            if (mRemainingTransitionTime/(mWeatherSettings[mCurrentWeather].mTransitionDelta * 24.f * 3600) <= 0.5)
                mCurrentWeather = mNextWeather;
        }

        mNextWeather = weather;
        mRemainingTransitionTime = mWeatherSettings[mCurrentWeather].mTransitionDelta * 24.f * 3600.f;
    }
    mFirstUpdate = false;
}

void WeatherManager::setResult(const String& weatherType)
{
    const Weather& current = mWeatherSettings[weatherType];

    mResult.mCloudTexture = current.mCloudTexture;
    mResult.mCloudBlendFactor = 0;
    mResult.mCloudOpacity = current.mCloudsMaximumPercent;
    mResult.mWindSpeed = current.mWindSpeed;
    mResult.mCloudSpeed = current.mCloudSpeed;
    mResult.mGlareView = current.mGlareView;
    mResult.mAmbientLoopSoundID = current.mAmbientLoopSoundID;
    mResult.mAmbientSoundVolume = 1.f;
    mResult.mEffectFade = 1.f;
    mResult.mSunColor = current.mSunDiscSunsetColor;

    mResult.mIsStorm = current.mIsStorm;

    mResult.mRainSpeed = current.mRainSpeed;
    mResult.mRainFrequency = current.mRainFrequency;

    mResult.mParticleEffect = current.mParticleEffect;
    mResult.mRainEffect = current.mRainEffect;

    mResult.mNight = (mHour < mSunriseTime || mHour > mNightStart - 1);

    mResult.mFogDepth = mResult.mNight ? current.mLandFogNightDepth : current.mLandFogDayDepth;

    // night
    if (mHour <= mNightEnd || mHour >= mNightStart + 1)
    {
        mResult.mFogColor = current.mFogNightColor;
        mResult.mAmbientColor = current.mAmbientNightColor;
        mResult.mSunColor = current.mSunNightColor;
        mResult.mSkyColor = current.mSkyNightColor;
        mResult.mNightFade = 1.f;
    }

    // sunrise
    else if (mHour >= mNightEnd && mHour <= mDayStart + 1)
    {
        if (mHour <= mSunriseTime)
        {
            // fade in
            float advance = mSunriseTime - mHour;
            float factor = advance / 0.5f;
            mResult.mFogColor = lerp(current.mFogSunriseColor, current.mFogNightColor, factor);
            mResult.mAmbientColor = lerp(current.mAmbientSunriseColor, current.mAmbientNightColor, factor);
            mResult.mSunColor = lerp(current.mSunSunriseColor, current.mSunNightColor, factor);
            mResult.mSkyColor = lerp(current.mSkySunriseColor, current.mSkyNightColor, factor);
            mResult.mNightFade = factor;
        }
        else //if (mHour >= 6)
        {
            // fade out
            float advance = mHour - mSunriseTime;
            float factor = advance / 3.f;
            mResult.mFogColor = lerp(current.mFogSunriseColor, current.mFogDayColor, factor);
            mResult.mAmbientColor = lerp(current.mAmbientSunriseColor, current.mAmbientDayColor, factor);
            mResult.mSunColor = lerp(current.mSunSunriseColor, current.mSunDayColor, factor);
            mResult.mSkyColor = lerp(current.mSkySunriseColor, current.mSkyDayColor, factor);
        }
    }

    // day
    else if (mHour >= mDayStart + 1 && mHour <= mDayEnd - 1)
    {
        mResult.mFogColor = current.mFogDayColor;
        mResult.mAmbientColor = current.mAmbientDayColor;
        mResult.mSunColor = current.mSunDayColor;
        mResult.mSkyColor = current.mSkyDayColor;
    }

    // sunset
    else if (mHour >= mDayEnd - 1 && mHour <= mNightStart + 1)
    {
        if (mHour <= mDayEnd + 1)
        {
            // fade in
            float advance = (mDayEnd + 1) - mHour;
            float factor = (advance / 2);
            mResult.mFogColor = lerp(current.mFogSunsetColor, current.mFogDayColor, factor);
            mResult.mAmbientColor = lerp(current.mAmbientSunsetColor, current.mAmbientDayColor, factor);
            mResult.mSunColor = lerp(current.mSunSunsetColor, current.mSunDayColor, factor);
            mResult.mSkyColor = lerp(current.mSkySunsetColor, current.mSkyDayColor, factor);
        }
        else //if (mHour >= 19)
        {
            // fade out
            float advance = mHour - (mDayEnd + 1);
            float factor = advance / 2.f;
            mResult.mFogColor = lerp(current.mFogSunsetColor, current.mFogNightColor, factor);
            mResult.mAmbientColor = lerp(current.mAmbientSunsetColor, current.mAmbientNightColor, factor);
            mResult.mSunColor = lerp(current.mSunSunsetColor, current.mSunNightColor, factor);
            mResult.mSkyColor = lerp(current.mSkySunsetColor, current.mSkyNightColor, factor);
            mResult.mNightFade = factor;
        }
    }
}

void WeatherManager::transition(float factor)
{
    setResult(mCurrentWeather);
    const WeatherResult current = mResult;
    setResult(mNextWeather);
    const WeatherResult other = mResult;

    mResult.mCloudTexture = current.mCloudTexture;
    mResult.mNextCloudTexture = other.mCloudTexture;
    mResult.mCloudBlendFactor = factor;

    mResult.mCloudOpacity = lerp(current.mCloudOpacity, other.mCloudOpacity, factor);
    mResult.mFogColor = lerp(current.mFogColor, other.mFogColor, factor);
    mResult.mSunColor = lerp(current.mSunColor, other.mSunColor, factor);
    mResult.mSkyColor = lerp(current.mSkyColor, other.mSkyColor, factor);

    mResult.mAmbientColor = lerp(current.mAmbientColor, other.mAmbientColor, factor);
    mResult.mSunDiscColor = lerp(current.mSunDiscColor, other.mSunDiscColor, factor);
    mResult.mFogDepth = lerp(current.mFogDepth, other.mFogDepth, factor);
    mResult.mWindSpeed = lerp(current.mWindSpeed, other.mWindSpeed, factor);
    mResult.mCloudSpeed = lerp(current.mCloudSpeed, other.mCloudSpeed, factor);
    mResult.mCloudOpacity = lerp(current.mCloudOpacity, other.mCloudOpacity, factor);
    mResult.mGlareView = lerp(current.mGlareView, other.mGlareView, factor);
    mResult.mNightFade = lerp(current.mNightFade, other.mNightFade, factor);

    mResult.mNight = current.mNight;

    if (factor < 0.5)
    {
        mResult.mIsStorm = current.mIsStorm;
        mResult.mParticleEffect = current.mParticleEffect;
        mResult.mRainEffect = current.mRainEffect;
        mResult.mParticleEffect = current.mParticleEffect;
        mResult.mRainSpeed = current.mRainSpeed;
        mResult.mRainFrequency = current.mRainFrequency;
        mResult.mAmbientSoundVolume = 1-(factor*2);
        mResult.mEffectFade = mResult.mAmbientSoundVolume;
        mResult.mAmbientLoopSoundID = current.mAmbientLoopSoundID;
    }
    else
    {
        mResult.mIsStorm = other.mIsStorm;
        mResult.mParticleEffect = other.mParticleEffect;
        mResult.mRainEffect = other.mRainEffect;
        mResult.mParticleEffect = other.mParticleEffect;
        mResult.mRainSpeed = other.mRainSpeed;
        mResult.mRainFrequency = other.mRainFrequency;
        mResult.mAmbientSoundVolume = 2*(factor-0.5f);
        mResult.mEffectFade = mResult.mAmbientSoundVolume;
        mResult.mAmbientLoopSoundID = other.mAmbientLoopSoundID;
    }
}

void WeatherManager::update(float duration, bool paused)
{
    float timePassed = static_cast<float>(mTimePassed);
    mTimePassed = 0;

    mWeatherUpdateTime -= timePassed;

    MWBase::World* world = MWBase::Environment::get().getWorld();
    const bool exterior = (world->isCellExterior() || world->isCellQuasiExterior());
    if (!exterior)
    {
        mRendering->skyDisable();
        mRendering->getSkyManager()->setLightningStrength(0.f);
        stopSounds();
        return;
    }

    switchToNextWeather(false);

    if (mNextWeather != "")
    {
        mRemainingTransitionTime -= timePassed;
        if (mRemainingTransitionTime < 0)
        {
            mCurrentWeather = mNextWeather;
            mNextWeather = "";
        }
    }

    if (mNextWeather != "")
        transition(1 - (mRemainingTransitionTime / (mWeatherSettings[mCurrentWeather].mTransitionDelta * 24.f * 3600)));
    else
        setResult(mCurrentWeather);

    mWindSpeed = mResult.mWindSpeed;
    mIsStorm = mResult.mIsStorm;

    if (mIsStorm)
    {
        MWWorld::Ptr player = world->getPlayerPtr();
        Ogre::Vector3 playerPos (player.getRefData().getPosition().pos);
        Ogre::Vector3 redMountainPos (19950, 72032, 27831);

        mStormDirection = (playerPos - redMountainPos);
        mStormDirection.z = 0;
        mRendering->getSkyManager()->setStormDirection(mStormDirection);
    }

    mRendering->configureFog(mResult.mFogDepth, mResult.mFogColor);

    // disable sun during night
    if (mHour >= mNightStart || mHour <= mSunriseTime)
        mRendering->getSkyManager()->sunDisable();
    else
        mRendering->getSkyManager()->sunEnable();

    // Update the sun direction.  Run it east to west at a fixed angle from overhead.
    // The sun's speed at day and night may differ, since mSunriseTime and mNightStart
    // mark when the sun is level with the horizon.
    {
        // Shift times into a 24-hour window beginning at mSunriseTime...
        float adjustedHour = mHour;
        float adjustedNightStart = mNightStart;
        if ( mHour < mSunriseTime )
            adjustedHour += 24.f;
        if ( mNightStart < mSunriseTime )
            adjustedNightStart += 24.f;

        const bool is_night = adjustedHour >= adjustedNightStart;
        const float dayDuration = adjustedNightStart - mSunriseTime;
        const float nightDuration = 24.f - dayDuration;

        double theta;
        if ( !is_night ) {
            theta = M_PI * (adjustedHour - mSunriseTime) / dayDuration;
        } else {
            theta = M_PI * (adjustedHour - adjustedNightStart) / nightDuration;
        }

        Vector3 final(
            static_cast<float>(cos(theta)),
            -0.268f, // approx tan( -15 degrees )
            static_cast<float>(sin(theta)));
        mRendering->setSunDirection( final, is_night );
    }

    /*
     * TODO: import separated fadeInStart/Finish, fadeOutStart/Finish
     * for masser and secunda
     */

    float fadeOutFinish=mFallback->getFallbackFloat("Moons_Masser_Fade_Out_Finish");
    float fadeInStart=mFallback->getFallbackFloat("Moons_Masser_Fade_In_Start");

    //moon calculations
    float moonHeight;
    if (mHour >= fadeInStart)
        moonHeight = mHour - fadeInStart;
    else if (mHour <= fadeOutFinish)
        moonHeight = mHour + fadeOutFinish;
    else
        moonHeight = 0;

    moonHeight /= (24.f - (fadeInStart - fadeOutFinish));

    if (moonHeight != 0)
    {
        int facing = (moonHeight <= 1) ? 1 : -1;
        Vector3 masser(
                (moonHeight - 1) * facing,
                (1 - moonHeight) * facing,
                moonHeight);

        Vector3 secunda(
                (moonHeight - 1) * facing * 1.25f,
                (1 - moonHeight) * facing * 0.8f,
                moonHeight);

        mRendering->getSkyManager()->setMasserDirection(masser);
        mRendering->getSkyManager()->setSecundaDirection(secunda);
        mRendering->getSkyManager()->masserEnable();
        mRendering->getSkyManager()->secundaEnable();

        float angle = (1-moonHeight) * 90.f * facing;
        float masserHourFade = calculateHourFade("Masser");
        float secundaHourFade = calculateHourFade("Secunda");
        float masserAngleFade = calculateAngleFade("Masser", angle);
        float secundaAngleFade = calculateAngleFade("Secunda", angle);

        masserAngleFade *= masserHourFade;
        secundaAngleFade *= secundaHourFade;

        mRendering->getSkyManager()->setMasserFade(masserAngleFade);
        mRendering->getSkyManager()->setSecundaFade(secundaAngleFade);
    }
    else
    {
        mRendering->getSkyManager()->masserDisable();
        mRendering->getSkyManager()->secundaDisable();
    }

    if (!paused)
    {
        if (mCurrentWeather == "thunderstorm" && mNextWeather == "")
        {
            if (mThunderFlash > 0)
            {
                // play the sound after a delay
                mThunderSoundDelay -= duration;
                if (mThunderSoundDelay <= 0)
                {
                    // pick a random sound
                    int sound = OEngine::Misc::Rng::rollDice(4);
                    std::string* soundName = NULL;
                    if (sound == 0) soundName = &mThunderSoundID0;
                    else if (sound == 1) soundName = &mThunderSoundID1;
                    else if (sound == 2) soundName = &mThunderSoundID2;
                    else if (sound == 3) soundName = &mThunderSoundID3;
                    if (soundName)
                        MWBase::Environment::get().getSoundManager()->playSound(*soundName, 1.0, 1.0);
                    mThunderSoundDelay = 1000;
                }

                mThunderFlash -= duration;
                if (mThunderFlash > 0)
                    mRendering->getSkyManager()->setLightningStrength( mThunderFlash / mThunderThreshold );
                else
                {
                    mThunderChanceNeeded = static_cast<float>(OEngine::Misc::Rng::rollDice(100));
                    mThunderChance = 0;
                    mRendering->getSkyManager()->setLightningStrength( 0.f );
                }
            }
            else
            {
                // no thunder active
                mThunderChance += duration*4; // chance increases by 4 percent every second
                if (mThunderChance >= mThunderChanceNeeded)
                {
                    mThunderFlash = mThunderThreshold;

                    mRendering->getSkyManager()->setLightningStrength( mThunderFlash / mThunderThreshold );

                    mThunderSoundDelay = 0.25;
                }
            }
        }
        else
            mRendering->getSkyManager()->setLightningStrength(0.f);
    }
    

    mRendering->setAmbientColour(mResult.mAmbientColor);
    mRendering->sunEnable(false);
    mRendering->setSunColour(mResult.mSunColor);

    mRendering->getSkyManager()->setWeather(mResult);

    // Play sounds
    if (mPlayingSoundID != mResult.mAmbientLoopSoundID)
    {
        stopSounds();
        if (!mResult.mAmbientLoopSoundID.empty())
            mAmbientSound = MWBase::Environment::get().getSoundManager()->playSound(mResult.mAmbientLoopSoundID, 1.0, 1.0, MWBase::SoundManager::Play_TypeSfx, MWBase::SoundManager::Play_Loop);

        mPlayingSoundID = mResult.mAmbientLoopSoundID;
    }
    if (mAmbientSound.get())
        mAmbientSound->setVolume(mResult.mAmbientSoundVolume);
}

void WeatherManager::stopSounds()
{
    if (mAmbientSound.get())
    {
        MWBase::Environment::get().getSoundManager()->stopSound(mAmbientSound);
        mAmbientSound.reset();
        mPlayingSoundID.clear();
    }
}

std::string WeatherManager::nextWeather(const ESM::Region* region) const
{
    std::vector<char> probability;

    RegionModMap::const_iterator iter = mRegionMods.find(Misc::StringUtils::lowerCase(region->mId));
    if(iter != mRegionMods.end())
        probability = iter->second;
    else
    {
        probability.reserve(10);
        probability.push_back(region->mData.mClear);
        probability.push_back(region->mData.mCloudy);
        probability.push_back(region->mData.mFoggy);
        probability.push_back(region->mData.mOvercast);
        probability.push_back(region->mData.mRain);
        probability.push_back(region->mData.mThunder);
        probability.push_back(region->mData.mAsh);
        probability.push_back(region->mData.mBlight);
        probability.push_back(region->mData.mA);
        probability.push_back(region->mData.mB);
    }

    /*
     * All probabilities must add to 100 (responsibility of the user).
     * If chances A and B has values 30 and 70 then by generating
     * 100 numbers 1..100, 30% will be lesser or equal 30 and
     * 70% will be greater than 30 (in theory).
     */

    int chance = OEngine::Misc::Rng::rollDice(100) + 1; // 1..100
    int sum = 0;
    unsigned int i = 0;
    for (; i < probability.size(); ++i)
    {
        sum += probability[i];
        if (chance < sum)
            break;
    }

    switch (i)
    {
        case 1:
            return "cloudy";
        case 2:
            return "foggy";
        case 3:
            return "overcast";
        case 4:
            return "rain";
        case 5:
            return "thunderstorm";
        case 6:
            return "ashstorm";
        case 7:
            return "blight";
        case 8:
            return "snow";
        case 9:
            return "blizzard";
        default: // case 0
            return "clear";
    }
}

void WeatherManager::setHour(const float hour)
{
    mHour = hour;
}

unsigned int WeatherManager::getWeatherID() const
{
    // Source: http://www.uesp.net/wiki/Tes3Mod:GetCurrentWeather

    if (mCurrentWeather == "clear")
        return 0;
    else if (mCurrentWeather == "cloudy")
        return 1;
    else if (mCurrentWeather == "foggy")
        return 2;
    else if (mCurrentWeather == "overcast")
        return 3;
    else if (mCurrentWeather == "rain")
        return 4;
    else if (mCurrentWeather == "thunderstorm")
        return 5;
    else if (mCurrentWeather == "ashstorm")
        return 6;
    else if (mCurrentWeather == "blight")
        return 7;
    else if (mCurrentWeather == "snow")
        return 8;
    else if (mCurrentWeather == "blizzard")
        return 9;

    else
        return 0;
}

void WeatherManager::changeWeather(const std::string& region, const unsigned int id)
{
    // make sure this region exists
    MWBase::Environment::get().getWorld()->getStore().get<ESM::Region>().find(region);

    std::string weather;
    if (id==0)
        weather = "clear";
    else if (id==1)
        weather = "cloudy";
    else if (id==2)
        weather = "foggy";
    else if (id==3)
        weather = "overcast";
    else if (id==4)
        weather = "rain";
    else if (id==5)
        weather = "thunderstorm";
    else if (id==6)
        weather = "ashstorm";
    else if (id==7)
        weather = "blight";
    else if (id==8)
        weather = "snow";
    else if (id==9)
        weather = "blizzard";
    else
        weather = "clear";

    mRegionOverrides[Misc::StringUtils::lowerCase(region)] = weather;

    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
    if (player.isInCell())
    {
        std::string playerRegion = player.getCell()->getCell()->mRegion;
        if (Misc::StringUtils::ciEqual(region, playerRegion))
            setWeather(weather);
    }
}

void WeatherManager::modRegion(const std::string &regionid, const std::vector<char> &chances)
{
    mRegionMods[Misc::StringUtils::lowerCase(regionid)] = chances;
    // Start transitioning right away if the region no longer supports the current weather type
    unsigned int current = getWeatherID();
    if(current >= chances.size() || chances[current] == 0)
        mWeatherUpdateTime = 0.0f;
}

float WeatherManager::getWindSpeed() const
{
    return mWindSpeed;
}

bool WeatherManager::isDark() const
{
    bool exterior = (MWBase::Environment::get().getWorld()->isCellExterior()
                     || MWBase::Environment::get().getWorld()->isCellQuasiExterior());
    return exterior && (mHour < mSunriseTime || mHour > mNightStart - 1);
}

void WeatherManager::write(ESM::ESMWriter& writer, Loading::Listener& progress)
{
    ESM::WeatherState state;
    state.mHour = mHour;
    state.mWindSpeed = mWindSpeed;
    state.mCurrentWeather = mCurrentWeather;
    state.mNextWeather = mNextWeather;
    state.mCurrentRegion = mCurrentRegion;
    state.mFirstUpdate = mFirstUpdate;
    state.mRemainingTransitionTime = mRemainingTransitionTime;
    state.mTimePassed = mTimePassed;

    writer.startRecord(ESM::REC_WTHR);
    state.save(writer);
    writer.endRecord(ESM::REC_WTHR);
}

bool WeatherManager::readRecord(ESM::ESMReader& reader, uint32_t type)
{
    if(ESM::REC_WTHR == type)
    {
        // load first so that if it fails, we haven't accidentally reset the state below
        ESM::WeatherState state;
        state.load(reader);

        // swap in the loaded values now that we can't fail
        mHour = state.mHour;
        mWindSpeed = state.mWindSpeed;
        mCurrentWeather.swap(state.mCurrentWeather);
        mNextWeather.swap(state.mNextWeather);
        mCurrentRegion.swap(state.mCurrentRegion);
        mFirstUpdate = state.mFirstUpdate;
        mRemainingTransitionTime = state.mRemainingTransitionTime;
        mTimePassed = state.mTimePassed;

        return true;
    }

    return false;
}

void WeatherManager::clear()
{
    stopSounds();
    mRegionOverrides.clear();
    mRegionMods.clear();
    mThunderFlash = 0.0;
    mThunderChance = 0.0;
    mThunderChanceNeeded = 50.0;
}

void WeatherManager::switchToNextWeather(bool instantly)
{
    MWBase::World* world = MWBase::Environment::get().getWorld();
    if (world->isCellExterior() || world->isCellQuasiExterior())
    {
        std::string regionstr = Misc::StringUtils::lowerCase(world->getPlayerPtr().getCell()->getCell()->mRegion);

        if (mWeatherUpdateTime <= 0 || regionstr != mCurrentRegion)
        {
            mCurrentRegion = regionstr;
            mWeatherUpdateTime = mHoursBetweenWeatherChanges * 3600;

            std::string weatherType = "clear";

            if (mRegionOverrides.find(regionstr) != mRegionOverrides.end())
            {
                weatherType = mRegionOverrides[regionstr];
            }
            else
            {
                // get weather probabilities for the current region
                const ESM::Region *region = world->getStore().get<ESM::Region>().search (regionstr);

                if (region != 0)
                {
                    weatherType = nextWeather(region);
                }
            }

            setWeather(weatherType, instantly);
        }
    }
}

bool WeatherManager::isInStorm() const
{
    return mIsStorm;
}

Ogre::Vector3 WeatherManager::getStormDirection() const
{
    return mStormDirection;
}
