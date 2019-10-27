#include <switch.h>
#include <stdarg.h>
#include <SDL.h>

#include <components/files/configurationmanager.hpp>
#include <components/debug/debuglog.hpp>
// TODO: separate this into a component maybe
#include <apps/mwiniimporter/importer.hpp> 

#include "switch_startup.hpp"

#ifdef _DEBUG

static int nxlinkSock = -1;

extern "C" void userAppInit(void)
{
    if (R_SUCCEEDED(socketInitializeDefault()))
        nxlinkSock = nxlinkStdio();
}

extern "C" void userAppExit(void)
{
    if (nxlinkSock >= 0)
        close(nxlinkSock);
    socketExit();
}

#endif

namespace Switch
{
    std::string username = "";

    // read username from the account with the specified id
    void readUsername(u128 userId)
    {
        auto rc = accountInitialize();
        if (R_FAILED(rc))
            return;

        AccountProfile profile;
        rc = accountGetProfile(&profile, userId);

        if (R_SUCCEEDED(rc))
        {
            AccountProfileBase pb;
            rc = accountProfileGet(&profile, nullptr, &pb);

            if (R_SUCCEEDED(rc))
            {
                // HACK: check if the username is printable ascii
                // otherwise throw it out for the time being, since fs seems
                // allergic to certain UTF-8 characters 
                bool clean = true;
                for (size_t i = 0; i < sizeof(pb.username) && pb.username[i]; ++i)
                {
                    if (!std::isprint(pb.username[i]))
                    {
                        clean = false;
                        break;
                    }
                }

                if (clean)
                    username = std::string(pb.username);
            }

            accountProfileClose(&profile);
        }

        accountExit();
    }

    // taken from Checkpoint's account.cpp
    // pops up a user selection applet and gets the user id you select
    u128 selectProfile()
    {
        u128 out_id = 0;
        LibAppletArgs args;
        libappletArgsCreate(&args, 0x10000);
        u8 st_in[0xA0]  = {0};
        u8 st_out[0x18] = {0};
        size_t repsz;
        Result res = libappletLaunch(AppletId_playerSelect, &args, st_in, 0xA0, st_out, 0x18, &repsz);
        if (R_SUCCEEDED(res))
        {
            u64 lres = *(u64*)st_out;
            u128 uid = *(u128*)&st_out[8];
            if (lres == 0)
                out_id = uid;
        }
        return out_id;
    }

    void fatal(const char *fmt, ...)
    {
        FILE *f = fopen("fatal.log", "w");

        if (f)
        {
            fprintf(f, "FATAL ERROR:\n");
            va_list args;
            va_start(args, fmt);
            vfprintf(f, fmt, args);
            va_end(args);
            fclose(f);
        }

        Log(Debug::Error) << "FATAL ERROR! Check fatal.log.";
        exit(1);
    }

    void importIni(Files::ConfigurationManager& cfgMgr)
    {
        // if the cfg already exists, we don't need to perform conversion
        auto cfgPath = cfgMgr.getUserConfigPath() / "openmw.cfg";
        if (boost::filesystem::exists(cfgPath))
            return;

        // if the ini does not exist, we have nothing to convert
        auto dataPath = cfgMgr.getGlobalDataPath();
        auto iniPath = dataPath / "Morrowind.ini";
        if (!boost::filesystem::exists(iniPath))
            fatal(
                "Could not find `%s` or `%s`.\n"
                "Either manually place your data files into `%s` and your `openmw.cfg` into `%s`,\n"
                "or copy the `Data Files` directory and `Morrowind.ini` from your TES3 installation\n"
                "into `%s`.\n",
                cfgPath.c_str(),
                iniPath.c_str(),
                dataPath.c_str(),
                cfgMgr.getUserConfigPath().c_str(),
                dataPath.c_str()
            );

        Log(Debug::Info) << "Found Morrowind.ini, converting.";

        // perform ini => cfg conversion and save the cfg
        try
        {
            MwIniImporter importer;
            // TODO: figure out how to detect this
            importer.setInputEncoding(ToUTF8::WINDOWS_1252);

            MwIniImporter::multistrmap ini = importer.loadIniFile(iniPath);
            MwIniImporter::multistrmap cfg;

            // add a default searchpath for the importer just in case
            cfg.insert( std::make_pair( "data", std::vector<std::string>{"\"" + dataPath.string() + "\""} ) );

            importer.merge(cfg, ini);
            importer.mergeFallback(cfg, ini);
            importer.importGameFiles(cfg, ini, iniPath);
            importer.importArchives(cfg, ini);

            // add the Data Files folder if it exists
            auto dataDir = dataPath / "Data Files";
            if (boost::filesystem::is_directory(dataDir))
                cfg["data"].push_back("\"" + dataDir.string() + "\"");

            boost::filesystem::ofstream file(cfgPath);
            importer.writeToFile(file, cfg);
        }
        catch (std::exception& e)
        {
            fatal("While converting INI file:\n%s\n", e.what());
        }

        Log(Debug::Info) << "INI conversion done.";
    }

    void startup()
    {
        // some env vars for optimization purposes
        setenv("__GL_THREADED_OPTIMIZATIONS", "1", 1);
        setenv("__GL_ALLOW_UNOFFICIAL_PROTOCOL", "0", 1);
        setenv("__GL_NextGenCompiler", "0", 1);
        setenv("MESA_NO_ERROR", "1", 1);
        setenv("OPENMW_OPTIMIZE", "MERGE_GEOMETRY", 1);
        setenv("OPENMW_DONT_PRECOMPILE", "1", 1);
        setenv("OPENMW_DECOMPRESS_TEXTURES", "1", 1);

        // swap A, B and X, Y to correct positions
        SDL_GameControllerAddMapping(
            "53776974636820436F6E74726F6C6C65,Switch Controller,"
            "a:b0,b:b1,back:b11,"
            "dpdown:b15,dpleft:b12,dpright:b14,dpup:b13,"
            "leftshoulder:b6,leftstick:b4,lefttrigger:b8,leftx:a0,lefty:a1,"
            "rightshoulder:b7,rightstick:b5,righttrigger:b9,rightx:a2,righty:a3,"
            "start:b10,x:b2,y:b3"
        );

        // get and save the username for getUserDataPath
        readUsername(selectProfile());

        // unlocked in shutdown()
        appletLockExit();
    }

    std::string getUsername()
    {
        return username;
    }

    void shutdown()
    {
        appletUnlockExit();
    }

} /* namespace Switch */
