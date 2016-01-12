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
#include "GUIChat.hpp"

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
    //mLocalPlayer->CharGen(0, 4);

    server = "mp.tes3mp.com";
    port = 25565;
    keySay = SDLK_y;
    keyChatMode = SDLK_F2;
}

Main::~Main()
{
    std::cout << "Main::~Main" << std::endl;
    delete mNetworking;
    delete mLocalPlayer;
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

    float chatDelay = mgr.getFloat("delay", "Chat");
    int chatY = mgr.getInt("y", "Chat");
    int chatX = mgr.getInt("x", "Chat");
    int chatW = mgr.getInt("w", "Chat");
    int chatH = mgr.getInt("h", "Chat");
    pMain->keySay =      SDL_GetKeyFromName(mgr.getString("keySay", "Chat").c_str());
    pMain->keyChatMode = SDL_GetKeyFromName(mgr.getString("keyChatMode", "Chat").c_str());


    pMain->mChat = new GUIChat(chatX, chatY, chatW, chatH);
    pMain->getChatBox()->SetDelay(chatDelay);

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

    delete pMain->mChat;

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

    get().getChatBox()->Update(dt);

}

void Main::UpdateWorld(float dt) const
{

    if(!mLocalPlayer->CharGenThread())
        return;

    if(!mNetworking->isConnected())
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWBase::Environment::get().getMechanicsManager()->toggleAI();

        (*mLocalPlayer->Npc()) = *player.get<ESM::NPC>()->mBase;

        mLocalPlayer->updateAttributesAndSkills();

        mNetworking->Connect(server, port);
        player.getClass().getCreatureStats(player).getSpells().add("fireball");
        mLocalPlayer->updateBaseStats(true);
        mChat->setVisible(true);
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

GUIChat *Main::getChatBox() const
{
    return mChat;
}

GUILogin *Main::getGUILogin() const
{
    return mGUILogin;
}


void Main::PressedKey(int key)
{

    if(pMain == nullptr || get().getChatBox() == nullptr) return;
    if(key == get().keyChatMode)
        get().getChatBox()->PressedChatMode();
    else if(key == get().keySay)
        get().getChatBox()->PressedSay();
}