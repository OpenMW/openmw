#ifndef _GAME_RENDER_CREATUREANIMATION_H
#define _GAME_RENDER_CREATUREANIMATION_H

#include "animation.hpp"
#include <components/nif/node.hpp>
namespace MWRender{

class CreatureAnimation: Animation{
    std::vector<Nif::NiTriShapeCopy> shapes;          //All the NiTriShapeData for this creature
};
}
#endif