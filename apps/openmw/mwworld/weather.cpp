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

using namespace Ogre;
using namespace MWWorld;
using namespace MWSound;

#define lerp(x, y) (x * (1-factor) + y * factor)

const std::string WeatherGlobals::mThunderSoundID0 = "Thunder0";
const std::string WeatherGlobals::mThunderSoundID1 = "Thunder1";
const std::string WeatherGlobals::mThunderSoundID2 = "Thunder2";
const std::string WeatherGlobals::mThunderSoundID3 = "Thunder3";
const float WeatherGlobals::mSunriseTime = 8;
const float WeatherGlobals::mSunsetTime = 18;
const float WeatherGlobals::mSunriseDuration = 2;
const float WeatherGlobals::mSunsetDuration = 2;
const float WeatherGlobals::mWeatherUpdateTime = 20.f;


// morrowind sets these per-weather, but since they are only used by 'thunderstorm'
// weather setting anyway, we can just as well set them globally
const float WeatherGlobals::mThunderFrequency = .4;
const float WeatherGlobals::mThunderThreshold = 0.6;
const float WeatherGlobals::mThunderSoundDelay = 0.25;

WeatherManager::WeatherManager(MWRender::RenderingManager* rendering) :
     mHour(14), mCurrentWeather("clear"), mFirstUpdate(true), mWeatherUpdateTime(0),
     mThunderFlash(0), mThunderChance(0), mThunderChanceNeeded(50), mThunderSoundDelay(0),
     mRemainingTransitionTime(0), mMonth(0), mDay(0),
     mTimePassed(0)
{
    mRendering = rendering;

    #define clr(r,g,b) ColourValue(r/255.f, g/255.f, b/255.f)

    /// \todo read these from Morrowind.ini
    Weather clear;
    clear.mCloudTexture = "tx_sky_clear.dds";
    clear.mCloudsMaximumPercent = 1.0;
    clear.mTransitionDelta = 0.015;
    clear.mSkySunriseColor = clr(118, 141, 164);
    clear.mSkyDayColor = clr(95, 135, 203);
    clear.mSkySunsetColor = clr(56, 89, 129);
    clear.mSkyNightColor = clr(9, 10, 11);
    clear.mFogSunriseColor = clr(255, 189, 157);
    clear.mFogDayColor = clr(206, 227, 255);
    clear.mFogSunsetColor = clr(255, 189, 157);
    clear.mFogNightColor = clr(9, 10, 11);
    clear.mAmbientSunriseColor = clr(47, 66, 96);
    clear.mAmbientDayColor = clr(137, 140, 160);
    clear.mAmbientSunsetColor = clr(68, 75, 96);
    clear.mAmbientNightColor = clr(32, 35, 42);
    clear.mSunSunriseColor = clr(242, 159, 99);
    clear.mSunDayColor = clr(255, 252, 238);
    clear.mSunSunsetColor = clr(255, 115, 79);
    clear.mSunNightColor = clr(59, 97, 176);
    clear.mSunDiscSunsetColor = clr(255, 189, 157);
    clear.mLandFogDayDepth = 0.69;
    clear.mLandFogNightDepth = 0.69;
    clear.mWindSpeed = 0.1;
    clear.mCloudSpeed = 1.25;
    clear.mGlareView = 1.0;
    mWeatherSettings["clear"] = clear;

    Weather cloudy;
    cloudy.mCloudTexture = "tx_sky_cloudy.dds";
    cloudy.mCloudsMaximumPercent = 1.0;
    cloudy.mTransitionDelta = 0.015;
    cloudy.mSkySunriseColor = clr(126, 158, 173);
    cloudy.mSkyDayColor = clr(117, 160, 215);
    cloudy.mSkySunsetColor = clr(111, 114, 159);
    cloudy.mSkyNightColor = clr(9, 10, 11);
    cloudy.mFogSunriseColor = clr(255, 207, 149);
    cloudy.mFogDayColor = clr(245, 235, 224);
    cloudy.mFogSunsetColor = clr(255, 155, 106);
    cloudy.mFogNightColor = clr(9, 10, 11);
    cloudy.mAmbientSunriseColor = clr(66, 74, 87);
    cloudy.mAmbientDayColor = clr(137, 145, 160);
    cloudy.mAmbientSunsetColor = clr(71, 80, 92);
    cloudy.mAmbientNightColor = clr(32, 39, 54);
    cloudy.mSunSunriseColor = clr(241, 177, 99);
    cloudy.mSunDayColor = clr(255, 236, 221);
    cloudy.mSunSunsetColor = clr(255, 89, 00);
    cloudy.mSunNightColor = clr(77, 91, 124);
    cloudy.mSunDiscSunsetColor = clr(255, 202, 179);
    cloudy.mLandFogDayDepth = 0.72;
    cloudy.mLandFogNightDepth = 0.72;
    cloudy.mWindSpeed = 0.2;
    cloudy.mCloudSpeed = 2;
    cloudy.mGlareView = 1.0;
    mWeatherSettings["cloudy"] = cloudy;

    Weather foggy;
    foggy.mCloudTexture = "tx_sky_foggy.dds";
    foggy.mCloudsMaximumPercent = 1.0;
    foggy.mTransitionDelta = 0.015;
    foggy.mSkySunriseColor = clr(197, 190, 180);
    foggy.mSkyDayColor = clr(184, 211, 228);
    foggy.mSkySunsetColor = clr(142, 159, 176);
    foggy.mSkyNightColor = clr(18, 23, 28);
    foggy.mFogSunriseColor = clr(173, 164, 148);
    foggy.mFogDayColor = clr(150, 187, 209);
    foggy.mFogSunsetColor = clr(113, 135, 157);
    foggy.mFogNightColor = clr(19, 24, 29);
    foggy.mAmbientSunriseColor = clr(48, 43, 37);
    foggy.mAmbientDayColor = clr(92, 109, 120);
    foggy.mAmbientSunsetColor = clr(28, 33, 39);
    foggy.mAmbientNightColor = clr(28, 33, 39);
    foggy.mSunSunriseColor = clr(177, 162, 137);
    foggy.mSunDayColor = clr(111, 131, 151);
    foggy.mSunSunsetColor = clr(125, 157, 189);
    foggy.mSunNightColor = clr(81, 100, 119);
    foggy.mSunDiscSunsetColor = clr(223, 223, 223);
    foggy.mLandFogDayDepth = 1.0;
    foggy.mLandFogNightDepth = 1.9;
    foggy.mWindSpeed = 0;
    foggy.mCloudSpeed = 1.25;
    foggy.mGlareView = 0.25;
    mWeatherSettings["foggy"] = foggy;

    Weather thunderstorm;
    thunderstorm.mCloudTexture = "tx_sky_thunder.dds";
    thunderstorm.mCloudsMaximumPercent = 0.66;
    thunderstorm.mTransitionDelta = 0.03;
    thunderstorm.mSkySunriseColor = clr(35, 36, 39);
    thunderstorm.mSkyDayColor = clr(97, 104, 115);
    thunderstorm.mSkySunsetColor = clr(35, 36, 39);
    thunderstorm.mSkyNightColor = clr(19, 20, 22);
    thunderstorm.mFogSunriseColor = clr(70, 74, 85);
    thunderstorm.mFogDayColor = clr(97, 104, 115);
    thunderstorm.mFogSunsetColor = clr(70, 74, 85);
    thunderstorm.mFogNightColor = clr(19, 20, 22);
    thunderstorm.mAmbientSunriseColor = clr(54, 54, 54);
    thunderstorm.mAmbientDayColor = clr(90, 90, 90);
    thunderstorm.mAmbientSunsetColor = clr(54, 54, 54);
    thunderstorm.mAmbientNightColor = clr(49, 51, 54);
    thunderstorm.mSunSunriseColor = clr(91, 99, 122);
    thunderstorm.mSunDayColor = clr(138, 144, 155);
    thunderstorm.mSunSunsetColor = clr(96, 101, 117);
    thunderstorm.mSunNightColor = clr(55, 76, 110);
    thunderstorm.mSunDiscSunsetColor = clr(128, 128, 128);
    thunderstorm.mLandFogDayDepth = 1;
    thunderstorm.mLandFogNightDepth = 1.15;
    thunderstorm.mWindSpeed = 0.5;
    thunderstorm.mCloudSpeed = 3;
    thunderstorm.mGlareView = 0;
    thunderstorm.mRainLoopSoundID = "rain heavy";
    mWeatherSettings["thunderstorm"] = thunderstorm;

    Weather rain;
    rain.mCloudTexture = "tx_sky_rainy.dds";
    rain.mCloudsMaximumPercent = 0.66;
    rain.mTransitionDelta = 0.015;
    rain.mSkySunriseColor = clr(71, 74, 75);
    rain.mSkyDayColor = clr(116, 120, 122);
    rain.mSkySunsetColor = clr(73, 73, 73);
    rain.mSkyNightColor = clr(24, 25, 26);
    rain.mFogSunriseColor = clr(71, 74, 75);
    rain.mFogDayColor = clr(116, 120, 122);
    rain.mFogSunsetColor = clr(73, 73, 73);
    rain.mFogNightColor = clr(24, 25, 26);
    rain.mAmbientSunriseColor = clr(97, 90, 88);
    rain.mAmbientDayColor = clr(105, 110, 113);
    rain.mAmbientSunsetColor = clr(88, 97, 97);
    rain.mAmbientNightColor = clr(50, 55, 67);
    rain.mSunSunriseColor = clr(131, 122, 120);
    rain.mSunDayColor = clr(149, 157, 170);
    rain.mSunSunsetColor = clr(120, 126, 131);
    rain.mSunNightColor = clr(50, 62, 101);
    rain.mSunDiscSunsetColor = clr(128, 128, 128);
    rain.mLandFogDayDepth = 0.8;
    rain.mLandFogNightDepth = 0.8;
    rain.mWindSpeed = 0.3;
    rain.mCloudSpeed = 2;
    rain.mGlareView = 0;
    rain.mRainLoopSoundID = "rain";
    mWeatherSettings["rain"] = rain;

    Weather overcast;
    overcast.mCloudTexture = "tx_sky_overcast.dds";
    overcast.mCloudsMaximumPercent = 1.0;
    overcast.mTransitionDelta = 0.015;
    overcast.mSkySunriseColor = clr(91, 99, 106);
    overcast.mSkyDayColor = clr(143, 146, 149);
    overcast.mSkySunsetColor = clr(108, 115, 121);
    overcast.mSkyNightColor = clr(19, 22, 25);
    overcast.mFogSunriseColor = clr(91, 99, 106);
    overcast.mFogDayColor = clr(143, 146, 149);
    overcast.mFogSunsetColor = clr(108, 115, 121);
    overcast.mFogNightColor = clr(19, 22, 25);
    overcast.mAmbientSunriseColor = clr(84, 88, 92);
    overcast.mAmbientDayColor = clr(93, 96, 105);
    overcast.mAmbientSunsetColor = clr(83, 77, 75);
    overcast.mAmbientNightColor = clr(57, 60, 66);
    overcast.mSunSunriseColor = clr(87, 125, 163);
    overcast.mSunDayColor = clr(163, 169, 183);
    overcast.mSunSunsetColor = clr(85, 103, 157);
    overcast.mSunNightColor = clr(32, 54, 100);
    overcast.mSunDiscSunsetColor = clr(128, 128, 128);
    overcast.mLandFogDayDepth = 0.7;
    overcast.mLandFogNightDepth = 0.7;
    overcast.mWindSpeed = 0.2;
    overcast.mCloudSpeed = 1.5;
    overcast.mGlareView = 0;
    mWeatherSettings["overcast"] = overcast;

    Weather ashstorm;
    ashstorm.mCloudTexture = "tx_sky_ashstorm.dds";
    ashstorm.mCloudsMaximumPercent = 1.0;
    ashstorm.mTransitionDelta = 0.035;
    ashstorm.mSkySunriseColor = clr(91, 56, 51);
    ashstorm.mSkyDayColor = clr(124, 73, 58);
    ashstorm.mSkySunsetColor = clr(106, 55, 40);
    ashstorm.mSkyNightColor = clr(20, 21, 22);
    ashstorm.mFogSunriseColor = clr(91, 56, 51);
    ashstorm.mFogDayColor = clr(124, 73, 58);
    ashstorm.mFogSunsetColor = clr(106, 55, 40);
    ashstorm.mFogNightColor = clr(20, 21, 22);
    ashstorm.mAmbientSunriseColor = clr(52, 42, 37);
    ashstorm.mAmbientDayColor = clr(75, 49, 41);
    ashstorm.mAmbientSunsetColor = clr(48, 39, 35);
    ashstorm.mAmbientNightColor = clr(36, 42, 49);
    ashstorm.mSunSunriseColor = clr(184, 91, 71);
    ashstorm.mSunDayColor = clr(228, 139, 114);
    ashstorm.mSunSunsetColor = clr(185, 86, 57);
    ashstorm.mSunNightColor = clr(54, 66, 74);
    ashstorm.mSunDiscSunsetColor = clr(128, 128, 128);
    ashstorm.mLandFogDayDepth = 1.1;
    ashstorm.mLandFogNightDepth = 1.2;
    ashstorm.mWindSpeed = 0.8;
    ashstorm.mCloudSpeed = 7;
    ashstorm.mGlareView = 0;
    ashstorm.mAmbientLoopSoundID = "ashstorm";
    mWeatherSettings["ashstorm"] = ashstorm;

    Weather blight;
    blight.mCloudTexture = "tx_sky_blight.dds";
    blight.mCloudsMaximumPercent = 1.0;
    blight.mTransitionDelta = 0.04;
    blight.mSkySunriseColor = clr(90, 35, 35);
    blight.mSkyDayColor = clr(90, 35, 35);
    blight.mSkySunsetColor = clr(92, 33, 33);
    blight.mSkyNightColor = clr(44, 14, 14);
    blight.mFogSunriseColor = clr(90, 35, 35);
    blight.mFogDayColor = clr(128, 19, 19);
    blight.mFogSunsetColor = clr(92, 33, 33);
    blight.mFogNightColor = clr(44, 14, 14);
    blight.mAmbientSunriseColor = clr(61, 40, 40);
    blight.mAmbientDayColor = clr(79, 54, 54);
    blight.mAmbientSunsetColor = clr(61, 40, 40);
    blight.mAmbientNightColor = clr(56, 58, 62);
    blight.mSunSunriseColor = clr(180, 78, 78);
    blight.mSunDayColor = clr(224, 84, 84);
    blight.mSunSunsetColor = clr(180, 78, 78);
    blight.mSunNightColor = clr(61, 91, 143);
    blight.mSunDiscSunsetColor = clr(128, 128, 128);
    blight.mLandFogDayDepth = 1.1;
    blight.mLandFogNightDepth = 1.2;
    blight.mWindSpeed = 0.9;
    blight.mCloudSpeed = 9;
    blight.mGlareView = 0;
    blight.mAmbientLoopSoundID = "blight";
    mWeatherSettings["blight"] = blight;

    /*
    Weather snow;
    snow.mCloudTexture = "tx_bm_sky_snow.dds";
    snow.mCloudsMaximumPercent = 1.0;
    snow.mTransitionDelta = 0.014;
    snow.mSkySunriseColor = clr(196, 91, 91);
    snow.mSkyDayColor = clr(153, 158, 166);
    snow.mSkySunsetColor = clr(96, 115, 134);
    snow.mSkyNightColor = clr(31, 35, 39);
    snow.mFogSunriseColor = clr(106, 91, 91);
    snow.mFogDayColor = clr(153, 158, 166);
    snow.mFogSunsetColor = clr(96, 115, 134);
    snow.mFogNightColor = clr(31, 35, 39);
    snow.mAmbientSunriseColor = clr(92, 84, 84);
    snow.mAmbientDayColor = clr(93, 96, 105);
    snow.mAmbientSunsetColor = clr(70, 79, 87);
    snow.mAmbientNightColor = clr(49, 58, 68);
    snow.mSunSunriseColor = clr(141, 109, 109);
    snow.mSunDayColor = clr(163, 169, 183);
    snow.mSunSunsetColor = clr(101, 121, 141);
    snow.mSunNightColor = clr(55, 66, 77);
    snow.mSunDiscSunsetColor = clr(128, 128, 128);
    snow.mLandFogDayDepth = 1.0;
    snow.mLandFogNightDepth = 1.2;
    snow.mWindSpeed = 0;
    snow.mCloudSpeed = 1.5;
    snow.mGlareView = 0;
    mWeatherSettings["snow"] = snow;

    Weather blizzard;
    blizzard.mCloudTexture = "tx_bm_sky_blizzard.dds";
    blizzard.mCloudsMaximumPercent = 1.0;
    blizzard.mTransitionDelta = 0.030;
    blizzard.mSkySunriseColor = clr(91, 99, 106);
    blizzard.mSkyDayColor = clr(121, 133, 145);
    blizzard.mSkySunsetColor = clr(108, 115, 121);
    blizzard.mSkyNightColor = clr(27, 29, 31);
    blizzard.mFogSunriseColor = clr(91, 99, 106);
    blizzard.mFogDayColor = clr(121, 133, 145);
    blizzard.mFogSunsetColor = clr(108, 115, 121);
    blizzard.mFogNightColor = clr(21, 24, 28);
    blizzard.mAmbientSunriseColor = clr(84, 88, 92);
    blizzard.mAmbientDayColor = clr(93, 96, 105);
    blizzard.mAmbientSunsetColor = clr(83, 77, 75);
    blizzard.mAmbientNightColor = clr(53, 62, 70);
    blizzard.mSunSunriseColor = clr(114, 128, 146);
    blizzard.mSunDayColor = clr(163, 169, 183);
    blizzard.mSunSunsetColor = clr(106, 114, 136);
    blizzard.mSunNightColor = clr(57, 66, 74);
    blizzard.mSunDiscSunsetColor = clr(128, 128, 128);
    blizzard.mLandFogDayDepth = 2.8;
    blizzard.mLandFogNightDepth = 3.0;
    blizzard.mWindSpeed = 0.9;
    blizzard.mCloudSpeed = 7.5;
    blizzard.mGlareView = 0;
    blizzard.mAmbientLoopSoundID = "BM Blizzard";
    mWeatherSettings["blizzard"] = blizzard;
    */
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
            result.mFogColor = lerp(current.mFogSunriseColor, current.mFogNightColor);
            result.mAmbientColor = lerp(current.mAmbientSunriseColor, current.mAmbientNightColor);
            result.mSunColor = lerp(current.mSunSunriseColor, current.mSunNightColor);
            result.mSkyColor = lerp(current.mSkySunriseColor, current.mSkyNightColor);
            result.mNightFade = factor;
        }
        else //if (mHour >= 6)
        {
            // fade out
            float advance = mHour-6;
            float factor = advance / 3.f;
            result.mFogColor = lerp(current.mFogSunriseColor, current.mFogDayColor);
            result.mAmbientColor = lerp(current.mAmbientSunriseColor, current.mAmbientDayColor);
            result.mSunColor = lerp(current.mSunSunriseColor, current.mSunDayColor);
            result.mSkyColor = lerp(current.mSkySunriseColor, current.mSkyDayColor);
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
            result.mFogColor = lerp(current.mFogSunsetColor, current.mFogDayColor);
            result.mAmbientColor = lerp(current.mAmbientSunsetColor, current.mAmbientDayColor);
            result.mSunColor = lerp(current.mSunSunsetColor, current.mSunDayColor);
            result.mSkyColor = lerp(current.mSkySunsetColor, current.mSkyDayColor);
        }
        else //if (mHour >= 19)
        {
            // fade out
            float advance = mHour-19;
            float factor = advance / 2.f;
            result.mFogColor = lerp(current.mFogSunsetColor, current.mFogNightColor);
            result.mAmbientColor = lerp(current.mAmbientSunsetColor, current.mAmbientNightColor);
            result.mSunColor = lerp(current.mSunSunsetColor, current.mSunNightColor);
            result.mSkyColor = lerp(current.mSkySunsetColor, current.mSkyNightColor);
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

    result.mCloudOpacity = lerp(current.mCloudOpacity, other.mCloudOpacity);
    result.mFogColor = lerp(current.mFogColor, other.mFogColor);
    result.mSunColor = lerp(current.mSunColor, other.mSunColor);
    result.mSkyColor = lerp(current.mSkyColor, other.mSkyColor);

    result.mAmbientColor = lerp(current.mAmbientColor, other.mAmbientColor);
    result.mSunDiscColor = lerp(current.mSunDiscColor, other.mSunDiscColor);
    result.mFogDepth = lerp(current.mFogDepth, other.mFogDepth);
    result.mWindSpeed = lerp(current.mWindSpeed, other.mWindSpeed);
    result.mCloudSpeed = lerp(current.mCloudSpeed, other.mCloudSpeed);
    result.mCloudOpacity = lerp(current.mCloudOpacity, other.mCloudOpacity);
    result.mGlareView = lerp(current.mGlareView, other.mGlareView);
    result.mNightFade = lerp(current.mNightFade, other.mNightFade);

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
            mWeatherUpdateTime = WeatherGlobals::mWeatherUpdateTime*3600;

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
                    //float snow = region->mData.a/255.f;
                    //float blizzard = region->mData.b/255.f;

                    // re-scale to 100 percent
                    const float total = clear+cloudy+foggy+overcast+rain+thunder+ash+blight;//+snow+blizzard;

                    float random = ((rand()%100)/100.f) * total;

                    //if (random >= snow+blight+ash+thunder+rain+overcast+foggy+cloudy+clear)
                    //    weather = "blizzard";
                    //else if (random >= blight+ash+thunder+rain+overcast+foggy+cloudy+clear)
                    //    weather = "snow";
                    /*else*/ if (random >= ash+thunder+rain+overcast+foggy+cloudy+clear)
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

            float hour_fade;
            if (mHour >= 7.f && mHour <= 14.f)
                hour_fade = 1-(mHour-7)/3.f;
            else if (mHour >= 14 && mHour <= 15.f)
                hour_fade = mHour-14;
            else
                hour_fade = 1;

            float secunda_angle_fade;
            float masser_angle_fade;
            float angle = moonHeight*90.f;

            if (angle >= 30 && angle <= 50)
                secunda_angle_fade = (angle-30)/20.f;
            else if (angle <30)
                secunda_angle_fade = 0.f;
            else
                secunda_angle_fade = 1.f;

            if (angle >= 40 && angle <= 50)
                masser_angle_fade = (angle-40)/10.f;
            else if (angle <40)
                masser_angle_fade = 0.f;
            else
                masser_angle_fade = 1.f;

            masser_angle_fade *= hour_fade;
            secunda_angle_fade *= hour_fade;

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
                    if (sound == 0) soundname = WeatherGlobals::mThunderSoundID0;
                    else if (sound == 1) soundname = WeatherGlobals::mThunderSoundID1;
                    else if (sound == 2) soundname = WeatherGlobals::mThunderSoundID2;
                    else if (sound == 3) soundname = WeatherGlobals::mThunderSoundID3;
                    MWBase::Environment::get().getSoundManager()->playSound(soundname, 1.0, 1.0);
                    mThunderSoundDelay = 1000;
                }

                mThunderFlash -= duration;
                if (mThunderFlash > 0)
                    mRendering->getSkyManager()->setLightningStrength( mThunderFlash / WeatherGlobals::mThunderThreshold );
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
                    mThunderFlash = WeatherGlobals::mThunderThreshold;

                    mRendering->getSkyManager()->setLightningStrength( mThunderFlash / WeatherGlobals::mThunderThreshold );

                    mThunderSoundDelay = WeatherGlobals::mThunderSoundDelay;
                }
            }
        }
        else
            mRendering->getSkyManager()->setLightningStrength(0.f);

        mRendering->setAmbientColour(result.mAmbientColor);
        mRendering->sunEnable();
        mRendering->setSunColour(result.mSunColor);

        mRendering->getSkyManager()->setWeather(result);
    }
    else
    {
        mRendering->sunDisable();
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
