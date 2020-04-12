#include "weather.hpp"

#include <components/misc/rng.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/weatherstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwsound/sound.hpp"

#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/sky.hpp"

#include "player.hpp"
#include "esmstore.hpp"
#include "cellstore.hpp"

#include <cmath>

using namespace MWWorld;

namespace
{
    static const int invalidWeatherID = -1;

    // linear interpolate between x and y based on factor.
    float lerp (float x, float y, float factor)
    {
        return x * (1-factor) + y * factor;
    }
    // linear interpolate between x and y based on factor.
    osg::Vec4f lerp (const osg::Vec4f& x, const osg::Vec4f& y, float factor)
    {
        return x * (1-factor) + y * factor;
    }
}

template <typename T>
T TimeOfDayInterpolator<T>::getValue(const float gameHour, const TimeOfDaySettings& timeSettings, const std::string& prefix) const
{
    WeatherSetting setting = timeSettings.getSetting(prefix);
    float preSunriseTime = setting.mPreSunriseTime;
    float postSunriseTime = setting.mPostSunriseTime;
    float preSunsetTime = setting.mPreSunsetTime;
    float postSunsetTime = setting.mPostSunsetTime;

    // night
    if (gameHour < timeSettings.mNightEnd - preSunriseTime || gameHour > timeSettings.mNightStart + postSunsetTime)
        return mNightValue;
    // sunrise
    else if (gameHour >= timeSettings.mNightEnd - preSunriseTime && gameHour <= timeSettings.mDayStart + postSunriseTime)
    {
        float duration = timeSettings.mDayStart + postSunriseTime - timeSettings.mNightEnd + preSunriseTime;
        float middle = timeSettings.mNightEnd - preSunriseTime + duration / 2.f;

        if (gameHour <= middle)
        {
            // fade in
            float advance = middle - gameHour;
            float factor = 0.f;
            if (duration > 0)
                factor = advance / duration * 2;
            return lerp(mSunriseValue, mNightValue, factor);
        }
        else
        {
            // fade out
            float advance = gameHour - middle;
            float factor = 1.f;
            if (duration > 0)
                factor = advance / duration * 2;
            return lerp(mSunriseValue, mDayValue, factor);
        }
    }
    // day
    else if (gameHour > timeSettings.mDayStart + postSunriseTime && gameHour < timeSettings.mDayEnd - preSunsetTime)
        return mDayValue;
    // sunset
    else if (gameHour >= timeSettings.mDayEnd - preSunsetTime && gameHour <= timeSettings.mNightStart + postSunsetTime)
    {
        float duration = timeSettings.mNightStart + postSunsetTime - timeSettings.mDayEnd + preSunsetTime;
        float middle = timeSettings.mDayEnd - preSunsetTime + duration / 2.f;

        if (gameHour <= middle)
        {
            // fade in
            float advance = middle - gameHour;
            float factor = 0.f;
            if (duration > 0)
                factor = advance / duration * 2;
            return lerp(mSunsetValue, mDayValue, factor);
        }
        else
        {
            // fade out
            float advance = gameHour - middle;
            float factor = 1.f;
            if (duration > 0)
                factor = advance / duration * 2;
            return lerp(mSunsetValue, mNightValue, factor);
        }
    }
    // shut up compiler
    return T();
}



template class MWWorld::TimeOfDayInterpolator<float>;
template class MWWorld::TimeOfDayInterpolator<osg::Vec4f>;

Weather::Weather(const std::string& name,
                 float stormWindSpeed,
                 float rainSpeed,
                 float dlFactor,
                 float dlOffset,
                 const std::string& particleEffect)
    : mCloudTexture(Fallback::Map::getString("Weather_" + name + "_Cloud_Texture"))
    , mSkyColor(Fallback::Map::getColour("Weather_" + name +"_Sky_Sunrise_Color"),
                Fallback::Map::getColour("Weather_" + name + "_Sky_Day_Color"),
                Fallback::Map::getColour("Weather_" + name + "_Sky_Sunset_Color"),
                Fallback::Map::getColour("Weather_" + name + "_Sky_Night_Color"))
    , mFogColor(Fallback::Map::getColour("Weather_" + name + "_Fog_Sunrise_Color"),
                Fallback::Map::getColour("Weather_" + name + "_Fog_Day_Color"),
                Fallback::Map::getColour("Weather_" + name + "_Fog_Sunset_Color"),
                Fallback::Map::getColour("Weather_" + name + "_Fog_Night_Color"))
    , mAmbientColor(Fallback::Map::getColour("Weather_" + name + "_Ambient_Sunrise_Color"),
                    Fallback::Map::getColour("Weather_" + name + "_Ambient_Day_Color"),
                    Fallback::Map::getColour("Weather_" + name + "_Ambient_Sunset_Color"),
                    Fallback::Map::getColour("Weather_" + name + "_Ambient_Night_Color"))
    , mSunColor(Fallback::Map::getColour("Weather_" + name + "_Sun_Sunrise_Color"),
                Fallback::Map::getColour("Weather_" + name + "_Sun_Day_Color"),
                Fallback::Map::getColour("Weather_" + name + "_Sun_Sunset_Color"),
                Fallback::Map::getColour("Weather_" + name + "_Sun_Night_Color"))
    , mLandFogDepth(Fallback::Map::getFloat("Weather_" + name + "_Land_Fog_Day_Depth"),
                    Fallback::Map::getFloat("Weather_" + name + "_Land_Fog_Day_Depth"),
                    Fallback::Map::getFloat("Weather_" + name + "_Land_Fog_Day_Depth"),
                    Fallback::Map::getFloat("Weather_" + name + "_Land_Fog_Night_Depth"))
    , mSunDiscSunsetColor(Fallback::Map::getColour("Weather_" + name + "_Sun_Disc_Sunset_Color"))
    , mWindSpeed(Fallback::Map::getFloat("Weather_" + name + "_Wind_Speed"))
    , mCloudSpeed(Fallback::Map::getFloat("Weather_" + name + "_Cloud_Speed"))
    , mGlareView(Fallback::Map::getFloat("Weather_" + name + "_Glare_View"))
    , mIsStorm(mWindSpeed > stormWindSpeed)
    , mRainSpeed(rainSpeed)
    , mRainEntranceSpeed(Fallback::Map::getFloat("Weather_" + name + "_Rain_Entrance_Speed"))
    , mRainMaxRaindrops(Fallback::Map::getFloat("Weather_" + name + "_Max_Raindrops"))
    , mRainDiameter(Fallback::Map::getFloat("Weather_" + name + "_Rain_Diameter"))
    , mRainThreshold(Fallback::Map::getFloat("Weather_" + name + "_Rain_Threshold"))
    , mRainMinHeight(Fallback::Map::getFloat("Weather_" + name + "_Rain_Height_Min"))
    , mRainMaxHeight(Fallback::Map::getFloat("Weather_" + name + "_Rain_Height_Max"))
    , mParticleEffect(particleEffect)
    , mRainEffect(Fallback::Map::getBool("Weather_" + name + "_Using_Precip") ? "meshes\\raindrop.nif" : "")
    , mTransitionDelta(Fallback::Map::getFloat("Weather_" + name + "_Transition_Delta"))
    , mCloudsMaximumPercent(Fallback::Map::getFloat("Weather_" + name + "_Clouds_Maximum_Percent"))
    , mThunderFrequency(Fallback::Map::getFloat("Weather_" + name + "_Thunder_Frequency"))
    , mThunderThreshold(Fallback::Map::getFloat("Weather_" + name + "_Thunder_Threshold"))
    , mThunderSoundID()
    , mFlashDecrement(Fallback::Map::getFloat("Weather_" + name + "_Flash_Decrement"))
    , mFlashBrightness(0.0f)
{
    mDL.FogFactor = dlFactor;
    mDL.FogOffset = dlOffset;
    mThunderSoundID[0] = Fallback::Map::getString("Weather_" + name + "_Thunder_Sound_ID_0");
    mThunderSoundID[1] = Fallback::Map::getString("Weather_" + name + "_Thunder_Sound_ID_1");
    mThunderSoundID[2] = Fallback::Map::getString("Weather_" + name + "_Thunder_Sound_ID_2");
    mThunderSoundID[3] = Fallback::Map::getString("Weather_" + name + "_Thunder_Sound_ID_3");

    // TODO: support weathers that have both "Ambient Loop Sound ID" and "Rain Loop Sound ID", need to play both sounds at the same time.

    if (!mRainEffect.empty()) // NOTE: in vanilla, the weathers with rain seem to be hardcoded; changing Using_Precip has no effect
    {
        mAmbientLoopSoundID = Fallback::Map::getString("Weather_" + name + "_Rain_Loop_Sound_ID");
        if (mAmbientLoopSoundID.empty()) // default to "rain" if not set
            mAmbientLoopSoundID = "rain";
    }
    else
        mAmbientLoopSoundID = Fallback::Map::getString("Weather_" + name + "_Ambient_Loop_Sound_ID");

    if (Misc::StringUtils::ciEqual(mAmbientLoopSoundID, "None"))
        mAmbientLoopSoundID.clear();
}

