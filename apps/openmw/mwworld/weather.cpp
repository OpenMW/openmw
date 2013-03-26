#include "weather.hpp"

#include <ctime>
#include <cstdlib>

#include <boost/algorithm/string.hpp>
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwrender/renderingmanager.hpp"

#include "player.hpp"
#include "esmstore.hpp"
#include "fallback.hpp"

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
    mWeatherSettings[name] = weather;
}

WeatherManager::WeatherManager(MWRender::RenderingManager* rendering,MWWorld::Fallback* fallback) :
     mHour(14), mCurrentWeather("clear"), mFirstUpdate(true), mWeatherUpdateTime(0),
     mThunderFlash(0), mThunderChance(0), mThunderChanceNeeded(50), mThunderSoundDelay(0),
     mRemainingTransitionTime(0), mMonth(0), mDay(0),
     mTimePassed(0), mFallback(fallback)
{
    mRendering = rendering;
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
    mWeatherUpdateTime = mHoursBetweenWeatherChanges*3600;
    mThunderFrequency = mFallback->getFallbackFloat("Weather_Thunderstorm_Thunder_Frequency");
    mThunderThreshold = mFallback->getFallbackFloat("Weather_Thunderstorm_Thunder_Threshold");
    mThunderSoundDelay = 0.25;
    //Weather
    Weather clear;
    clear.mCloudTexture = "tx_sky_clear.dds";
    setFallbackWeather(clear,"clear");

    Weather cloudy;
    cloudy.mCloudTexture = "tx_sky_cloudy.dds";
    setFallbackWeather(cloudy,"cloudy");

    Weather foggy;
    foggy.mCloudTexture = "tx_sky_foggy.dds";
    setFallbackWeather(foggy,"foggy");

    Weather thunderstorm;
    thunderstorm.mCloudTexture = "tx_sky_thunder.dds";
    thunderstorm.mRainLoopSoundID = "rain heavy";
    setFallbackWeather(thunderstorm,"thunderstorm");

    Weather rain;
    rain.mCloudTexture = "tx_sky_rainy.dds";
    rain.mRainLoopSoundID = "rain";
    setFallbackWeather(rain,"rain");

    Weather overcast;
    overcast.mCloudTexture = "tx_sky_overcast.dds";
    setFallbackWeather(overcast,"overcast");

    Weather ashstorm;
    ashstorm.mCloudTexture = "tx_sky_ashstorm.dds";
    ashstorm.mAmbientLoopSoundID = "ashstorm";
    setFallbackWeather(ashstorm,"ashstorm");

    Weather blight;
    blight.mCloudTexture = "tx_sky_blight.dds";
    blight.mAmbientLoopSoundID = "blight";
    setFallbackWeather(blight,"blight");

    Weather snow;
    snow.mCloudTexture = "tx_bm_sky_snow.dds";
    setFallbackWeather(snow, "snow");

    Weather blizzard;
    blizzard.mCloudTexture = "tx_bm_sky_blizzard.dds";
    blizzard.mAmbientLoopSoundID = "BM Blizzard";
    setFallbackWeather(blizzard,"blizzard");
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
            if (mRemainingTransitionTime/(mWeatherSettings[mCurrentWeather].mTransitionDelta*24.f*3600) <= 0.5)
                mCurrentWeather = mNextWeather;
        }

        mNextWeather = weather;
        mRemainingTransitionTime = mWeatherSettings[mCurrentWeather].mTransitionDelta*24.f*3600;
    }
    mFirstUpdate = false;
}

