#include <iostream>

#include <string>
#include <fstream>

#include <boost/program_options.hpp>

#include <components/misc/fileops.hpp>
#include "engine.hpp"
#include "path.hpp"

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

using namespace std;

/// Parse command line options and openmw.cfg file (if one exists). Results are directly
/// written to \a engine.
/// \return Run OpenMW?

bool parseOptions (int argc, char**argv, OMW::Engine& engine)
{
    // Create a local alias for brevity
    namespace bpo = boost::program_options;

    bpo::options_description desc (
        "Syntax: openmw <options>\nAllowed options");

    desc.add_options()
        ("help", "print help message")
        ("data", bpo::value<std::string>()->default_value ("data"),
            "set data directory")
        ("resources", bpo::value<std::string>()->default_value ("resources"),
            "set resources directory")
        ("start", bpo::value<std::string>()->default_value ("Beshara"),
            "set initial cell")
        ("master", bpo::value<std::vector<std::string> >()
            ->default_value (std::vector<std::string>(), "")
            ->multitoken(),
            "master file(s)")
        ("plugin", bpo::value<std::vector<std::string> >()
            ->default_value (std::vector<std::string>(), "")
            ->multitoken(),
            "plugin file(s)")
        ( "fps", "show fps counter")
        ( "debug", "debug mode" )
        ( "nosound", "disable all sound" )
        ( "script-verbose", "verbose script output" )
        ( "new-game", "activate char gen/new game mechanics" )
        ( "script-all", "compile all scripts (excluding dialogue scripts) at startup")
        ;

    bpo::variables_map variables;

    std::string cfgFile = OMW::Path::getPath(OMW::Path::GLOBAL_CFG_PATH, "openmw", "openmw.cfg");
    std::cout << "Using global config file: " << cfgFile << std::endl;
    std::ifstream globalConfigFile(cfgFile.c_str());

    cfgFile = OMW::Path::getPath(OMW::Path::USER_CFG_PATH, "openmw", "openmw.cfg");
    std::cout << "Using user config file: " << cfgFile << std::endl;
    std::ifstream userConfigFile(cfgFile.c_str());

    bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv).options(desc).allow_unregistered().run();

    bpo::store(valid_opts, variables);
    bpo::notify(variables);

    if (userConfigFile.is_open())
        bpo::store ( bpo::parse_config_file (userConfigFile, desc), variables);
    if (globalConfigFile.is_open())
        bpo::store ( bpo::parse_config_file (globalConfigFile, desc), variables);

    if (variables.count ("help"))
    {
        std::cout << desc << std::endl;
        return false;
    }

    // directory settings
    engine.setDataDir (variables["data"].as<std::string>());
    engine.setResourceDir (variables["resources"].as<std::string>());

    // master and plugin
    std::vector<std::string> master = variables["master"].as<std::vector<std::string> >();
    if (master.empty())
    {
        std::cout << "No master file given. Assuming Morrowind.esm" << std::endl;
        master.push_back ("Morrowind");
    }

    if (master.size()>1)
    {
        std::cout
            << "Ignoring all but the first master file (multiple master files not yet supported)."
            << std::endl;
    }

    engine.addMaster (master[0]);

    std::vector<std::string> plugin = variables["plugin"].as<std::vector<std::string> >();

    if (!plugin.empty())
        std::cout << "Ignoring plugin files (plugins not yet supported)." << std::endl;

    // startup-settings
    engine.setCell (variables["start"].as<std::string>());

    if (variables.count ("new-game"))
        engine.setNewGame();

    // other settings
    if (variables.count ("fps"))
        engine.showFPS();

    if (variables.count ("debug"))
        engine.enableDebugMode();

    if (variables.count ("no-sound"))
        engine.disableSound();

    if (variables.count ("script-verbose"))
        engine.enableVerboseScripts();

    if (variables.count ("script-all"))
        engine.setCompileAll (true);

    return true;
}

int main(int argc, char**argv)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    // set current dir to bundle path
    boost::filesystem::path bundlePath = boost::filesystem::path(Ogre::macBundlePath());
    boost::filesystem::current_path(bundlePath);
#endif

    try
    {
        OMW::Engine engine;

        if (parseOptions (argc, argv, engine))
        {
            engine.go();
        }
    }
    catch(exception &e)
    {
        cout << "\nERROR: " << e.what() << endl;
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
