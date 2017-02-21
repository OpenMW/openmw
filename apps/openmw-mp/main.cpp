#include <RakPeerInterface.h>
#include <BitStream.h>
#include "Player.hpp"
#include "Networking.hpp"
#include "MasterClient.hpp"
#include <RakPeer.h>
#include <MessageIdentifiers.h>
#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Script/Script.hpp>
#include <iostream>
#include <components/files/configurationmanager.hpp>
#include <components/settings/settings.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/filesystem/fstream.hpp>
#include <components/openmw-mp/Version.hpp>

#include "MasterClient.hpp"

#ifdef ENABLE_BREAKPAD
#include <handler/exception_handler.h>
#endif

using namespace std;
using namespace mwmp;

void printVersion(string version, int protocol)
{
    cout << "TES3:MP dedicated server " << version;
    cout << " (";
#if defined(_WIN32)
    cout << "Windows";
#elif defined(__linux)
    cout << "Linux";
#elif defined(__APPLE__)
    cout << "OS X";
#else
    cout << "Unknown OS";
#endif
    cout << " ";
#ifdef __x86_64__
    cout << "64-bit";
#elif defined(__i386__) || defined(_M_I86)
    cout << "32-bit";
#elif defined(__arm__)
    cout << "ARMv" << __ARM_ARCH << " ";
    #ifdef __aarch64__
        cout << "64-bit";
    #else
        cout << "32-bit";
    #endif
#else
    cout << "Unknown architecture";
#endif
    cout << ")" << endl;
    cout << "Protocol version: " << protocol << endl;

    cout << "------------------------------------------------------------" << endl;
}

#ifdef ENABLE_BREAKPAD
google_breakpad::ExceptionHandler *pHandler = 0;
#if defined(_WIN32)
bool DumpCallback(const wchar_t* _dump_dir,const wchar_t* _minidump_id,void* context,EXCEPTION_POINTERS* exinfo,MDRawAssertionInfo* assertion,bool success)
#elif defined(__linux)
bool DumpCallback(const google_breakpad::MinidumpDescriptor &md, void *context, bool success)
#endif
{
    // NO STACK USE, NO HEAP USE THERE !!!
    return success;
}

void breakpad(std::string pathToDump)
{
#ifdef _WIN32
    pHandler = new google_breakpad::ExceptionHandler(
            L"crashdumps\\",
            /*FilterCallback*/ 0,
            DumpCallback,
            0,
            google_breakpad::ExceptionHandler::HANDLER_ALL);
#else
    google_breakpad::MinidumpDescriptor md(pathToDump);
    pHandler = new google_breakpad::ExceptionHandler(
            md,
            /*FilterCallback*/ 0,
            DumpCallback,
            /*context*/ 0,
            true,
            -1
    );
#endif
}

void breakpad_close()
{
    delete pHandler;
}
#else
void breakpad(std::string pathToDump){}
void breakpad_close(){}
#endif

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

class Tee : public boost::iostreams::sink
{
public:
    Tee(std::ostream &stream, std::ostream &stream2)
            : out(stream), out2(stream2)
    {
    }

    std::streamsize write(const char *str, std::streamsize size)
    {
        out.write (str, size);
        out.flush();
        out2.write (str, size);
        out2.flush();
        return size;
    }

private:
    std::ostream &out;
    std::ostream &out2;
};

