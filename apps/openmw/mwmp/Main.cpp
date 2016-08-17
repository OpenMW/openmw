//
// Created by koncord on 01.01.16.
//

#include "Main.hpp"
#include <apps/openmw/mwworld/manualref.hpp>
#include <apps/openmw/mwmechanics/aitravel.hpp>
#include <components/esm/esmwriter.hpp>
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
#include "../mwclass/npc.hpp"
#include "../mwclass/creature.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwworld/cellstore.hpp"
#include <apps/openmw/mwdialogue/dialoguemanagerimp.hpp>
#include <apps/openmw/mwworld/inventorystore.hpp>
#include <apps/openmw/mwmechanics/spellcasting.hpp>

#include "DedicatedPlayer.hpp"
#include "LocalPlayer.hpp"

using namespace mwmp;
using namespace std;

Main *Main::pMain = 0;

std::string loadSettings (Settings::Manager & settings)
{
    Files::ConfigurationManager mCfgMgr;
    // Create the settings manager and load default settings file
    const std::string localdefault = (mCfgMgr.getLocalPath() / "tes3mp-client-default.cfg").string();
    const std::string globaldefault = (mCfgMgr.getGlobalPath() / "tes3mp-client-default.cfg").string();

    // prefer local
    if (boost::filesystem::exists(localdefault))
        settings.loadDefault(localdefault);
    else if (boost::filesystem::exists(globaldefault))
        settings.loadDefault(globaldefault);
    else
        throw std::runtime_error ("No default settings file found! Make sure the file \"tes3mp-client-default.cfg\" was properly installed.");

    // load user settings if they exist
    const std::string settingspath = (mCfgMgr.getUserConfigPath() / "tes3mp-client.cfg").string();
    if (boost::filesystem::exists(settingspath))
        settings.loadUser(settingspath);

    return settingspath;
}

Main::Main()
{
    std::cout << "Main::Main" << std::endl;
    mNetworking = new Networking();
    mLocalPlayer = new LocalPlayer();
    mGUIController = new GUIController();
    //mLocalPlayer->CharGen(0, 4);

    server = "mp.tes3mp.com";
    port = 25565;
}

Main::~Main()
{
    std::cout << "Main::~Main" << std::endl;
    delete mNetworking;
    delete mLocalPlayer;
    delete mGUIController;
    Players::CleanUp();
}

void Main::Create()
{
    assert(!pMain);
    pMain = new Main();

    Settings::Manager mgr;
    Settings::CategorySettingValueMap saveUserSettings = mgr.mUserSettings;
    Settings::CategorySettingValueMap saveDefaultSettings = mgr.mDefaultSettings;
    Settings::CategorySettingVector saveChangedSettings = mgr.mChangedSettings;
    mgr.mUserSettings.clear();
    mgr.mDefaultSettings.clear();
    mgr.mChangedSettings.clear();
    loadSettings(mgr);

    pMain->server = mgr.getString("server", "General");
    pMain->port = (unsigned short)mgr.getInt("port", "General");

    pMain->mGUIController->setupChat(mgr);

    mgr.mUserSettings = saveUserSettings;
    mgr.mDefaultSettings = saveDefaultSettings;
    mgr.mChangedSettings = saveChangedSettings;

    //pMain->mGUILogin = new GUILogin();
    const MWBase::Environment &environment = MWBase::Environment::get();
    environment.getStateManager()->newGame(true);
}

void Main::Destroy()
{
    assert(pMain);

    delete pMain->mGUIController;

    delete pMain;
    pMain = 0;
}

void Main::Frame(float dt)
{
    const MWBase::Environment &environment = MWBase::Environment::get();
    if (environment.getWindowManager()->containsMode(MWGui::GM_MainMenu))
    {
        //environment.getWindowManager()->exitCurrentGuiMode();
    }

    get().getNetworking()->Update();

    Players::Update(dt);
    get().UpdateWorld(dt);

    get().getGUIConroller()->update(dt);

}

void Main::UpdateWorld(float dt) const
{

    if (!mLocalPlayer->CharGenThread())
        return;

    if (!mNetworking->isConnected())
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWBase::Environment::get().getMechanicsManager()->toggleAI();

        (*mLocalPlayer->Npc()) = *player.get<ESM::NPC>()->mBase;

        mLocalPlayer->updateAttributesAndSkills();

        mNetworking->Connect(server, port);
        player.getClass().getCreatureStats(player).getSpells().add("fireball");
        mLocalPlayer->updateBaseStats(true);
        get().getGUIConroller()->setChatVisible(true);
    }
    else
        mLocalPlayer->Update();
}

const Main &Main::get()
{
    return *pMain;
}

Networking *Main::getNetworking() const
{
    return mNetworking;
}

LocalPlayer *Main::getLocalPlayer() const
{
    return mLocalPlayer;
}


GUIController *Main::getGUIConroller() const
{
    return mGUIController;
}

void Main::PressedKey(int key)
{
    if (pMain == nullptr) return;
    if (get().getGUIConroller()->pressedKey(key))
        return; // if any gui bind pressed
}