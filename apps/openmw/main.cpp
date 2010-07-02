#include <iostream>

#include <string>
#include <fstream>

#include <boost/program_options.hpp>

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
            "set initial cell (only interior cells supported at the moment")
        ("master", bpo::value<std::string>()->default_value ("Morrowind"),
            "master file")
        ;
  
    bpo::variables_map variables;
    
    std::ifstream configFile ("openmw.cfg");

    bpo::store ( bpo::parse_command_line (argc, argv, desc), variables );
    bpo::notify (variables);

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
