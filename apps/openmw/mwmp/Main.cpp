//
// Created by koncord on 01.01.16.
//

#include "Main.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwmechanics/aitravel.hpp"
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
#include "../mwdialogue/dialoguemanagerimp.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include <components/openmw-mp/Log.hpp>
#include <cstdlib>
#include <components/openmw-mp/Version.hpp>

#include "Networking.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"
#include "GUIController.hpp"
#include "WorldController.hpp"

using namespace mwmp;
using namespace std;

Main *Main::pMain = 0;
std::string Main::addr = "";
std::string Main::passw = TES3MP_DEFAULT_PASSW;

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
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "tes3mp started");
    mNetworking = new Networking();
    mLocalPlayer = new LocalPlayer();
    mGUIController = new GUIController();
    mWorldController = new WorldController();
    //mLocalPlayer->CharGen(0, 4);

    server = "mp.tes3mp.com";
    port = 25565;
}

Main::~Main()
{
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "tes3mp stopped");
    delete mNetworking;
    delete mLocalPlayer;
    Players::cleanUp();
}

void Main::optionsDesc(boost::program_options::options_description *desc)
{
    namespace bpo = boost::program_options;
    desc->add_options()
            ("connect", bpo::value<std::string>()->default_value(""),
                        "connect to server (e.g. --connect=127.0.0.1:25565)")
            ("password", bpo::value<std::string>()->default_value(TES3MP_DEFAULT_PASSW),
                        "сonnect to a secured server. (e.g. --password=AnyPassword");
}

void Main::configure(const boost::program_options::variables_map &variables)
{
    Main::addr = variables["connect"].as<string>();
    Main::passw = variables["password"].as<string>();
}

static Settings::CategorySettingValueMap saveUserSettings;
static Settings::CategorySettingValueMap saveDefaultSettings;
static Settings::CategorySettingVector saveChangedSettings;

void InitMgr(Settings::Manager &mgr)
{
    saveUserSettings = mgr.mUserSettings;
    saveDefaultSettings = mgr.mDefaultSettings;
    saveChangedSettings = mgr.mChangedSettings;
    mgr.mUserSettings.clear();
    mgr.mDefaultSettings.clear();
    mgr.mChangedSettings.clear();
    loadSettings(mgr);
}

void RestoreMgr(Settings::Manager &mgr)
{
    mgr.mUserSettings = saveUserSettings;
    mgr.mDefaultSettings = saveDefaultSettings;
    mgr.mChangedSettings = saveChangedSettings;
}

bool Main::init(std::vector<std::string> &content)
{
    assert(!pMain);
    pMain = new Main();

    Settings::Manager mgr;
    InitMgr(mgr);

    int logLevel = mgr.getInt("loglevel", "General");
    Log::SetLevel(logLevel);
    if (addr.empty())
    {
        pMain->server = mgr.getString("server", "General");
        pMain->port = (unsigned short) mgr.getInt("port", "General");

        passw = mgr.getString("password", "General");
        if (passw.empty())
            passw = TES3MP_DEFAULT_PASSW;
    }
    else
    {
        size_t delim_pos = addr.find(':');
        pMain->server = addr.substr(0, delim_pos);
        pMain->port = atoi(addr.substr(delim_pos + 1).c_str());
    }
    get().mLocalPlayer->passw = passw;
    
    pMain->mNetworking->connect(pMain->server, pMain->port);
    RestoreMgr(mgr);
    return pMain->mNetworking->isConnected();
}

void Main::postInit()
{
    Settings::Manager mgr;
    InitMgr(mgr);

    pMain->mGUIController->setupChat(mgr);
    RestoreMgr(mgr);

    const MWBase::Environment &environment = MWBase::Environment::get();
    environment.getStateManager()->newGame(true);
    MWBase::Environment::get().getMechanicsManager()->toggleAI();
}

void Main::destroy()
{
    assert(pMain);

    delete pMain;
    pMain = 0;
}

void Main::frame(float dt)
{
    const MWBase::Environment &environment = MWBase::Environment::get();

    get().getNetworking()->update();

    Players::update(dt);
    get().updateWorld(dt);

    get().getGUIController()->update(dt);

}

void Main::updateWorld(float dt) const
{

    if (!mLocalPlayer->charGenThread())
        return;

    static bool init = true;
    if (init)
    {
        init = false;
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_PLAYER_BASEINFO to server");

        mNetworking->getPlayerPacket(ID_PLAYER_BASEINFO)->Send(getLocalPlayer());
        mNetworking->getPlayerPacket(ID_LOADED)->Send(getLocalPlayer());
        mLocalPlayer->updateDynamicStats(true);
        get().getGUIController()->setChatVisible(true);
    }
    else
        mLocalPlayer->update();
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


GUIController *Main::getGUIController() const
{
    return mGUIController;
}

WorldController *Main::getWorldController() const
{
    return mWorldController;
}

void Main::pressedKey(int key)
{
    if (pMain == nullptr) return;
    if (get().getGUIController()->pressedKey(key))
        return; // if any gui bind pressed
}

// When sending packets with ingame script values, certain packets
// should be ignored because of their potential for spam
bool Main::isValidPacketScript(std::string script)
{
    static const int validPacketScriptsCount = 21;
    static const std::string validPacketScripts[validPacketScriptsCount] = {
        // Ghostgate buttons
        "GG_OpenGate1", // coc Ghostgate
        "GG_OpenGate2",
        // Dwemer ruin cranks
        "Arkn_doors", // coe 0, -2
        "nchuleftingthWrong1", // coc "Nchuleftingth, Test of Pattern"
        "nchuleftingthWrong2",
        "nchulfetingthRight",
        "Akula_innerdoors", // coc "Akulakhan's Chamber"
        "Dagoth_doors", // coe 2, 8
        // Sotha Sil levers
        "SothaLever1", // coc "Sotha Sil, Outer Flooded Halls"
        "SothaLever2",
        "SothaLever3",
        "SothaLever4",
        "SothaLever5",
        "SothaLever6",
        "SothaLever7",
        "SothaLever8",
        "SothaLever9",
        "SothaLever10",
        "SothaLever11",
        "SothaOilLever", // coc "Sotha Sil, Dome of Udok"
        // Generic state script
        "LocalState"
    };

    static const int invalidPacketScriptsCount = 17;
    static const std::string invalidPacketScripts[invalidPacketScriptsCount] = {
        // Spammy shorts
        "OutsideBanner",
        "sleeperScript",
        "dreamer_talkerEnable",
        "drenSlaveOwners",
        "ahnassiScript",
        "hlormarScript",
        // Spammy floats
        "Float",
        "SignRotate",
        "FaluraScript",
        "jsaddhaScript",
        // Spammy globals
        "wraithguardScript",
        // Spammy globals leading to crashes
        "LegionUniform",
        "OrdinatorUniform",
        "LorkhanHeart",
        "ouch_keening",
        "ouch_sunder",
        "ouch_wraithguard"
    };

    for (int i = 0; i < validPacketScriptsCount; i++)
    {
        if (Misc::StringUtils::ciEqual(script, validPacketScripts[i]))
            return true;
    }

    return false;

    /* Switch over to this when using a blacklist system
    for (int i = 0; i < invalidPacketScriptsCount; i++)
    {
        if (Misc::StringUtils::ciEqual(script, invalidPacketScripts[i]))
            return false;
    }

    return true;
    */
}
