#include "weather.hpp"

#include "../mwrender/renderingmanager.hpp"

using namespace MWWorld;

WeatherManager::WeatherManager(MWRender::RenderingManager* rendering)
{
    mRendering = rendering;
}
