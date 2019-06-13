#include <components/version/version.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/files/escape.hpp>
#include <components/fallback/fallback.hpp>
#include <components/fallback/validate.hpp>
#include <components/debug/debugging.hpp>
#include <components/misc/rng.hpp>

#include "engine.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// makes __argc and __argv available on windows
#include <cstdlib>
#endif

#if (defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix))
#include <unistd.h>
#endif

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


using namespace Fallback;

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
        ("data", bpo::value<Files::EscapePathContainer>()->default_value(Files::EscapePathContainer(), "data")
            ->multitoken()->composing(), "set data directories (later directories have higher priority)")

            ("data-local", bpo::value<Files::EscapeHashString>()->default_value(""),
            "set local data directory (highest priority)")

        ("fallback-archive", bpo::value<Files::EscapeStringVector>()->default_value(Files::EscapeStringVector(), "fallback-archive")
            ->multitoken(), "set fallback BSA archives (later archives have higher priority)")

            ("resources", bpo::value<Files::EscapeHashString>()->default_value("resources"),
            "set resources directory")

            ("start", bpo::value<Files::EscapeHashString>()->default_value(""),
            "set initial cell")

        ("content", bpo::value<Files::EscapeStringVector>()->default_value(Files::EscapeStringVector(), "")
            ->multitoken(), "content file(s): esm/esp, or omwgame/omwaddon")

        ("no-sound", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "disable all sounds")

        ("script-all", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "compile all scripts (excluding dialogue scripts) at startup")

        ("script-all-dialogue", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "compile all dialogue scripts at startup")

        ("script-console", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "enable console-only script functionality")

            ("script-run", bpo::value<Files::EscapeHashString>()->default_value(""),
            "select a file containing a list of console commands that is executed on startup")

        ("script-warn", bpo::value<int>()->implicit_value (1)
            ->default_value (1),
            "handling of warnings when compiling scripts\n"
            "\t0 - ignore warning\n"
            "\t1 - show warning but consider script as correctly compiled anyway\n"
            "\t2 - treat warnings as errors")

        ("script-blacklist", bpo::value<Files::EscapeStringVector>()->default_value(Files::EscapeStringVector(), "")
            ->multitoken(), "ignore the specified script (if the use of the blacklist is enabled)")

        ("script-blacklist-use", bpo::value<bool>()->implicit_value(true)
            ->default_value(true), "enable script blacklisting")

            ("load-savegame", bpo::value<Files::EscapeHashString>()->default_value(""),
            "load a save game file on game startup (specify an absolute filename or a filename relative to the current working directory)")

        ("skip-menu", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "skip main menu on game startup")

        ("new-game", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "run new game sequence (ignored if skip-menu=0)")

        ("fs-strict", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "strict file system handling (no case folding)")

            ("encoding", bpo::value<Files::EscapeHashString>()->
            default_value("win1252"),
            "Character encoding used in OpenMW game messages:\n"
            "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
            "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
            "\n\twin1252 - Western European (Latin) alphabet, used by default")

            ("fallback", bpo::value<FallbackMap>()->default_value(FallbackMap(), "")
            ->multitoken()->composing(), "fallback values")

        ("no-grab", bpo::value<bool>()->implicit_value(true)->default_value(false), "Don't grab mouse cursor")

        ("export-fonts", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "Export Morrowind .fnt fonts to PNG image and XML file in current directory")

        ("activate-dist", bpo::value <int> ()->default_value (-1), "activation distance override")

        ("random-seed", bpo::value <unsigned int> ()
            ->default_value(Misc::Rng::generateDefaultSeed()),
            "seed value for random number generator")
    ;

    bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv)
        .options(desc).allow_unregistered().run();

    bpo::variables_map variables;

    // Runtime options override settings from all configs
    bpo::store(valid_opts, variables);
    bpo::notify(variables);

    if (variables.count ("help"))
    {
        std::cout << desc << std::endl;
        return false;
    }

    if (variables.count ("version"))
    {
        cfgMgr.readConfiguration(variables, desc, true);

        Version::Version v = Version::getOpenmwVersion(variables["resources"].as<Files::EscapeHashString>().toStdString());
        std::cout << v.describe() << std::endl;
        return false;
    }

    cfgMgr.readConfiguration(variables, desc);

    Version::Version v = Version::getOpenmwVersion(variables["resources"].as<Files::EscapeHashString>().toStdString());
    std::cout << v.describe() << std::endl;

    engine.setGrabMouse(!variables["no-grab"].as<bool>());

    // Font encoding settings
    std::string encoding(variables["encoding"].as<Files::EscapeHashString>().toStdString());
    std::cout << ToUTF8::encodingUsingMessage(encoding) << std::endl;
    engine.setEncoding(ToUTF8::calculateEncoding(encoding));

    // directory settings
    engine.enableFSStrict(variables["fs-strict"].as<bool>());

    Files::PathContainer dataDirs(Files::EscapePath::toPathContainer(variables["data"].as<Files::EscapePathContainer>()));

    std::string local(variables["data-local"].as<Files::EscapeHashString>().toStdString());
    if (!local.empty())
    {
        if (local.front() == '\"')
            local = local.substr(1, local.length() - 2);

        dataDirs.push_back(Files::PathContainer::value_type(local));
    }

    cfgMgr.processPaths(dataDirs);

    engine.setResourceDir(variables["resources"].as<Files::EscapeHashString>().toStdString());
    engine.setDataDirs(dataDirs);

    // fallback archives
    StringsVector archives = variables["fallback-archive"].as<Files::EscapeStringVector>().toStdStringVector();
    for (StringsVector::const_iterator it = archives.begin(); it != archives.end(); ++it)
    {
        engine.addArchive(*it);
    }

    StringsVector content = variables["content"].as<Files::EscapeStringVector>().toStdStringVector();
    if (content.empty())
    {
        Log(Debug::Error) << "No content file given (esm/esp, nor omwgame/omwaddon). Aborting...";
        return false;
    }

    StringsVector::const_iterator it(content.begin());
    StringsVector::const_iterator end(content.end());
    for (; it != end; ++it)
    {
      engine.addContentFile(*it);
    }

    // startup-settings
    engine.setCell(variables["start"].as<Files::EscapeHashString>().toStdString());
    engine.setSkipMenu (variables["skip-menu"].as<bool>(), variables["new-game"].as<bool>());
    if (!variables["skip-menu"].as<bool>() && variables["new-game"].as<bool>())
        Log(Debug::Warning) << "Warning: new-game used without skip-menu -> ignoring it";

    // scripts
    engine.setCompileAll(variables["script-all"].as<bool>());
    engine.setCompileAllDialogue(variables["script-all-dialogue"].as<bool>());
    engine.setScriptConsoleMode (variables["script-console"].as<bool>());
    engine.setStartupScript (variables["script-run"].as<Files::EscapeHashString>().toStdString());
    engine.setWarningsMode (variables["script-warn"].as<int>());
    engine.setScriptBlacklist (variables["script-blacklist"].as<Files::EscapeStringVector>().toStdStringVector());
    engine.setScriptBlacklistUse (variables["script-blacklist-use"].as<bool>());
    engine.setSaveGameFile (variables["load-savegame"].as<Files::EscapeHashString>().toStdString());

    // other settings
    Fallback::Map::init(variables["fallback"].as<FallbackMap>().mMap);
    engine.setSoundUsage(!variables["no-sound"].as<bool>());
    engine.setActivationDistanceOverride (variables["activate-dist"].as<int>());
    engine.enableFontExport(variables["export-fonts"].as<bool>());
    engine.setRandomSeed(variables["random-seed"].as<unsigned int>());

    return true;
}

int runApplication(int argc, char *argv[])
{
#ifdef __APPLE__
    boost::filesystem::path binary_path = boost::filesystem::system_complete(boost::filesystem::path(argv[0]));
    boost::filesystem::current_path(binary_path.parent_path());
    setenv("OSG_GL_TEXTURE_STORAGE", "OFF", 0);
#endif

    Files::ConfigurationManager cfgMgr;
    std::unique_ptr<OMW::Engine> engine;
    engine.reset(new OMW::Engine(cfgMgr));

    if (parseOptions(argc, argv, *engine, cfgMgr))
    {
        engine->go();
    }

    return 0;
}

#ifdef ANDROID
extern "C" int SDL_main(int argc, char**argv)
#else
int main(int argc, char**argv)
#endif
{
    return wrapApplication(&runApplication, argc, argv, "OpenMW");
}

// Platform specific for Windows when there is no console built into the executable.
// Windows will call the WinMain function instead of main in this case, the normal
// main function is then called with the __argc and __argv parameters.
#if defined(_WIN32) && !defined(_CONSOLE)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    return main(__argc, __argv);
}
#endif
