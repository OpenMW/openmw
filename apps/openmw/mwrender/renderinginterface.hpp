#ifndef _GAME_RENDERING_INTERFACE_H
#define _GAME_RENDERING_INTERFACE_H
namespace MWRender{
      class Npcs;
      class Creatures;
      class Objects;
      class Player;
class RenderingInterface{
    public:
        virtual MWRender::Npcs& getNPCs() = 0;
        virtual MWRender::Creatures& getCreatures() = 0;
        virtual MWRender::Objects& getObjects() = 0;
	    virtual MWRender::Player& getPlayer() = 0;
        virtual ~RenderingInterface(){};
    };
}
#endif