WeatherResult WeatherManager::getResult(const String& weather)
{
    const Weather& current = mWeatherSettings[weather];
    WeatherResult result;

    result.mCloudTexture = current.mCloudTexture;
    result.mCloudBlendFactor = 0;
    result.mCloudOpacity = current.mCloudsMaximumPercent;
    result.mWindSpeed = current.mWindSpeed;
    result.mCloudSpeed = current.mCloudSpeed;
    result.mGlareView = current.mGlareView;
    result.mAmbientLoopSoundID = current.mAmbientLoopSoundID;
    result.mSunColor = current.mSunDiscSunsetColor;

    result.mNight = (mHour < 6 || mHour > 19);

    result.mFogDepth = result.mNight ? current.mLandFogNightDepth : current.mLandFogDayDepth;

    // night
    if (mHour <= 5.5f || mHour >= 21)
    {
        result.mFogColor = current.mFogNightColor;
        result.mAmbientColor = current.mAmbientNightColor;
        result.mSunColor = current.mSunNightColor;
        result.mSkyColor = current.mSkyNightColor;
        result.mNightFade = 1.f;
    }

    // sunrise
    else if (mHour >= 5.5f && mHour <= 9)
    {
        if (mHour <= 6)
        {
            // fade in
            float advance = 6-mHour;
            float factor = advance / 0.5f;
            result.mFogColor = lerp(current.mFogSunriseColor, current.mFogNightColor, factor);
            result.mAmbientColor = lerp(current.mAmbientSunriseColor, current.mAmbientNightColor, factor);
            result.mSunColor = lerp(current.mSunSunriseColor, current.mSunNightColor, factor);
            result.mSkyColor = lerp(current.mSkySunriseColor, current.mSkyNightColor, factor);
            result.mNightFade = factor;
        }
        else //if (mHour >= 6)
        {
            // fade out
            float advance = mHour-6;
            float factor = advance / 3.f;
            result.mFogColor = lerp(current.mFogSunriseColor, current.mFogDayColor, factor);
            result.mAmbientColor = lerp(current.mAmbientSunriseColor, current.mAmbientDayColor, factor);
            result.mSunColor = lerp(current.mSunSunriseColor, current.mSunDayColor, factor);
            result.mSkyColor = lerp(current.mSkySunriseColor, current.mSkyDayColor, factor);
        }
    }

    // day
    else if (mHour >= 9 && mHour <= 17)
    {
        result.mFogColor = current.mFogDayColor;
        result.mAmbientColor = current.mAmbientDayColor;
        result.mSunColor = current.mSunDayColor;
        result.mSkyColor = current.mSkyDayColor;
    }

    // sunset
    else if (mHour >= 17 && mHour <= 21)
    {
        if (mHour <= 19)
        {
            // fade in
            float advance = 19-mHour;
            float factor = (advance / 2);
            result.mFogColor = lerp(current.mFogSunsetColor, current.mFogDayColor, factor);
            result.mAmbientColor = lerp(current.mAmbientSunsetColor, current.mAmbientDayColor, factor);
            result.mSunColor = lerp(current.mSunSunsetColor, current.mSunDayColor, factor);
            result.mSkyColor = lerp(current.mSkySunsetColor, current.mSkyDayColor, factor);
        }
        else //if (mHour >= 19)
        {
            // fade out
            float advance = mHour-19;
            float factor = advance / 2.f;
            result.mFogColor = lerp(current.mFogSunsetColor, current.mFogNightColor, factor);
            result.mAmbientColor = lerp(current.mAmbientSunsetColor, current.mAmbientNightColor, factor);
            result.mSunColor = lerp(current.mSunSunsetColor, current.mSunNightColor, factor);
            result.mSkyColor = lerp(current.mSkySunsetColor, current.mSkyNightColor, factor);
            result.mNightFade = factor;
        }
    }

    return result;
}

