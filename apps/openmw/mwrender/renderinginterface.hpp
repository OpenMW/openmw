#ifndef _GAME_RENDERING_INTERFACE_H
#define _GAME_RENDERING_INTERFACE_H
namespace MWRender{
      class Objects;
      class Actors;
      class Player;
      
class RenderingInterface{
    public:
        virtual MWRender::Objects& getObjects() = 0;
	    virtual MWRender::Player& getPlayer() = 0;
        virtual MWRender::Actors& getActors() = 0;
        virtual ~RenderingInterface(){};
    };
}
#endif