float Weather::transitionDelta() const
{
    // Transition Delta describes how quickly transitioning to the weather in question will take, in Hz. Note that the
    // measurement is in real time, not in-game time.
    return mTransitionDelta;
}

float Weather::cloudBlendFactor(const float transitionRatio) const
{
    // Clouds Maximum Percent affects how quickly the sky transitions from one sky texture to the next.
    return transitionRatio / mCloudsMaximumPercent;
}

float Weather::calculateThunder(const float transitionRatio, const float elapsedSeconds, const bool isPaused)
{
    // When paused, the flash brightness remains the same and no new strikes can occur.
    if(!isPaused)
    {
        // Morrowind doesn't appear to do any calculations unless the transition ratio is higher than the Thunder Threshold.
        if(transitionRatio >= mThunderThreshold && mThunderFrequency > 0.0f)
        {
            flashDecrement(elapsedSeconds);

            if(Misc::Rng::rollProbability() <= thunderChance(transitionRatio, elapsedSeconds))
            {
                lightningAndThunder();
            }
        }
        else
        {
            mFlashBrightness = 0.0f;
        }
    }

    return mFlashBrightness;
}

inline void Weather::flashDecrement(const float elapsedSeconds)
{
   // The Flash Decrement is measured in whole units per second. This means that if the flash brightness was
   // currently 1.0, then it should take approximately 0.25 seconds to decay to 0.0 (the minimum).
   float decrement = mFlashDecrement * elapsedSeconds;
   mFlashBrightness = decrement > mFlashBrightness ? 0.0f : mFlashBrightness - decrement;
}

inline float Weather::thunderChance(const float transitionRatio, const float elapsedSeconds) const
{
   // This formula is reversed from the observation that with Thunder Frequency set to 1, there are roughly 10 strikes
   // per minute. It doesn't appear to be tied to in game time as Timescale doesn't affect it. Various values of
   // Thunder Frequency seem to change the average number of strikes in a linear fashion.. During a transition, it appears to
   // scaled based on how far past it is past the Thunder Threshold.
   float scaleFactor = (transitionRatio - mThunderThreshold) / (1.0f - mThunderThreshold);
   return ((mThunderFrequency * 10.0f) / 60.0f) * elapsedSeconds * scaleFactor;
}

inline void Weather::lightningAndThunder(void)
{
    // Morrowind seems to vary the intensity of the brightness based on which of the four sound IDs it selects.
    // They appear to go from 0 (brightest, closest) to 3 (faintest, farthest). The value of 0.25 per distance
    // was derived by setting the Flash Decrement to 0.1 and measuring how long each value took to decay to 0.
    // TODO: Determine the distribution of each distance to see if it's evenly weighted.
    unsigned int distance = Misc::Rng::rollDice(4);
    // Flash brightness appears additive, since if multiple strikes occur, it takes longer for it to decay to 0.
    mFlashBrightness += 1 - (distance * 0.25f);
    MWBase::Environment::get().getSoundManager()->playSound(mThunderSoundID[distance], 1.0, 1.0);
}

RegionWeather::RegionWeather(const ESM::Region& region)
    : mWeather(invalidWeatherID)
    , mChances()
{
    mChances.reserve(10);
    mChances.push_back(region.mData.mClear);
    mChances.push_back(region.mData.mCloudy);
    mChances.push_back(region.mData.mFoggy);
    mChances.push_back(region.mData.mOvercast);
    mChances.push_back(region.mData.mRain);
    mChances.push_back(region.mData.mThunder);
    mChances.push_back(region.mData.mAsh);
    mChances.push_back(region.mData.mBlight);
    mChances.push_back(region.mData.mA);
    mChances.push_back(region.mData.mB);
}

RegionWeather::RegionWeather(const ESM::RegionWeatherState& state)
    : mWeather(state.mWeather)
    , mChances(state.mChances)
{
}

RegionWeather::operator ESM::RegionWeatherState() const
{
    ESM::RegionWeatherState state =
        {
            mWeather,
            mChances
        };

    return state;
}

void RegionWeather::setChances(const std::vector<char>& chances)
{
    if(mChances.size() < chances.size())
    {
        mChances.reserve(chances.size());
    }

    int i = 0;
    for(char chance : chances)
    {
        mChances[i] = chance;
        i++;
    }

    // Regional weather no longer supports the current type, select a new weather pattern.
    if((static_cast<size_t>(mWeather) >= mChances.size()) || (mChances[mWeather] == 0))
    {
        chooseNewWeather();
    }
}

void RegionWeather::setWeather(int weatherID)
{
    mWeather = weatherID;
}

int RegionWeather::getWeather()
{
    // If the region weather was already set (by ChangeWeather, or by a previous call) then just return that value.
    // Note that the region weather will be expired periodically when the weather update timer expires.
    if(mWeather == invalidWeatherID)
    {
        chooseNewWeather();
    }

    return mWeather;
}