WeatherResult WeatherManager::transition(float factor)
{
    const WeatherResult& current = getResult(mCurrentWeather);
    const WeatherResult& other = getResult(mNextWeather);
    WeatherResult result;

    result.mCloudTexture = current.mCloudTexture;
    result.mNextCloudTexture = other.mCloudTexture;
    result.mCloudBlendFactor = factor;

    result.mCloudOpacity = lerp(current.mCloudOpacity, other.mCloudOpacity, factor);
    result.mFogColor = lerp(current.mFogColor, other.mFogColor, factor);
    result.mSunColor = lerp(current.mSunColor, other.mSunColor, factor);
    result.mSkyColor = lerp(current.mSkyColor, other.mSkyColor, factor);

    result.mAmbientColor = lerp(current.mAmbientColor, other.mAmbientColor, factor);
    result.mSunDiscColor = lerp(current.mSunDiscColor, other.mSunDiscColor, factor);
    result.mFogDepth = lerp(current.mFogDepth, other.mFogDepth, factor);
    result.mWindSpeed = lerp(current.mWindSpeed, other.mWindSpeed, factor);
    result.mCloudSpeed = lerp(current.mCloudSpeed, other.mCloudSpeed, factor);
    result.mCloudOpacity = lerp(current.mCloudOpacity, other.mCloudOpacity, factor);
    result.mGlareView = lerp(current.mGlareView, other.mGlareView, factor);
    result.mNightFade = lerp(current.mNightFade, other.mNightFade, factor);

    result.mNight = current.mNight;

    return result;
}

