#include <iostream>

#include <string>
#include <fstream>

#include <boost/program_options.hpp>

#include <components/misc/fileops.hpp>
#include "engine.hpp"

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
        ("start", bpo::value<std::string>()->default_value ("Beshara"),
            "set initial cell")
        ("master", bpo::value<std::string>()->default_value ("Morrowind"),
            "master file")
        ( "debug", "debug mode" )
        ( "nosound", "disable all sound" )
        ( "script-verbose", "verbose script output" )
        ( "new-game", "activate char gen/new game mechanics" )
        ( "script-all", "compile all scripts (excluding dialogue scripts) at startup")
        ;

    bpo::variables_map variables;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	std::string configFilePath(macBundlePath() + "/Contents/MacOS/openmw.cfg");
	std::ifstream configFile (configFilePath.c_str());
#else
	std::ifstream configFile ("openmw.cfg");
#endif

    bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv).options(desc).allow_unregistered().run();

    bpo::store(valid_opts, variables);
    bpo::notify(variables);

    if (configFile.is_open())
        bpo::store ( bpo::parse_config_file (configFile, desc), variables);

    if (variables.count ("help"))
    {
        std::cout << desc << std::endl;
        return false;
    }

    engine.setDataDir (variables["data"].as<std::string>());
    engine.setCell (variables["start"].as<std::string>());
    engine.addMaster (variables["master"].as<std::string>());

    if (variables.count ("debug"))
        engine.enableDebugMode();

    if (variables.count ("nosound"))
        engine.disableSound();

    if (variables.count ("script-verbose"))
        engine.enableVerboseScripts();

    if (variables.count ("new-game"))
        engine.setNewGame();

    if (variables.count ("script-all"))
        engine.setCompileAll (true);

    return true;
}

int main(int argc, char**argv)
{
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