void RegionWeather::chooseNewWeather()
{
    // All probabilities must add to 100 (responsibility of the user).
    // If chances A and B has values 30 and 70 then by generating 100 numbers 1..100, 30% will be lesser or equal 30
    // and 70% will be greater than 30 (in theory).
    int chance = Misc::Rng::rollDice(100) + 1; // 1..100
    int sum = 0;
    int i = 0;
    for(; static_cast<size_t>(i) < mChances.size(); ++i)
    {
        sum += mChances[i];
        if(chance <= sum)
        {
            mWeather = i;
            return;
        }
    }

    // if we hit this path then the chances don't add to 100, choose a default weather instead
    mWeather = 0;
}

MoonModel::MoonModel(const std::string& name)
  : mFadeInStart(Fallback::Map::getFloat("Moons_" + name + "_Fade_In_Start"))
  , mFadeInFinish(Fallback::Map::getFloat("Moons_" + name + "_Fade_In_Finish"))
  , mFadeOutStart(Fallback::Map::getFloat("Moons_" + name + "_Fade_Out_Start"))
  , mFadeOutFinish(Fallback::Map::getFloat("Moons_" + name + "_Fade_Out_Finish"))
  , mAxisOffset(Fallback::Map::getFloat("Moons_" + name + "_Axis_Offset"))
  , mSpeed(Fallback::Map::getFloat("Moons_" + name + "_Speed"))
  , mDailyIncrement(Fallback::Map::getFloat("Moons_" + name + "_Daily_Increment"))
  , mFadeStartAngle(Fallback::Map::getFloat("Moons_" + name + "_Fade_Start_Angle"))
  , mFadeEndAngle(Fallback::Map::getFloat("Moons_" + name + "_Fade_End_Angle"))
  , mMoonShadowEarlyFadeAngle(Fallback::Map::getFloat("Moons_" + name + "_Moon_Shadow_Early_Fade_Angle"))
{
    // Morrowind appears to have a minimum speed in order to avoid situations where the moon couldn't conceivably
    // complete a rotation in a single 24 hour period. The value of 180/23 was deduced from reverse engineering.
    mSpeed = std::min(mSpeed, 180.0f / 23.0f);
}

MWRender::MoonState MoonModel::calculateState(const TimeStamp& gameTime) const
{
    float rotationFromHorizon = angle(gameTime);
    MWRender::MoonState state =
        {
            rotationFromHorizon,
            mAxisOffset, // Reverse engineered from Morrowind's scene graph rotation matrices.
            static_cast<MWRender::MoonState::Phase>(phase(gameTime)),
            shadowBlend(rotationFromHorizon),
            earlyMoonShadowAlpha(rotationFromHorizon) * hourlyAlpha(gameTime.getHour())
        };

    return state;
}

inline float MoonModel::angle(const TimeStamp& gameTime) const
{
    // Morrowind's moons start travel on one side of the horizon (let's call it H-rise) and travel 180 degrees to the
    // opposite horizon (let's call it H-set). Upon reaching H-set, they reset to H-rise until the next moon rise.

    // When calculating the angle of the moon, several cases have to be taken into account:
    // 1. Moon rises and then sets in one day.
    // 2. Moon sets and doesn't rise in one day (occurs when the moon rise hour is >= 24).
    // 3. Moon sets and then rises in one day.
    float moonRiseHourToday = moonRiseHour(gameTime.getDay());
    float moonRiseAngleToday = 0;

    if(gameTime.getHour() < moonRiseHourToday)
    {
        float moonRiseHourYesterday = moonRiseHour(gameTime.getDay() - 1);
        if(moonRiseHourYesterday < 24)
        {
            float moonRiseAngleYesterday = rotation(24 - moonRiseHourYesterday);
            if(moonRiseAngleYesterday < 180)
            {
                // The moon rose but did not set yesterday, so accumulate yesterday's angle with how much we've travelled today.
                moonRiseAngleToday = rotation(gameTime.getHour()) + moonRiseAngleYesterday;
            }
        }
    }
    else
    {
        moonRiseAngleToday = rotation(gameTime.getHour() - moonRiseHourToday);
    }

    if(moonRiseAngleToday >= 180)
    {
        // The moon set today, reset the angle to the horizon.
        moonRiseAngleToday = 0;
    }

    return moonRiseAngleToday;
}

inline float MoonModel::moonRiseHour(unsigned int daysPassed) const
{
    // This arises from the start date of 16 Last Seed, 427
    // TODO: Find an alternate formula that doesn't rely on this day being fixed.
    static const unsigned int startDay = 16;

    // This odd formula arises from the fact that on 16 Last Seed, 17 increments have occurred, meaning
    // that upon starting a new game, it must only calculate the moon phase as far back as 1 Last Seed.
    // Note that we don't modulo after adding the latest daily increment because other calculations need to
    // know if doing so would cause the moon rise to be postponed until the next day (which happens when
    // the moon rise hour is >= 24 in Morrowind).
    return mDailyIncrement + std::fmod((daysPassed - 1 + startDay) * mDailyIncrement, 24.0f);
}

inline float MoonModel::rotation(float hours) const
{
    // 15 degrees per hour was reverse engineered from the rotation matrices of the Morrowind scene graph.
    // Note that this correlates to 360 / 24, which is a full rotation every 24 hours, so speed is a measure
    // of whole rotations that could be completed in a day.
    return 15.0f * mSpeed * hours;
}

inline unsigned int MoonModel::phase(const TimeStamp& gameTime) const
{
    // Morrowind starts with a full moon on 16 Last Seed and then begins to wane 17 Last Seed, working on 3 day phase cycle.
    // Note: this is an internal helper, and as such we don't want to return MWRender::MoonState::Phase since we can't
    // forward declare it (C++11 strongly typed enums solve this).

    // If the moon didn't rise yet today, use yesterday's moon phase.
    if(gameTime.getHour() < moonRiseHour(gameTime.getDay()))
        return (gameTime.getDay() / 3) % 8;
    else
        return ((gameTime.getDay() + 1) / 3) % 8;
}

inline float MoonModel::shadowBlend(float angle) const
{
    // The Fade End Angle and Fade Start Angle describe a region where the moon transitions from a solid disk
    // that is roughly the color of the sky, to a textured surface.
    // Depending on the current angle, the following values describe the ratio between the textured moon
    // and the solid disk:
    // 1. From Fade End Angle 1 to Fade Start Angle 1 (during moon rise): 0..1
    // 2. From Fade Start Angle 1 to Fade Start Angle 2 (between moon rise and moon set): 1 (textured)
    // 3. From Fade Start Angle 2 to Fade End Angle 2 (during moon set): 1..0
    // 4. From Fade End Angle 2 to Fade End Angle 1 (between moon set and moon rise): 0 (solid disk)
    float fadeAngle = mFadeStartAngle - mFadeEndAngle;
    float fadeEndAngle2 = 180.0f - mFadeEndAngle;
    float fadeStartAngle2 = 180.0f - mFadeStartAngle;
    if((angle >= mFadeEndAngle) && (angle < mFadeStartAngle))
        return (angle - mFadeEndAngle) / fadeAngle;
    else if((angle >= mFadeStartAngle) && (angle < fadeStartAngle2))
        return 1.0f;
    else if((angle >= fadeStartAngle2) && (angle < fadeEndAngle2))
        return (fadeEndAngle2 - angle) / fadeAngle;
    else
        return 0.0f;
}

