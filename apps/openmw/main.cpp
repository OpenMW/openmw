#include <iostream>
#include <cstdio>

#include <components/version/version.hpp>
#include <components/files/configurationmanager.hpp>

#include <SDL.h>
#include "engine.hpp"

#if defined(_WIN32) && !defined(_CONSOLE)
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream_buffer.hpp>

// For OutputDebugString
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
// makes __argc and __argv available on windows
#include <cstdlib>

#endif


#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <csignal>
extern int cc_install_handlers(int argc, char **argv, int num_signals, int *sigs, const char *logfile, int (*user_info)(char*, char*));
extern int is_debugger_attached(void);
#endif

// for Ogre::macBundlePath
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <OSX/macUtils.h>
#endif

#include <boost/version.hpp>
/**
 * Workaround for problems with whitespaces in paths in older versions of Boost library
 */
#if (BOOST_VERSION <= 104600)
namespace boost
{

template<>
inline boost::filesystem::path lexical_cast<boost::filesystem::path, std::string>(const std::string& arg)
{
    return boost::filesystem::path(arg);
}

} /* namespace boost */
#endif /* (BOOST_VERSION <= 104600) */

struct FallbackMap {
    std::map<std::string,std::string> mMap;
};

void validate(boost::any &v, std::vector<std::string> const &tokens, FallbackMap*, int)
{
    if(v.empty())
    {
        v = boost::any(FallbackMap());
    }

    FallbackMap *map = boost::any_cast<FallbackMap>(&v);

    std::map<std::string,std::string>::iterator mapIt;
    for(std::vector<std::string>::const_iterator it=tokens.begin(); it != tokens.end(); ++it)
    {
        int sep = it->find(",");
        if(sep < 1 || sep == (int)it->length()-1)
#if (BOOST_VERSION < 104200)
            throw boost::program_options::validation_error("invalid value");
#else
            throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
#endif

        std::string key(it->substr(0,sep));
        std::string value(it->substr(sep+1));

        if((mapIt = map->mMap.find(key)) == map->mMap.end())
        {
            map->mMap.insert(std::make_pair (key,value));
        }
    }
}


/**
 * \brief Parses application command line and calls \ref Cfg::ConfigurationManager
 * to parse configuration files.
 *
 * Results are directly written to \ref Engine class.
 *
 * \retval true - Everything goes OK
 * \retval false - Error
 */
bool parseOptions (int argc, char** argv, OMW::Engine& engine, Files::ConfigurationManager& cfgMgr)
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

        ("fallback-archive", bpo::value<StringsVector>()->default_value(StringsVector(), "fallback-archive")
            ->multitoken(), "set fallback BSA archives (later archives have higher priority)")

        ("resources", bpo::value<std::string>()->default_value("resources"),
            "set resources directory")

        ("start", bpo::value<std::string>()->default_value(""),
            "set initial cell")

        ("content", bpo::value<StringsVector>()->default_value(StringsVector(), "")
            ->multitoken(), "content file(s): esm/esp, or omwgame/omwaddon")

        ("no-sound", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "disable all sounds")

        ("script-verbose", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "verbose script output")

        ("script-all", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "compile all scripts (excluding dialogue scripts) at startup")

        ("script-console", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "enable console-only script functionality")

        ("script-run", bpo::value<std::string>()->default_value(""),
            "select a file containing a list of console commands that is executed on startup")

        ("skip-menu", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "skip main menu on game startup")

        ("fs-strict", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "strict file system handling (no case folding)")

        ( "encoding", bpo::value<std::string>()->
            default_value("win1252"),
            "Character encoding used in OpenMW game messages:\n"
            "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
            "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
            "\n\twin1252 - Western European (Latin) alphabet, used by default")

        ("fallback", bpo::value<FallbackMap>()->default_value(FallbackMap(), "")
            ->multitoken()->composing(), "fallback values")

        ("no-grab", "Don't grab mouse cursor")

        ("activate-dist", bpo::value <int> ()->default_value (-1), "activation distance override");

    bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv)
        .options(desc).allow_unregistered().run();

    bpo::variables_map variables;

    // Runtime options override settings from all configs
    bpo::store(valid_opts, variables);
    bpo::notify(variables);

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

    cfgMgr.readConfiguration(variables, desc);

    engine.setGrabMouse(!variables.count("no-grab"));

    // Font encoding settings
    std::string encoding(variables["encoding"].as<std::string>());
    std::cout << ToUTF8::encodingUsingMessage(encoding) << std::endl;
    engine.setEncoding(ToUTF8::calculateEncoding(encoding));

    // directory settings
    engine.enableFSStrict(variables["fs-strict"].as<bool>());

    Files::PathContainer dataDirs(variables["data"].as<Files::PathContainer>());

    std::string local(variables["data-local"].as<std::string>());
    if (!local.empty())
    {
        dataDirs.push_back(Files::PathContainer::value_type(local));
    }

    cfgMgr.processPaths(dataDirs);

    engine.setDataDirs(dataDirs);

    // fallback archives
    StringsVector archives = variables["fallback-archive"].as<StringsVector>();
    for (StringsVector::const_iterator it = archives.begin(); it != archives.end(); ++it)
    {
        engine.addArchive(*it);
    }

    engine.setResourceDir(variables["resources"].as<std::string>());

    StringsVector content = variables["content"].as<StringsVector>();
    if (content.empty())
    {
      std::cout << "No content file given (esm/esp, nor omwgame/omwaddon). Aborting..." << std::endl;
      return false;
    }

    StringsVector::const_iterator it(content.begin());
    StringsVector::const_iterator end(content.end());
    for (; it != end; ++it)
    {
      engine.addContentFile(*it);
    }

    // startup-settings
    engine.setCell(variables["start"].as<std::string>());
    engine.setSkipMenu (variables["skip-menu"].as<bool>());

    // other settings
    engine.setSoundUsage(!variables["no-sound"].as<bool>());
    engine.setScriptsVerbosity(variables["script-verbose"].as<bool>());
    engine.setCompileAll(variables["script-all"].as<bool>());
    engine.setFallbackValues(variables["fallback"].as<FallbackMap>().mMap);
    engine.setScriptConsoleMode (variables["script-console"].as<bool>());
    engine.setStartupScript (variables["script-run"].as<std::string>());
    engine.setActivationDistanceOverride (variables["activate-dist"].as<int>());

    return true;
}

int main(int argc, char**argv)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    // Unix crash catcher
    if ((argc == 2 && strcmp(argv[1], "--cc-handle-crash") == 0) || !is_debugger_attached())
    {
        int s[5] = { SIGSEGV, SIGILL, SIGFPE, SIGBUS, SIGABRT };
        cc_install_handlers(argc, argv, 5, s, "crash.log", NULL);
        std::cout << "Installing crash catcher" << std::endl;
    }
    else
        std::cout << "Running in a debugger, not installing crash catcher" << std::endl;
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    // set current dir to bundle path
    boost::filesystem::path bundlePath = boost::filesystem::path(Ogre::macBundlePath()).parent_path();
    boost::filesystem::current_path(bundlePath);
#endif

    try
    {
        Files::ConfigurationManager cfgMgr;
        OMW::Engine engine(cfgMgr);

        if (parseOptions(argc, argv, engine, cfgMgr))
        {
            engine.go();
        }
    }
    catch (std::exception &e)
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        if (isatty(fileno(stdin)) || !SDL_WasInit(SDL_INIT_VIDEO))
            std::cerr << "\nERROR: " << e.what() << std::endl;
        else
#endif
            SDL_ShowSimpleMessageBox(0, "OpenMW: Fatal error", e.what(), NULL);

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
