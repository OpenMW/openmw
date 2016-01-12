#include <RakPeerInterface.h>
#include <BitStream.h>
#include "Player.hpp"
#include "Networking.hpp"
#include <RakPeer.h>
#include <MessageIdentifiers.h>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Script/Script.hpp>
#include <iostream>
#include <components/files/configurationmanager.hpp>
#include <components/settings/settings.hpp>

using namespace std;
using namespace mwmp;

void printVersion(string version, int protocol)
{
    cout << "TES3:MP dedicated server " << version;
    cout << " (";
#ifdef __WIN32__
    cout << "Windows";
#elif __linux
    cout << "Linux";
#elif __APPLE__
    cout << "OS X";
#else
    cout << "Unknown OS";
#endif
    cout << " ";
#ifdef __x86_64__
    cout << "64-bit";
#elif defined __i386__ || defined _M_I86
    cout << "32-bit";
#else
    cout << "Unknown architecture";
#endif
    cout << ")" << endl;
    cout << "Protocol version: " << protocol << endl;

    cout << "------------------------------------------------------------" << endl;
}

std::string loadSettings (Settings::Manager & settings)
{
    Files::ConfigurationManager mCfgMgr;
    // Create the settings manager and load default settings file
    const std::string localdefault = (mCfgMgr.getLocalPath() / "tes3mp-server-default.cfg").string();
    const std::string globaldefault = (mCfgMgr.getGlobalPath() / "tes3mp-server-default.cfg").string();

    // prefer local
    if (boost::filesystem::exists(localdefault))
        settings.loadDefault(localdefault);
    else if (boost::filesystem::exists(globaldefault))
        settings.loadDefault(globaldefault);
    else
        throw std::runtime_error ("No default settings file found! Make sure the file \"tes3mp-server-default.cfg\" was properly installed.");

    // load user settings if they exist
    const std::string settingspath = (mCfgMgr.getUserConfigPath() / "tes3mp-server.cfg").string();
    if (boost::filesystem::exists(settingspath))
        settings.loadUser(settingspath);

    return settingspath;
}

const vector<string> split(const string &str, int delimiter)
{
    string buffer;
    vector<string> result;

    for (auto symb:str)
        if (symb != delimiter)
            buffer += symb;
        else if (!buffer.empty())
        {
            result.push_back(move(buffer));
            buffer.clear();
        }
    if (!buffer.empty())
        result.push_back(move(buffer));

    return result;
}



int main(int argc, char *argv[])
{
    Settings::Manager mgr;

    loadSettings(mgr);

    //string plugin_home = "/home/koncord/ClionProjects/tes3mp-server/files";

    int players = mgr.getInt("players", "General");
    int port = mgr.getInt("port", "General");

    std::string plugin_home = mgr.getString("home", "Plugins");
    string moddir = plugin_home + "/files";

    vector<string> plugins (split(mgr.getString("plugins", "Plugins"), ','));

    cout << plugins[0] << endl;

    printVersion("0.0.1b", 1);


    setenv("AMXFILE", moddir.c_str(), 1);
    setenv("MOD_DIR", moddir.c_str(), 1); // hack for lua

    setenv("LUA_PATH", (plugin_home + "/scripts/?.lua" + ";" + plugin_home + "/scripts/?.t").c_str(), 1);

    for(auto plugin : plugins)
        Script::LoadScript(plugin.c_str(), plugin_home.c_str());

    RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();

    const char passw[8] = "1234567";
    peer->SetIncomingPassword(passw, sizeof(passw));

    RakNet::SocketDescriptor sd((unsigned short)port, 0);
    if (peer->Startup((unsigned)players, &sd, 1) != RakNet::RAKNET_STARTED)
        return 0;

    peer->SetMaximumIncomingConnections((unsigned short)(players / 2));

    Networking networking(peer);

    int code = networking.MainLoop();

    RakNet::RakPeerInterface::DestroyInstance(peer);
    if (code == 0)
        printf("Quitting peacefully.\n");

    return code;
}