inline float MoonModel::hourlyAlpha(float gameHour) const
{
    // The Fade Out Start / Finish and Fade In Start / Finish describe the hours at which the moon
    // appears and disappears.
    // Depending on the current hour, the following values describe how transparent the moon is.
    // 1. From Fade Out Start to Fade Out Finish: 1..0
    // 2. From Fade Out Finish to Fade In Start: 0 (transparent)
    // 3. From Fade In Start to Fade In Finish: 0..1
    // 4. From Fade In Finish to Fade Out Start: 1 (solid)
    if((gameHour >= mFadeOutStart) && (gameHour < mFadeOutFinish))
        return (mFadeOutFinish - gameHour) / (mFadeOutFinish - mFadeOutStart);
    else if((gameHour >= mFadeOutFinish) && (gameHour < mFadeInStart))
        return 0.0f;
    else if((gameHour >= mFadeInStart) && (gameHour < mFadeInFinish))
        return (gameHour - mFadeInStart) / (mFadeInFinish - mFadeInStart);
    else
        return 1.0f;
}

inline float MoonModel::earlyMoonShadowAlpha(float angle) const
{
    // The Moon Shadow Early Fade Angle describes an arc relative to Fade End Angle.
    // Depending on the current angle, the following values describe how transparent the moon is.
    // 1. From Moon Shadow Early Fade Angle 1 to Fade End Angle 1 (during moon rise): 0..1
    // 2. From Fade End Angle 1 to Fade End Angle 2 (between moon rise and moon set): 1 (solid)
    // 3. From Fade End Angle 2 to Moon Shadow Early Fade Angle 2 (during moon set): 1..0
    // 4. From Moon Shadow Early Fade Angle 2 to Moon Shadow Early Fade Angle 1: 0 (transparent)
    float moonShadowEarlyFadeAngle1 = mFadeEndAngle - mMoonShadowEarlyFadeAngle;
    float fadeEndAngle2 = 180.0f - mFadeEndAngle;
    float moonShadowEarlyFadeAngle2 = fadeEndAngle2 + mMoonShadowEarlyFadeAngle;
    if((angle >= moonShadowEarlyFadeAngle1) && (angle < mFadeEndAngle))
        return (angle - moonShadowEarlyFadeAngle1) / mMoonShadowEarlyFadeAngle;
    else if((angle >= mFadeEndAngle) && (angle < fadeEndAngle2))
        return 1.0f;
    else if((angle >= fadeEndAngle2) && (angle < moonShadowEarlyFadeAngle2))
        return (moonShadowEarlyFadeAngle2 - angle) / mMoonShadowEarlyFadeAngle;
    else
        return 0.0f;
}

WeatherManager::WeatherManager(MWRender::RenderingManager& rendering, MWWorld::ESMStore& store)
    : mStore(store)
    , mRendering(rendering)
    , mSunriseTime(Fallback::Map::getFloat("Weather_Sunrise_Time"))
    , mSunsetTime(Fallback::Map::getFloat("Weather_Sunset_Time"))
    , mSunriseDuration(Fallback::Map::getFloat("Weather_Sunrise_Duration"))
    , mSunsetDuration(Fallback::Map::getFloat("Weather_Sunset_Duration"))
    , mSunPreSunsetTime(Fallback::Map::getFloat("Weather_Sun_Pre-Sunset_Time"))
    , mNightFade(0, 0, 0, 1)
    , mHoursBetweenWeatherChanges(Fallback::Map::getFloat("Weather_Hours_Between_Weather_Changes"))
    , mRainSpeed(Fallback::Map::getFloat("Weather_Precip_Gravity"))
    , mUnderwaterFog(Fallback::Map::getFloat("Water_UnderwaterSunriseFog"),
                    Fallback::Map::getFloat("Water_UnderwaterDayFog"),
                    Fallback::Map::getFloat("Water_UnderwaterSunsetFog"),
                    Fallback::Map::getFloat("Water_UnderwaterNightFog"))
    , mWeatherSettings()
    , mMasser("Masser")
    , mSecunda("Secunda")
    , mWindSpeed(0.f)
    , mCurrentWindSpeed(0.f)
    , mNextWindSpeed(0.f)
    , mIsStorm(false)
    , mPrecipitation(false)
    , mStormDirection(0,1,0)
    , mCurrentRegion()
    , mTimePassed(0)
    , mFastForward(false)
    , mWeatherUpdateTime(mHoursBetweenWeatherChanges)
    , mTransitionFactor(0)
    , mNightDayMode(Default)
    , mCurrentWeather(0)
    , mNextWeather(0)
    , mQueuedWeather(0)
    , mRegions()
    , mResult()
    , mAmbientSound(nullptr)
    , mPlayingSoundID()
{
    mTimeSettings.mNightStart = mSunsetTime + mSunsetDuration;
    mTimeSettings.mNightEnd = mSunriseTime;
    mTimeSettings.mDayStart = mSunriseTime + mSunriseDuration;
    mTimeSettings.mDayEnd = mSunsetTime;

    mTimeSettings.addSetting("Sky");
    mTimeSettings.addSetting("Ambient");
    mTimeSettings.addSetting("Fog");
    mTimeSettings.addSetting("Sun");

    // Morrowind handles stars settings differently for other ones
    mTimeSettings.mStarsPostSunsetStart = Fallback::Map::getFloat("Weather_Stars_Post-Sunset_Start");
    mTimeSettings.mStarsPreSunriseFinish = Fallback::Map::getFloat("Weather_Stars_Pre-Sunrise_Finish");
    mTimeSettings.mStarsFadingDuration = Fallback::Map::getFloat("Weather_Stars_Fading_Duration");

    WeatherSetting starSetting = {
        mTimeSettings.mStarsPreSunriseFinish,
        mTimeSettings.mStarsFadingDuration - mTimeSettings.mStarsPreSunriseFinish,
        mTimeSettings.mStarsPostSunsetStart,
        mTimeSettings.mStarsFadingDuration - mTimeSettings.mStarsPostSunsetStart
    };

    mTimeSettings.mSunriseTransitions["Stars"] = starSetting;

    mWeatherSettings.reserve(10);
    // These distant land fog factor and offset values are the defaults MGE XE provides. Should be
    // provided by settings somewhere?
    addWeather("Clear", 1.0f, 0.0f); // 0
    addWeather("Cloudy", 0.9f, 0.0f); // 1
    addWeather("Foggy", 0.2f, 30.0f); // 2
    addWeather("Overcast", 0.7f, 0.0f); // 3
    addWeather("Rain", 0.5f, 10.0f); // 4
    addWeather("Thunderstorm", 0.5f, 20.0f); // 5
    addWeather("Ashstorm", 0.2f, 50.0f, "meshes\\ashcloud.nif"); // 6
    addWeather("Blight", 0.2f, 60.0f, "meshes\\blightcloud.nif"); // 7
    addWeather("Snow", 0.5f, 40.0f, "meshes\\snow.nif"); // 8
    addWeather("Blizzard", 0.16f, 70.0f, "meshes\\blizzard.nif"); // 9

    Store<ESM::Region>::iterator it = store.get<ESM::Region>().begin();
    for(; it != store.get<ESM::Region>().end(); ++it)
    {
        std::string regionID = Misc::StringUtils::lowerCase(it->mId);
        mRegions.insert(std::make_pair(regionID, RegionWeather(*it)));
    }

    forceWeather(0);
}