int main(int argc, char *argv[])
{
    Settings::Manager mgr;
    Files::ConfigurationManager cfgMgr;

    breakpad(boost::filesystem::path(cfgMgr.getLogPath()).string());

    loadSettings(mgr);

    int logLevel = mgr.getInt("loglevel", "General");
    if (logLevel < Log::LOG_VERBOSE || logLevel > Log::LOG_FATAL)
        logLevel = Log::LOG_VERBOSE;

    // Some objects used to redirect cout and cerr
    // Scope must be here, so this still works inside the catch block for logging exceptions
    std::streambuf* cout_rdbuf = std::cout.rdbuf ();
    std::streambuf* cerr_rdbuf = std::cerr.rdbuf ();

    boost::iostreams::stream_buffer<Tee> coutsb;
    boost::iostreams::stream_buffer<Tee> cerrsb;

    std::ostream oldcout(cout_rdbuf);
    std::ostream oldcerr(cerr_rdbuf);

    boost::filesystem::ofstream logfile;

    // Redirect cout and cerr to tes3mp server log
    logfile.open (boost::filesystem::path(cfgMgr.getLogPath() / "/tes3mp-server-" += Log::getFilenameTimestamp() += ".log"));

    coutsb.open (Tee(logfile, oldcout));
    cerrsb.open (Tee(logfile, oldcerr));

    std::cout.rdbuf (&coutsb);
    std::cerr.rdbuf (&cerrsb);

    LOG_INIT(logLevel);

    int players = mgr.getInt("players", "General");
    string addr = mgr.getString("address", "General");
    int port = mgr.getInt("port", "General");

    string passw = mgr.getString("password", "General");

    string plugin_home = mgr.getString("home", "Plugins");
    string moddir = Utils::convertPath(plugin_home + "/data");

    vector<string> plugins (Utils::split(mgr.getString("plugins", "Plugins"), ','));

    printVersion(TES3MP_VERSION, TES3MP_PROTO_VERSION);


    setenv("AMXFILE", moddir.c_str(), 1);
    setenv("MOD_DIR", moddir.c_str(), 1); // hack for lua

    setenv("LUA_PATH", Utils::convertPath(plugin_home + "/scripts/?.lua" + ";"
                                          + plugin_home + "/scripts/?.t" + ";"
                                          + plugin_home + "/lib/lua/?.lua" + ";"
                                          + plugin_home + "/lib/lua/?.t").c_str(), 1);
#ifdef _WIN32
    setenv("LUA_CPATH", Utils::convertPath(plugin_home + "/lib/?.dll").c_str(), 1);
#else
    setenv("LUA_CPATH", Utils::convertPath(plugin_home + "/lib/?.so").c_str(), 1);
#endif

    for (auto plugin : plugins)
        Script::LoadScript(plugin.c_str(), plugin_home.c_str());

    RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();

    stringstream sstr(TES3MP_VERSION);
    sstr << TES3MP_PROTO_VERSION;

    peer->SetIncomingPassword(sstr.str().c_str(), (int)sstr.str().size());

    if (RakNet::NonNumericHostString(addr.c_str()))
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, "You cannot use non-numeric addresses for the server.");
        return 1;
    }

    RakNet::SocketDescriptor sd((unsigned short)port, addr.c_str());
    if (peer->Startup((unsigned)players, &sd, 1) != RakNet::RAKNET_STARTED)
        return 1;

    peer->SetMaximumIncomingConnections((unsigned short)(players));

    Networking networking(peer);
    networking.setServerPassword(passw);

    if ( mgr.getBool("enabled", "MasterServer"))
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sharing server query info to master enabled.");
        string masterAddr = mgr.getString("address", "MasterServer");
        int masterPort = mgr.getInt("port", "MasterServer");

        networking.InitQuery(masterAddr, (unsigned short) masterPort, addr, (unsigned short) port);
        networking.getMasterClient()->SetMaxPlayers((unsigned)players);
        string hostname = mgr.getString("hostname", "General");
        networking.getMasterClient()->SetHostname(hostname);

        networking.getMasterClient()->Start();
    }

    int code = networking.mainLoop();

    RakNet::RakPeerInterface::DestroyInstance(peer);

    networking.getMasterClient()->Stop();

    if (code == 0)
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Quitting peacefully.");

    // Restore cout and cerr
    std::cout.rdbuf(cout_rdbuf);
    std::cerr.rdbuf(cerr_rdbuf);

    breakpad_close();
    return code;
}
