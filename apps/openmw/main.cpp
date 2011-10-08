#include <iostream>

#include <string>
#include <fstream>

#include <boost/program_options.hpp>

#include <components/files/fileops.hpp>
#include <components/files/path.hpp>
#include <components/files/collections.hpp>
#include <components/cfg/configurationmanager.hpp>

#include "engine.hpp"

#if defined(_WIN32) && !defined(_CONSOLE)
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#  if !defined(_DEBUG)
#  include <iostream>
#  include <fstream>
#  endif

// For OutputDebugString
#include <Windows.h>
// makes __argc and __argv available on windows
#include <stdlib.h>

#endif

// for Ogre::macBundlePath
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <OSX/macUtils.h>
#endif

#include "config.hpp"

using namespace std;

/**
 * \brief Parses application command line and calls \ref Cfg::ConfigurationManager
 * to parse configuration files.
 *
 * Results are directly written to \ref Engine class.
 *
 * \retval true - Everything goes OK
 * \retval false - Error
 */
bool parseOptions (int argc, char** argv, OMW::Engine& engine, Cfg::ConfigurationManager& cfgMgr)
{
    // Create a local alias for brevity
    namespace bpo = boost::program_options;
    typedef std::vector<std::string> StringsVector;

    bpo::options_description desc("Syntax: openmw <options>\nAllowed options");

    desc.add_options()
        ("help", "print help message")
        ("version", "print version information and quit")
        ("data", bpo::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")
            ->multitoken(), "set data directories (later directories have higher priority)")

        ("data-local", bpo::value<std::string>()->default_value(""),
            "set local data directory (highest priority)")

        ("resources", bpo::value<std::string>()->default_value("resources"),
            "set resources directory")

        ("start", bpo::value<std::string>()->default_value("Beshara"),
            "set initial cell")

        ("master", bpo::value<StringsVector>()->default_value(StringsVector(), "")
            ->multitoken(), "master file(s)")

        ("plugin", bpo::value<StringsVector>()->default_value(StringsVector(), "")
            ->multitoken(), "plugin file(s)")

        ("fps", boost::program_options::value<bool>()->implicit_value(true)
            ->default_value(false), "show fps counter")

        ("debug", boost::program_options::value<bool>()->implicit_value(true)
            ->default_value(false), "debug mode")

        ("nosound", boost::program_options::value<bool>()->implicit_value(true)
            ->default_value(false), "disable all sounds")

        ("script-verbose", boost::program_options::value<bool>()->implicit_value(true)
            ->default_value(false), "verbose script output")

        ("new-game", boost::program_options::value<bool>()->implicit_value(true)
            ->default_value(false), "activate char gen/new game mechanics")

        ("script-all", boost::program_options::value<bool>()->implicit_value(true)
            ->default_value(false), "compile all scripts (excluding dialogue scripts) at startup")

        ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)
            ->default_value(false), "strict file system handling (no case folding)")

        ( "encoding", boost::program_options::value<std::string>()->
            default_value("win1252"),
            "Character encoding used in OpenMW game messages:\n"
            "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
            "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
            "\n\twin1252 - Western European (Latin) alphabet, used by default")

        ("report-focus", boost::program_options::value<bool>()->implicit_value(true)
            ->default_value(false), "write name of focussed object to cout")
        ;

    bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv)
        .options(desc).allow_unregistered().run();

    bpo::variables_map variables;

    // Runtime options override settings from all configs
    bpo::store(valid_opts, variables);
    bpo::notify(variables);

    cfgMgr.readConfiguration(variables, desc);

    bool run = true;

    if (variables.count ("help"))
    {
        std::cout << desc << std::endl;
        run = false;
    }

    if (variables.count ("version"))
    {
        std::cout << "OpenMW version " << OPENMW_VERSION << std::endl;
        run = false;
    }

    if (!run)
        return false;

    // Font encoding settings
    std::string encoding(variables["encoding"].as<std::string>());
    if (encoding == "win1250")
    {
      std::cout << "Using Central and Eastern European font encoding." << std::endl;
      engine.setEncoding(encoding);
    }
    else if (encoding == "win1251")
    {
      std::cout << "Using Cyrillic font encoding." << std::endl;
      engine.setEncoding(encoding);
    }
    else
    {
      std::cout << "Using default (English) font encoding." << std::endl;
      engine.setEncoding("win1252");
    }

    // directory settings
    engine.enableFSStrict(variables["fs-strict"].as<bool>());

    Files::PathContainer dataDirs(variables["data"].as<Files::PathContainer>());

    std::string local(variables["data-local"].as<std::string>());
    if (!local.empty())
    {
        dataDirs.push_back(Files::PathContainer::value_type(local));
    }

    if (dataDirs.empty())
    {
        dataDirs.push_back(cfgMgr.getLocalDataPath());
    }

    engine.setDataDirs(dataDirs);

    engine.setResourceDir(variables["resources"].as<std::string>());

    // master and plugin
    StringsVector master = variables["master"].as<StringsVector>();
    if (master.empty())
    {
        std::cout << "No master file given. Assuming Morrowind.esm" << std::endl;
        master.push_back("Morrowind");
    }

    if (master.size() > 1)
    {
        std::cout
            << "Ignoring all but the first master file (multiple master files not yet supported)."
            << std::endl;
    }
    engine.addMaster(master[0]);

    StringsVector plugin = variables["plugin"].as<StringsVector>();
    if (!plugin.empty())
    {
        std::cout << "Ignoring plugin files (plugins not yet supported)." << std::endl;
    }

    // startup-settings
    engine.setCell(variables["start"].as<std::string>());
    engine.setNewGame(variables["new-game"].as<bool>());

    // other settings
    engine.showFPS(variables["fps"].as<bool>());
    engine.setDebugMode(variables["debug"].as<bool>());
    engine.setSoundUsage(!variables["nosound"].as<bool>());
    engine.setScriptsVerbosity(variables["script-verbose"].as<bool>());
    engine.setCompileAll(variables["script-all"].as<bool>());
    engine.setReportFocus(variables["report-focus"].as<bool>());

    return true;
}