void WeatherManager::update(float duration)
{
    float timePassed = mTimePassed;
    mTimePassed = 0;

    mWeatherUpdateTime -= timePassed;

    bool exterior = (MWBase::Environment::get().getWorld()->isCellExterior() || MWBase::Environment::get().getWorld()->isCellQuasiExterior());

    if (exterior)
    {
        std::string regionstr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell()->mCell->mRegion;
        Misc::StringUtils::toLower(regionstr);

        if (mWeatherUpdateTime <= 0 || regionstr != mCurrentRegion)
        {
            mCurrentRegion = regionstr;
            mWeatherUpdateTime = mHoursBetweenWeatherChanges*3600;

            std::string weather = "clear";

            if (mRegionOverrides.find(regionstr) != mRegionOverrides.end())
                weather = mRegionOverrides[regionstr];
            else
            {
                // get weather probabilities for the current region
                const ESM::Region *region =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Region>().search (regionstr);

                if (region != 0)
                {
                    float clear = region->mData.mClear/255.f;
                    float cloudy = region->mData.mCloudy/255.f;
                    float foggy = region->mData.mFoggy/255.f;
                    float overcast = region->mData.mOvercast/255.f;
                    float rain = region->mData.mRain/255.f;
                    float thunder = region->mData.mThunder/255.f;
                    float ash = region->mData.mAsh/255.f;
                    float blight = region->mData.mBlight/255.f;
                    float snow = region->mData.mA/255.f;
                    float blizzard = region->mData.mB/255.f;

                    // re-scale to 100 percent
                    const float total = clear+cloudy+foggy+overcast+rain+thunder+ash+blight+snow+blizzard;

                    float random = ((rand()%100)/100.f) * total;

                    if (random >= snow+blight+ash+thunder+rain+overcast+foggy+cloudy+clear)
                        weather = "blizzard";
                    else if (random >= blight+ash+thunder+rain+overcast+foggy+cloudy+clear)
                        weather = "snow";
                    else if (random >= ash+thunder+rain+overcast+foggy+cloudy+clear)
                        weather = "blight";
                    else if (random >= thunder+rain+overcast+foggy+cloudy+clear)
                        weather = "ashstorm";
                    else if (random >= rain+overcast+foggy+cloudy+clear)
                        weather = "thunderstorm";
                    else if (random >= overcast+foggy+cloudy+clear)
                        weather = "rain";
                    else if (random >= foggy+cloudy+clear)
                        weather = "overcast";
                    else if (random >= cloudy+clear)
                        weather = "foggy";
                    else if (random >= clear)
                        weather = "cloudy";
                    else
                        weather = "clear";
                }
            }

            setWeather(weather, false);
        }

        WeatherResult result;

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
            result = transition(1-(mRemainingTransitionTime/(mWeatherSettings[mCurrentWeather].mTransitionDelta*24.f*3600)));
        else
            result = getResult(mCurrentWeather);

        mRendering->configureFog(result.mFogDepth, result.mFogColor);

        // disable sun during night
        if (mHour >= 20 || mHour <= 6.f)
            mRendering->getSkyManager()->sunDisable();
        else
            mRendering->getSkyManager()->sunEnable();

        // sun angle
        float height;

        // rise at 6, set at 20
        if (mHour >= 6 && mHour <= 20)
            height = 1-std::abs(((mHour-13)/7.f));
        else if (mHour > 20)
            height = (mHour-20.f)/4.f;
        else //if (mHour > 0 && mHour < 6)
            height = 1-(mHour/6.f);

        int facing = (mHour > 13.f) ? 1 : -1;

        Vector3 final(
            -(1-height)*facing,
            -(1-height)*facing,
            height);
        mRendering->setSunDirection(final);

        // moon calculations
        float night;
        if (mHour >= 14)
            night = mHour-14;
        else if (mHour <= 10)
            night = mHour+10;
        else
            night = 0;

        night /= 20.f;

        if (night != 0)
        {
            float moonHeight = 1-std::abs((night-0.5)*2);
            int facing = (mHour > 0.f && mHour<12.f) ? 1 : -1;
            Vector3 masser(
                (1-moonHeight)*facing,
                (1-moonHeight)*facing,
                moonHeight);

            Vector3 secunda(
                (1-moonHeight)*facing*0.8,
                (1-moonHeight)*facing*1.25,
                moonHeight);

            mRendering->getSkyManager()->setMasserDirection(masser);
            mRendering->getSkyManager()->setSecundaDirection(secunda);
            mRendering->getSkyManager()->masserEnable();
            mRendering->getSkyManager()->secundaEnable();

            float secunda_hour_fade;
            float secunda_fadeout_start=mFallback->getFallbackFloat("Moons_Secunda_Fade_Out_Start");
            float secunda_fadein_start=mFallback->getFallbackFloat("Moons_Secunda_Fade_In_Start");
            float secunda_fadein_finish=mFallback->getFallbackFloat("Moons_Secunda_Fade_In_Finish");

            if (mHour >= secunda_fadeout_start && mHour <= secunda_fadein_start)
                secunda_hour_fade = 1-(mHour-secunda_fadeout_start)/3.f;
            else if (mHour >= secunda_fadein_start && mHour <= secunda_fadein_finish)
                secunda_hour_fade = mHour-secunda_fadein_start;
            else
                secunda_hour_fade = 1;

            float masser_hour_fade;
            float masser_fadeout_start=mFallback->getFallbackFloat("Moons_Masser_Fade_Out_Start");
            float masser_fadein_start=mFallback->getFallbackFloat("Moons_Masser_Fade_In_Start");
            float masser_fadein_finish=mFallback->getFallbackFloat("Moons_Masser_Fade_In_Finish");
            if (mHour >= masser_fadeout_start && mHour <= masser_fadein_start)
                masser_hour_fade = 1-(mHour-masser_fadeout_start)/3.f;
            else if (mHour >= masser_fadein_start && mHour <= masser_fadein_finish)
                masser_hour_fade = mHour-masser_fadein_start;
            else
                masser_hour_fade = 1;

            float angle = moonHeight*90.f;

            float secunda_angle_fade;
            float secunda_end_angle=mFallback->getFallbackFloat("Moons_Secunda_Fade_End_Angle");
            float secunda_start_angle=mFallback->getFallbackFloat("Moons_Secunda_Fade_Start_Angle");
            if (angle >= secunda_end_angle && angle <= secunda_start_angle)
                secunda_angle_fade = (angle-secunda_end_angle)/20.f;
            else if (angle < secunda_end_angle)
                secunda_angle_fade = 0.f;
            else
                secunda_angle_fade = 1.f;

            float masser_angle_fade;
            float masser_end_angle=mFallback->getFallbackFloat("Moons_Masser_Fade_End_Angle");
            float masser_start_angle=mFallback->getFallbackFloat("Moons_Masser_Fade_Start_Angle");
            if (angle >= masser_end_angle && angle <= masser_start_angle)
                masser_angle_fade = (angle-masser_end_angle)/10.f;
            else if (angle < masser_end_angle)
                masser_angle_fade = 0.f;
            else
                masser_angle_fade = 1.f;

            masser_angle_fade *= masser_hour_fade;
            secunda_angle_fade *= secunda_hour_fade;

            mRendering->getSkyManager()->setMasserFade(masser_angle_fade);
            mRendering->getSkyManager()->setSecundaFade(secunda_angle_fade);
        }
        else
        {
            mRendering->getSkyManager()->masserDisable();
            mRendering->getSkyManager()->secundaDisable();
        }

        if (mCurrentWeather == "thunderstorm" && mNextWeather == "" && exterior)
        {
            if (mThunderFlash > 0)
            {
                // play the sound after a delay
                mThunderSoundDelay -= duration;
                if (mThunderSoundDelay <= 0)
                {
                    // pick a random sound
                    int sound = rand() % 4;
                    std::string soundname;
                    if (sound == 0) soundname = mThunderSoundID0;
                    else if (sound == 1) soundname = mThunderSoundID1;
                    else if (sound == 2) soundname = mThunderSoundID2;
                    else if (sound == 3) soundname = mThunderSoundID3;
                    MWBase::Environment::get().getSoundManager()->playSound(soundname, 1.0, 1.0);
                    mThunderSoundDelay = 1000;
                }

                mThunderFlash -= duration;
                if (mThunderFlash > 0)
                    mRendering->getSkyManager()->setLightningStrength( mThunderFlash / mThunderThreshold );
                else
                {
                    srand(time(NULL));
                    mThunderChanceNeeded = rand() % 100;
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

        mRendering->setAmbientColour(result.mAmbientColor);
        mRendering->sunEnable(false);
        mRendering->setSunColour(result.mSunColor);

        mRendering->getSkyManager()->setWeather(result);
    }
    else
    {
        mRendering->sunDisable(false);
        mRendering->skyDisable();
        mRendering->getSkyManager()->setLightningStrength(0.f);
    }

    // play sounds
    std::string ambientSnd = (mNextWeather == "" ? mWeatherSettings[mCurrentWeather].mAmbientLoopSoundID : "");
    if (!exterior) ambientSnd = "";
    if (ambientSnd != "")
    {
        if (std::find(mSoundsPlaying.begin(), mSoundsPlaying.end(), ambientSnd) == mSoundsPlaying.end())
        {
            mSoundsPlaying.push_back(ambientSnd);
            MWBase::Environment::get().getSoundManager()->playSound(ambientSnd, 1.0, 1.0, MWBase::SoundManager::Play_Loop);
        }
    }

    std::string rainSnd = (mNextWeather == "" ? mWeatherSettings[mCurrentWeather].mRainLoopSoundID : "");
    if (!exterior) rainSnd = "";
    if (rainSnd != "")
    {
        if (std::find(mSoundsPlaying.begin(), mSoundsPlaying.end(), rainSnd) == mSoundsPlaying.end())
        {
            mSoundsPlaying.push_back(rainSnd);
            MWBase::Environment::get().getSoundManager()->playSound(rainSnd, 1.0, 1.0, MWBase::SoundManager::Play_Loop);
        }
    }

    // stop sounds
    std::vector<std::string>::iterator it=mSoundsPlaying.begin();
    while (it!=mSoundsPlaying.end())
    {
        if ( *it != ambientSnd && *it != rainSnd)
        {
            MWBase::Environment::get().getSoundManager()->stopSound(*it);
            it = mSoundsPlaying.erase(it);
        }
        else
            ++it;
    }
}

void WeatherManager::setHour(const float hour)
{
    mHour = hour;
}

void WeatherManager::setDate(const int day, const int month)
{
    mDay = day;
    mMonth = month;
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

    mRegionOverrides[region] = weather;
}
