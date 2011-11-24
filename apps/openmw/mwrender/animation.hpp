#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H
#include <components/nif/data.hpp>
namespace MWRender{
class Animation{
    std::vector<Nif::NiKeyframeData> transformations;
    std::map<std::string,float> textmappings;
};
}
#endif