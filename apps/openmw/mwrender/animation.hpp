#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H
#include <components/nif/data.hpp>
#include <openengine/ogre/renderer.hpp>
#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/environment.hpp"

namespace MWRender{

class Animation{
   protected:
    OEngine::Render::OgreRenderer &mRend;
    MWWorld::Environment& mEnvironment;    
   

    std::vector<Nif::NiKeyframeData> transformations;
    std::map<std::string,float> textmappings;
    Ogre::Entity* base;
    public:
     Animation(MWWorld::Environment& _env, OEngine::Render::OgreRenderer& _rend): mRend(_rend), mEnvironment(_env){};
     ~Animation();
 
};
}
#endif