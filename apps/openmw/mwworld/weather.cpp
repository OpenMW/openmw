#include "weather.hpp"

#include "../mwrender/renderingmanager.hpp"

using namespace Ogre;
using namespace MWWorld;

#define TRANSITION_TIME 10

WeatherManager::WeatherManager(MWRender::RenderingManager* rendering) : 
    mCurrentWeather("clear")
{
    mRendering = rendering;
    
    /// \todo read these from Morrowind.ini
    Weather clear;
    clear.mCloudTexture = "tx_sky_clear.dds";
    clear.mCloudsMaximumPercent = 0.75;
    mWeatherSettings["clear"] = clear;
    
    Weather cloudy;
    cloudy.mCloudTexture = "tx_sky_cloudy.dds";
    cloudy.mCloudsMaximumPercent = 0.9;
    mWeatherSettings["cloudy"] = cloudy;
    
    Weather overcast;
    overcast.mCloudTexture = "tx_sky_overcast.dds";
    overcast.mCloudsMaximumPercent = 1.0;
    mWeatherSettings["overcast"] = overcast;
    
    setWeather("clear", true);
    setWeather("cloudy", false);
}

void WeatherManager::setWeather(const String& weather, bool instant)
{
    if (instant)
    {
        mNextWeather = "";
        mCurrentWeather = weather;
    }
    else if (weather != mCurrentWeather)
    {
        mNextWeather = weather;
        mRemainingTransitionTime = TRANSITION_TIME;
    }
}

WeatherResult WeatherManager::getResult(const String& weather)
{
    const Weather& current = mWeatherSettings[weather];
    WeatherResult result;
    
    result.mCloudTexture = current.mCloudTexture;
    result.mCloudBlendFactor = 0;
    result.mCloudOpacity = current.mCloudsMaximumPercent;
    /// \todo
    
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
        
    #define lerp(x, y) (x * (1-factor) + y * factor)
    
    result.mCloudOpacity = lerp(current.mCloudOpacity, other.mCloudOpacity);
    
    /// \todo
    
    return result;
}

void WeatherManager::update(float duration)
{
    WeatherResult result;
    
    if (mNextWeather != "")
    {
        mRemainingTransitionTime -= duration;
        if (mRemainingTransitionTime < 0)
        {
            mCurrentWeather = mNextWeather;
            mNextWeather = "";
        }
    }
    
    if (mNextWeather != "")
        result = transition(1-(mRemainingTransitionTime/TRANSITION_TIME));
    else
        result = getResult(mCurrentWeather);
    
    mRendering->getSkyManager()->setWeather(result);
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