WeatherManager::~WeatherManager()
{
    stopSounds();
}

void WeatherManager::changeWeather(const std::string& regionID, const unsigned int weatherID)
{
    // In Morrowind, this seems to have the following behavior, when applied to the current region:
    // - When there is no transition in progress, start transitioning to the new weather.
    // - If there is a transition in progress, queue up the transition and process it when the current one completes.
    // - If there is a transition in progress, and a queued transition, overwrite the queued transition.
    // - If multiple calls to ChangeWeather are made while paused (console up), only the last call will be used,
    //   meaning that if there was no transition in progress, only the last ChangeWeather will be processed.
    // If the region isn't current, Morrowind will store the new weather for the region in question.

    if(weatherID < mWeatherSettings.size())
    {
        std::string lowerCaseRegionID = Misc::StringUtils::lowerCase(regionID);
        std::map<std::string, RegionWeather>::iterator it = mRegions.find(lowerCaseRegionID);
        if(it != mRegions.end())
        {
            it->second.setWeather(weatherID);
            regionalWeatherChanged(it->first, it->second);
        }
    }
}

void WeatherManager::modRegion(const std::string& regionID, const std::vector<char>& chances)
{
    // Sets the region's probability for various weather patterns. Note that this appears to be saved permanently.
    // In Morrowind, this seems to have the following behavior when applied to the current region:
    // - If the region supports the current weather, no change in current weather occurs.
    // - If the region no longer supports the current weather, and there is no transition in progress, begin to
    //   transition to a new supported weather type.
    // - If the region no longer supports the current weather, and there is a transition in progress, queue a
    //   transition to a new supported weather type.

    std::string lowerCaseRegionID = Misc::StringUtils::lowerCase(regionID);
    std::map<std::string, RegionWeather>::iterator it = mRegions.find(lowerCaseRegionID);
    if(it != mRegions.end())
    {
        it->second.setChances(chances);
        regionalWeatherChanged(it->first, it->second);
    }
}

void WeatherManager::playerTeleported(const std::string& playerRegion, bool isExterior)
{
    // If the player teleports to an outdoors cell in a new region (for instance, by travelling), the weather needs to
    // be changed immediately, and any transitions for the previous region discarded.
    {
        std::map<std::string, RegionWeather>::iterator it = mRegions.find(playerRegion);
        if(it != mRegions.end() && playerRegion != mCurrentRegion)
        {
            mCurrentRegion = playerRegion;
            forceWeather(it->second.getWeather());
        }
    }
}

float WeatherManager::calculateWindSpeed(int weatherId, float currentSpeed)
{
    float targetSpeed = std::min(8.0f * mWeatherSettings[weatherId].mWindSpeed, 70.f);
    if (currentSpeed == 0.f)
        currentSpeed = targetSpeed;

    float multiplier = mWeatherSettings[weatherId].mRainEffect.empty() ? 1.f : 0.5f;
    float updatedSpeed = (Misc::Rng::rollClosedProbability() - 0.5f) * multiplier * targetSpeed + currentSpeed;

    if (updatedSpeed > 0.5f * targetSpeed && updatedSpeed < 2.f * targetSpeed)
        currentSpeed = updatedSpeed;

    return currentSpeed;
}

