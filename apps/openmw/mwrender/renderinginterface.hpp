#include "objects.hpp"
#include "npcs.hpp"
#include "creatures.hpp"
#include "player.hpp"
#define RENDERING_INTERFACE 1
namespace MWRender{
class RenderingInterface{
    public:
        virtual MWRender::Npcs& getNPCs() = 0;
        virtual MWRender::Creatures& getCreatures() = 0;
        virtual MWRender::Objects& getObjects() = 0;
	    virtual MWRender::Player& getPlayer() = 0;
    };
}