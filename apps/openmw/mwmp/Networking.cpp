//
// Created by koncord on 01.01.16.
//

#include "Networking.hpp"
#include <cassert>
#include <apps/openmw/mwworld/manualref.hpp>
#include <apps/openmw/mwmechanics/aitravel.hpp>
#include "../mwbase/environment.hpp"
#include "../mwstate/statemanagerimp.hpp"
#include "../mwinput/inputmanagerimp.hpp"
#include "../mwscript/scriptmanagerimp.hpp"
#include "../mwgui/windowmanagerimp.hpp"
#include "../mwworld/worldimp.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwclass/npc.hpp"
#include "../mwclass/creature.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"

#include "../mwmechanics/aistate.hpp"
#include "Player.hpp"

using namespace mwmp;
using namespace std;

Main *Main::pMain = 0;

Main::Main()
{
    std::cout << "Main::Main" << std::endl;
}

Main::~Main()
{
    std::cout << "Main::~Main" << std::endl;
}

void Main::Create()
{
    assert(!pMain);
    pMain = new Main();
    const MWBase::Environment &environment = MWBase::Environment::get();
    environment.getStateManager()->newGame(true);

}

void Main::Destroy()
{
    Player::CleanUp();
    delete pMain;
}

void Main::Frame(float dt)
{
    const MWBase::Environment &environment = MWBase::Environment::get();
    if (environment.getWindowManager()->containsMode(MWGui::GM_MainMenu))
    {
        //environment.getWindowManager()->exitCurrentGuiMode();
    }
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWBase::ScriptManager *script = MWBase::Environment::get().getScriptManager();
    MWWorld::Ptr player = world->getPlayerPtr();

    float x = player.getRefData().getPosition().pos[0];
    float y = player.getRefData().getPosition().pos[1];
    float z = player.getRefData().getPosition().pos[2];
    float rot_x = player.getRefData().getPosition().rot[0];
    float rot_y = player.getRefData().getPosition().rot[1];
    float rot_z = player.getRefData().getPosition().rot[2];

    static bool connected = true;
    if (connected)
    {
        connected = false;
        world->toggleGodMode();
        // create item
        MWWorld::CellStore* store = player.getCell();

        Player::CreatePlayer(1, "Ashot the Orc", "Orc", "b_n_orc_m_head_01", "b_n_orc_m_hair_01");
        Player *ref = Player::GetPlayer(1);

        ref->getPtr().getCellRef().setPosition(player.getRefData().getPosition());
        world->moveObject(ref->getPtr(),player.getCell(), x, y, z); // move to player
    }
    Player *ref = Player::GetPlayer(1);

    ref->Move(player.getRefData().getPosition(), player.getCell());

    //cout << "x:\t" << x << "\ty:\t" << y << "\tz:\t" << z<< endl;

    //ref->getPtr().getRefData().
    //loc.getRefData().setPosition(player.getRefData().getPosition());
    MWMechanics::NpcStats *npcStats = &ref->getPtr().getClass().getNpcStats(ref->getPtr());
    npcStats->setHealth(1000);
    npcStats->setMagicka(1000);
    npcStats->setFatigue(1000);
    if(npcStats->isDead())
        npcStats->resurrect();
    npcStats->setAttacked(false);

    npcStats->setBaseDisposition(255);

    if(player.getClass().getNpcStats(ref->getPtr()).getHealth().getCurrent() <= 0)
    {

    }

}

void Main::UpdateWorld(float dt)
{

}