void WeatherManager::update(float duration, bool paused, const TimeStamp& time, bool isExterior)
{
    MWWorld::ConstPtr player = MWMechanics::getPlayer();

    if(!paused || mFastForward)
    {
        // Add new transitions when either the player's current external region changes.
        std::string playerRegion = Misc::StringUtils::lowerCase(player.getCell()->getCell()->mRegion);
        if(updateWeatherTime() || updateWeatherRegion(playerRegion))
        {
            std::map<std::string, RegionWeather>::iterator it = mRegions.find(mCurrentRegion);
            if(it != mRegions.end())
            {
                addWeatherTransition(it->second.getWeather());
            }
        }

        updateWeatherTransitions(duration);
    }

    bool isDay = time.getHour() >= mSunriseTime && time.getHour() <= mTimeSettings.mNightStart;
    if (isExterior && !isDay)
        mNightDayMode = ExteriorNight;
    else if (!isExterior && isDay && mWeatherSettings[mCurrentWeather].mGlareView >= 0.5f)
        mNightDayMode = InteriorDay;
    else
        mNightDayMode = Default;

    if(!isExterior)
    {
        mRendering.setSkyEnabled(false);
        stopSounds();
        mWindSpeed = 0.f;
        mCurrentWindSpeed = 0.f;
        mNextWindSpeed = 0.f;
        return;
    }

    calculateWeatherResult(time.getHour(), duration, paused);

    if (!paused)
    {
        mWindSpeed = mResult.mWindSpeed;
        mCurrentWindSpeed = mResult.mCurrentWindSpeed;
        mNextWindSpeed = mResult.mNextWindSpeed;
    }

    mIsStorm = mResult.mIsStorm;

    // For some reason Ash Storm is not considered as a precipitation weather in game
    mPrecipitation = !(mResult.mParticleEffect.empty() && mResult.mRainEffect.empty())
                                    && mResult.mParticleEffect != "meshes\\ashcloud.nif";

    if (mIsStorm)
    {
        osg::Vec3f stormDirection(0, 1, 0);
        if (mResult.mParticleEffect == "meshes\\ashcloud.nif" || mResult.mParticleEffect == "meshes\\blightcloud.nif")
        {
            osg::Vec3f playerPos (MWMechanics::getPlayer().getRefData().getPosition().asVec3());
            playerPos.z() = 0;
            osg::Vec3f redMountainPos (25000, 70000, 0);
            stormDirection = (playerPos - redMountainPos);
            stormDirection.normalize();
        }
        mStormDirection = stormDirection;
        mRendering.getSkyManager()->setStormDirection(mStormDirection);
    }

    // disable sun during night
    if (time.getHour() >= mTimeSettings.mNightStart || time.getHour() <= mSunriseTime)
        mRendering.getSkyManager()->sunDisable();
    else
        mRendering.getSkyManager()->sunEnable();

    // Update the sun direction.  Run it east to west at a fixed angle from overhead.
    // The sun's speed at day and night may differ, since mSunriseTime and mNightStart
    // mark when the sun is level with the horizon.
    {
        // Shift times into a 24-hour window beginning at mSunriseTime...
        float adjustedHour = time.getHour();
        float adjustedNightStart = mTimeSettings.mNightStart;
        if ( time.getHour() < mSunriseTime )
            adjustedHour += 24.f;
        if ( mTimeSettings.mNightStart < mSunriseTime )
            adjustedNightStart += 24.f;

        const bool is_night = adjustedHour >= adjustedNightStart;
        const float dayDuration = adjustedNightStart - mSunriseTime;
        const float nightDuration = 24.f - dayDuration;

        double theta;
        if ( !is_night )
        {
            theta = static_cast<float>(osg::PI) * (adjustedHour - mSunriseTime) / dayDuration;
        }
        else
        {
            theta = static_cast<float>(osg::PI) - static_cast<float>(osg::PI) * (adjustedHour - adjustedNightStart) / nightDuration;
        }

        osg::Vec3f final(
            static_cast<float>(cos(theta)),
            -0.268f, // approx tan( -15 degrees )
            static_cast<float>(sin(theta)));
        mRendering.setSunDirection( final * -1 );
    }

    float underwaterFog = mUnderwaterFog.getValue(time.getHour(), mTimeSettings, "Fog");

    float peakHour = mSunriseTime + (mTimeSettings.mNightStart - mSunriseTime) / 2;
    float glareFade = 1.f;
    if (time.getHour() < mSunriseTime || time.getHour() > mTimeSettings.mNightStart)
        glareFade = 0.f;
    else if (time.getHour() < peakHour)
        glareFade = 1.f - (peakHour - time.getHour()) / (peakHour - mSunriseTime);
    else
        glareFade = 1.f - (time.getHour() - peakHour) / (mTimeSettings.mNightStart - peakHour);

    mRendering.getSkyManager()->setGlareTimeOfDayFade(glareFade);

    mRendering.getSkyManager()->setMasserState(mMasser.calculateState(time));
    mRendering.getSkyManager()->setSecundaState(mSecunda.calculateState(time));

    mRendering.configureFog(mResult.mFogDepth, underwaterFog, mResult.mDLFogFactor,
                            mResult.mDLFogOffset/100.0f, mResult.mFogColor);
    mRendering.setAmbientColour(mResult.mAmbientColor);
    mRendering.setSunColour(mResult.mSunColor, mResult.mSunColor * mResult.mGlareView * glareFade);

    mRendering.getSkyManager()->setWeather(mResult);

    // Play sounds
    if (mPlayingSoundID != mResult.mAmbientLoopSoundID)
    {
        stopSounds();
        if (!mResult.mAmbientLoopSoundID.empty())
            mAmbientSound = MWBase::Environment::get().getSoundManager()->playSound(
                mResult.mAmbientLoopSoundID, mResult.mAmbientSoundVolume, 1.0,
                MWSound::Type::Sfx, MWSound::PlayMode::Loop
            );
        mPlayingSoundID = mResult.mAmbientLoopSoundID;
    }
    else if (mAmbientSound)
        mAmbientSound->setVolume(mResult.mAmbientSoundVolume);
}

void WeatherManager::stopSounds()
{
    if (mAmbientSound)
        MWBase::Environment::get().getSoundManager()->stopSound(mAmbientSound);
    mAmbientSound = nullptr;
    mPlayingSoundID.clear();
}

float WeatherManager::getWindSpeed() const
{
    return mWindSpeed;
}

bool WeatherManager::isInStorm() const
{
    return mIsStorm;
}

osg::Vec3f WeatherManager::getStormDirection() const
{
    return mStormDirection;
}

void WeatherManager::advanceTime(double hours, bool incremental)
{
    // In Morrowind, when the player sleeps/waits, serves jail time, travels, or trains, all weather transitions are
    // immediately applied, regardless of whatever transition time might have been remaining.
    mTimePassed += hours;
    mFastForward = !incremental ? true : mFastForward;
}

unsigned int WeatherManager::getWeatherID() const
{
    return mCurrentWeather;
}

NightDayMode WeatherManager::getNightDayMode() const
{
    return mNightDayMode;
}

bool WeatherManager::useTorches(float hour) const
{
    bool isDark = hour < mSunriseTime || hour > mTimeSettings.mNightStart;

    return isDark && !mPrecipitation;
}

void WeatherManager::write(ESM::ESMWriter& writer, Loading::Listener& progress)
{
    ESM::WeatherState state;
    state.mCurrentRegion = mCurrentRegion;
    state.mTimePassed = mTimePassed;
    state.mFastForward = mFastForward;
    state.mWeatherUpdateTime = mWeatherUpdateTime;
    state.mTransitionFactor = mTransitionFactor;
    state.mCurrentWeather = mCurrentWeather;
    state.mNextWeather = mNextWeather;
    state.mQueuedWeather = mQueuedWeather;

    std::map<std::string, RegionWeather>::iterator it = mRegions.begin();
    for(; it != mRegions.end(); ++it)
    {
        state.mRegions.insert(std::make_pair(it->first, it->second));
    }

    writer.startRecord(ESM::REC_WTHR);
    state.save(writer);
    writer.endRecord(ESM::REC_WTHR);
}

bool WeatherManager::readRecord(ESM::ESMReader& reader, uint32_t type)
{
    if(ESM::REC_WTHR == type)
    {
        static const int oldestCompatibleSaveFormat = 2;
        if(reader.getFormat() < oldestCompatibleSaveFormat)
        {
            // Weather state isn't really all that important, so to preserve older save games, we'll just discard the
            // older weather records, rather than fail to handle the record.
            reader.skipRecord();
        }
        else
        {
            ESM::WeatherState state;
            state.load(reader);

            mCurrentRegion.swap(state.mCurrentRegion);
            mTimePassed = state.mTimePassed;
            mFastForward = state.mFastForward;
            mWeatherUpdateTime = state.mWeatherUpdateTime;
            mTransitionFactor = state.mTransitionFactor;
            mCurrentWeather = state.mCurrentWeather;
            mNextWeather = state.mNextWeather;
            mQueuedWeather = state.mQueuedWeather;

            mRegions.clear();
            importRegions();

            for(std::map<std::string, ESM::RegionWeatherState>::iterator it = state.mRegions.begin(); it != state.mRegions.end(); ++it)
            {
                std::map<std::string, RegionWeather>::iterator found = mRegions.find(it->first);
                if (found != mRegions.end())
                {
                    found->second = RegionWeather(it->second);
                }
            }
        }

        return true;
    }

    return false;
}

