#ifndef _GAME_RENDER_NPCANIMATION_H
#define _GAME_RENDER_NPCANIMATION_H
#include "animation.hpp"
#include <components/nif/data.hpp>
#include <components/nif/node.hpp>
namespace MWRender{

class NpcAnimation: Animation{
    std::vector<std::vector<Nif::NiTriShapeCopy>> shapeparts;   //All the NiTriShape data that we need for animating this particular npc
};
}
#endif