int main(int argc, char**argv)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    // set current dir to bundle path
    boost::filesystem::path bundlePath = boost::filesystem::path(Ogre::macBundlePath()).parent_path();
    boost::filesystem::current_path(bundlePath);
#endif

    try
    {
        Cfg::ConfigurationManager cfgMgr;
        OMW::Engine engine(cfgMgr);

        if (parseOptions(argc, argv, engine, cfgMgr))
        {
            engine.go();
        }
    }
    catch (std::exception &e)
    {
        std::cout << "\nERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// Platform specific for Windows when there is no console built into the executable.
// Windows will call the WinMain function instead of main in this case, the normal
// main function is then called with the __argc and __argv parameters.
// In addition if it is a debug build it will redirect cout to the debug console in Visual Studio
#if defined(_WIN32) && !defined(_CONSOLE)

#if defined(_DEBUG)
class DebugOutput : public boost::iostreams::sink
{
public:
    std::streamsize write(const char *str, std::streamsize size)
    {
        // Make a copy for null termination
        std::string tmp (str, size);
        // Write string to Visual Studio Debug output
        OutputDebugString (tmp.c_str ());
        return size;
    }
};
#else
class Logger : public boost::iostreams::sink
{
public:
    Logger(std::ofstream &stream)
        : out(stream)
    {
    }

    std::streamsize write(const char *str, std::streamsize size)
    {
        out.write (str, size);
        out.flush();
        return size;
    }

private:
    std::ofstream &out;
};
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    std::streambuf* old_rdbuf = std::cout.rdbuf ();

    int ret = 0;
#if defined(_DEBUG)
    // Redirect cout to VS debug output when running in debug mode
    {
        boost::iostreams::stream_buffer<DebugOutput> sb;
        sb.open(DebugOutput());
#else
    // Redirect cout to openmw.log
    std::ofstream logfile ("openmw.log");
    {
        boost::iostreams::stream_buffer<Logger> sb;
        sb.open (Logger (logfile));
#endif
        std::cout.rdbuf (&sb);

        ret = main (__argc, __argv);

        std::cout.rdbuf(old_rdbuf);
    }
    return ret;
}

#endif