void WeatherManager::clear()
{
    stopSounds();

    mCurrentRegion = "";
    mTimePassed = 0.0f;
    mWeatherUpdateTime = 0.0f;
    forceWeather(0);
    mRegions.clear();
    importRegions();
}

inline void WeatherManager::addWeather(const std::string& name,
                                       float dlFactor, float dlOffset,
                                       const std::string& particleEffect)
{
    static const float fStromWindSpeed = mStore.get<ESM::GameSetting>().find("fStromWindSpeed")->mValue.getFloat();

    Weather weather(name, fStromWindSpeed, mRainSpeed, dlFactor, dlOffset, particleEffect);

    mWeatherSettings.push_back(weather);
}

inline void WeatherManager::importRegions()
{
    for(const ESM::Region& region : mStore.get<ESM::Region>())
    {
        std::string regionID = Misc::StringUtils::lowerCase(region.mId);
        mRegions.insert(std::make_pair(regionID, RegionWeather(region)));
    }
}

inline void WeatherManager::regionalWeatherChanged(const std::string& regionID, RegionWeather& region)
{
    // If the region is current, then add a weather transition for it.
    MWWorld::ConstPtr player = MWMechanics::getPlayer();
    if(player.isInCell())
    {
        if(Misc::StringUtils::ciEqual(regionID, mCurrentRegion))
        {
            addWeatherTransition(region.getWeather());
        }
    }
}

inline bool WeatherManager::updateWeatherTime()
{
    mWeatherUpdateTime -= mTimePassed;
    mTimePassed = 0.0f;
    if(mWeatherUpdateTime <= 0.0f)
    {
        // Expire all regional weather, so that any call to getWeather() will return a new weather ID.
        std::map<std::string, RegionWeather>::iterator it = mRegions.begin();
        for(; it != mRegions.end(); ++it)
        {
            it->second.setWeather(invalidWeatherID);
        }

        mWeatherUpdateTime += mHoursBetweenWeatherChanges;

        return true;
    }

    return false;
}

inline bool WeatherManager::updateWeatherRegion(const std::string& playerRegion)
{
    if(!playerRegion.empty() && playerRegion != mCurrentRegion)
    {
        mCurrentRegion = playerRegion;

        return true;
    }

    return false;
}

inline void WeatherManager::updateWeatherTransitions(const float elapsedRealSeconds)
{
    // When a player chooses to train, wait, or serves jail time, any transitions will be fast forwarded to the last
    // weather type set, regardless of the remaining transition time.
    if(!mFastForward && inTransition())
    {
        const float delta = mWeatherSettings[mNextWeather].transitionDelta();
        mTransitionFactor -= elapsedRealSeconds * delta;
        if(mTransitionFactor <= 0.0f)
        {
            mCurrentWeather = mNextWeather;
            mNextWeather = mQueuedWeather;
            mQueuedWeather = invalidWeatherID;

            // We may have begun processing the queued transition, so we need to apply the remaining time towards it.
            if(inTransition())
            {
                const float newDelta = mWeatherSettings[mNextWeather].transitionDelta();
                const float remainingSeconds = -(mTransitionFactor / delta);
                mTransitionFactor = 1.0f - (remainingSeconds * newDelta);
            }
            else
            {
                mTransitionFactor = 0.0f;
            }
        }
    }
    else
    {
        if(mQueuedWeather != invalidWeatherID)
        {
            mCurrentWeather = mQueuedWeather;
        }
        else if(mNextWeather != invalidWeatherID)
        {
            mCurrentWeather = mNextWeather;
        }

        mNextWeather = invalidWeatherID;
        mQueuedWeather = invalidWeatherID;
        mFastForward = false;
    }
}

inline void WeatherManager::forceWeather(const int weatherID)
{
    mTransitionFactor = 0.0f;
    mCurrentWeather = weatherID;
    mNextWeather = invalidWeatherID;
    mQueuedWeather = invalidWeatherID;
}

inline bool WeatherManager::inTransition()
{
    return mNextWeather != invalidWeatherID;
}

inline void WeatherManager::addWeatherTransition(const int weatherID)
{
    // In order to work like ChangeWeather expects, this method begins transitioning to the new weather immediately if
    // no transition is in progress, otherwise it queues it to be transitioned.

    assert(weatherID >= 0 && static_cast<size_t>(weatherID) < mWeatherSettings.size());

    if(!inTransition() && (weatherID != mCurrentWeather))
    {
        mNextWeather = weatherID;
        mTransitionFactor = 1.0f;
    }
    else if(inTransition() && (weatherID != mNextWeather))
    {
        mQueuedWeather = weatherID;
    }
}

inline void WeatherManager::calculateWeatherResult(const float gameHour,
                                                   const float elapsedSeconds,
                                                   const bool isPaused)
{
    float flash = 0.0f;
    if(!inTransition())
    {
        calculateResult(mCurrentWeather, gameHour);
        flash = mWeatherSettings[mCurrentWeather].calculateThunder(1.0f, elapsedSeconds, isPaused);
    }
    else
    {
        calculateTransitionResult(1 - mTransitionFactor, gameHour);
        float currentFlash = mWeatherSettings[mCurrentWeather].calculateThunder(mTransitionFactor,
                                                                                elapsedSeconds,
                                                                                isPaused);
        float nextFlash = mWeatherSettings[mNextWeather].calculateThunder(1 - mTransitionFactor,
                                                                          elapsedSeconds,
                                                                          isPaused);
        flash = currentFlash + nextFlash;
    }
    osg::Vec4f flashColor(flash, flash, flash, 0.0f);

    mResult.mFogColor += flashColor;
    mResult.mAmbientColor += flashColor;
    mResult.mSunColor += flashColor;
}

inline void WeatherManager::calculateResult(const int weatherID, const float gameHour)
{
    const Weather& current = mWeatherSettings[weatherID];

    mResult.mCloudTexture = current.mCloudTexture;
    mResult.mCloudBlendFactor = 0;
    mResult.mNextWindSpeed = 0;
    mResult.mWindSpeed = mResult.mCurrentWindSpeed = calculateWindSpeed(weatherID, mWindSpeed);

    mResult.mCloudSpeed = current.mCloudSpeed;
    mResult.mGlareView = current.mGlareView;
    mResult.mAmbientLoopSoundID = current.mAmbientLoopSoundID;
    mResult.mAmbientSoundVolume = 1.f;
    mResult.mEffectFade = 1.f;

    mResult.mIsStorm = current.mIsStorm;

    mResult.mRainSpeed = current.mRainSpeed;
    mResult.mRainEntranceSpeed = current.mRainEntranceSpeed;
    mResult.mRainDiameter = current.mRainDiameter;
    mResult.mRainMinHeight = current.mRainMinHeight;
    mResult.mRainMaxHeight = current.mRainMaxHeight;
    mResult.mRainMaxRaindrops = current.mRainMaxRaindrops;

    mResult.mParticleEffect = current.mParticleEffect;
    mResult.mRainEffect = current.mRainEffect;

    mResult.mNight = (gameHour < mSunriseTime || gameHour > mTimeSettings.mNightStart + mTimeSettings.mStarsPostSunsetStart - mTimeSettings.mStarsFadingDuration);

    mResult.mFogDepth = current.mLandFogDepth.getValue(gameHour, mTimeSettings, "Fog");
    mResult.mFogColor = current.mFogColor.getValue(gameHour, mTimeSettings, "Fog");
    mResult.mAmbientColor = current.mAmbientColor.getValue(gameHour, mTimeSettings, "Ambient");
    mResult.mSunColor = current.mSunColor.getValue(gameHour, mTimeSettings, "Sun");
    mResult.mSkyColor = current.mSkyColor.getValue(gameHour, mTimeSettings, "Sky");
    mResult.mNightFade = mNightFade.getValue(gameHour, mTimeSettings, "Stars");
    mResult.mDLFogFactor = current.mDL.FogFactor;
    mResult.mDLFogOffset = current.mDL.FogOffset;

    WeatherSetting setting = mTimeSettings.getSetting("Sun");
    float preSunsetTime = setting.mPreSunsetTime;

    if (gameHour >= mTimeSettings.mDayEnd - preSunsetTime)
    {
        float factor = 1.f;
        if (preSunsetTime > 0)
            factor = (gameHour - (mTimeSettings.mDayEnd - preSunsetTime)) / preSunsetTime;
        factor = std::min(1.f, factor);
        mResult.mSunDiscColor = lerp(osg::Vec4f(1,1,1,1), current.mSunDiscSunsetColor, factor);
        // The SunDiscSunsetColor in the INI isn't exactly the resulting color on screen, most likely because
        // MW applied the color to the ambient term as well. After the ambient and emissive terms are added together, the fixed pipeline
        // would then clamp the total lighting to (1,1,1). A noticeable change in color tone can be observed when only one of the color components gets clamped.
        // Unfortunately that means we can't use the INI color as is, have to replicate the above nonsense.
        mResult.mSunDiscColor = mResult.mSunDiscColor + osg::componentMultiply(mResult.mSunDiscColor, mResult.mAmbientColor);
        for (int i=0; i<3; ++i)
            mResult.mSunDiscColor[i] = std::min(1.f, mResult.mSunDiscColor[i]);
    }
    else
        mResult.mSunDiscColor = osg::Vec4f(1,1,1,1);

    if (gameHour >= mTimeSettings.mDayEnd)
    {
        // sunset
        float fade = std::min(1.f, (gameHour - mTimeSettings.mDayEnd) / (mTimeSettings.mNightStart - mTimeSettings.mDayEnd));
        fade = fade*fade;
        mResult.mSunDiscColor.a() = 1.f - fade;
    }
    else if (gameHour >= mTimeSettings.mNightEnd && gameHour <= mTimeSettings.mNightEnd + mSunriseDuration / 2.f)
    {
        // sunrise
        mResult.mSunDiscColor.a() = gameHour - mTimeSettings.mNightEnd;
    }
    else
        mResult.mSunDiscColor.a() = 1;

}

inline void WeatherManager::calculateTransitionResult(const float factor, const float gameHour)
{
    calculateResult(mCurrentWeather, gameHour);
    const MWRender::WeatherResult current = mResult;
    calculateResult(mNextWeather, gameHour);
    const MWRender::WeatherResult other = mResult;

    mResult.mCloudTexture = current.mCloudTexture;
    mResult.mNextCloudTexture = other.mCloudTexture;
    mResult.mCloudBlendFactor = mWeatherSettings[mNextWeather].cloudBlendFactor(factor);

    mResult.mFogColor = lerp(current.mFogColor, other.mFogColor, factor);
    mResult.mSunColor = lerp(current.mSunColor, other.mSunColor, factor);
    mResult.mSkyColor = lerp(current.mSkyColor, other.mSkyColor, factor);

    mResult.mAmbientColor = lerp(current.mAmbientColor, other.mAmbientColor, factor);
    mResult.mSunDiscColor = lerp(current.mSunDiscColor, other.mSunDiscColor, factor);
    mResult.mFogDepth = lerp(current.mFogDepth, other.mFogDepth, factor);
    mResult.mDLFogFactor = lerp(current.mDLFogFactor, other.mDLFogFactor, factor);
    mResult.mDLFogOffset = lerp(current.mDLFogOffset, other.mDLFogOffset, factor);

    mResult.mCurrentWindSpeed = calculateWindSpeed(mCurrentWeather, mCurrentWindSpeed);
    mResult.mNextWindSpeed = calculateWindSpeed(mNextWeather, mNextWindSpeed);

    mResult.mWindSpeed = lerp(mResult.mCurrentWindSpeed, mResult.mNextWindSpeed, factor);
    mResult.mCloudSpeed = lerp(current.mCloudSpeed, other.mCloudSpeed, factor);
    mResult.mGlareView = lerp(current.mGlareView, other.mGlareView, factor);
    mResult.mNightFade = lerp(current.mNightFade, other.mNightFade, factor);

    mResult.mNight = current.mNight;

    float threshold = mWeatherSettings[mNextWeather].mRainThreshold;
    if (threshold <= 0)
        threshold = 0.5f;

    if(factor < threshold)
    {
        mResult.mIsStorm = current.mIsStorm;
        mResult.mParticleEffect = current.mParticleEffect;
        mResult.mRainEffect = current.mRainEffect;
        mResult.mRainSpeed = current.mRainSpeed;
        mResult.mRainEntranceSpeed = current.mRainEntranceSpeed;
        mResult.mAmbientSoundVolume = 1 - factor / threshold;
        mResult.mEffectFade = mResult.mAmbientSoundVolume;
        mResult.mAmbientLoopSoundID = current.mAmbientLoopSoundID;
        mResult.mRainDiameter = current.mRainDiameter;
        mResult.mRainMinHeight = current.mRainMinHeight;
        mResult.mRainMaxHeight = current.mRainMaxHeight;
        mResult.mRainMaxRaindrops = current.mRainMaxRaindrops;
    }
    else
    {
        mResult.mIsStorm = other.mIsStorm;
        mResult.mParticleEffect = other.mParticleEffect;
        mResult.mRainEffect = other.mRainEffect;
        mResult.mRainSpeed = other.mRainSpeed;
        mResult.mRainEntranceSpeed = other.mRainEntranceSpeed;
        mResult.mAmbientSoundVolume = (factor - threshold) / (1 - threshold);
        mResult.mEffectFade = mResult.mAmbientSoundVolume;
        mResult.mAmbientLoopSoundID = other.mAmbientLoopSoundID;

        mResult.mRainDiameter = other.mRainDiameter;
        mResult.mRainMinHeight = other.mRainMinHeight;
        mResult.mRainMaxHeight = other.mRainMaxHeight;
        mResult.mRainMaxRaindrops = other.mRainMaxRaindrops;
    }
}

