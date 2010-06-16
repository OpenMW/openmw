#include <iostream>

#include <string>
#include <fstream>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "engine.hpp"

using namespace std;

void maintest (boost::filesystem::path dataDir, const std::string& cellName,
    std::string master)
{
    assert (!dataDir.empty());
  
    OMW::Engine engine;

    engine.setDataDir (dataDir);

    engine.setCell (cellName);

    engine.addMaster (master);

    engine.go();
}

int main(int argc, char**argv)
{
  try
  {
    boost::program_options::options_description desc (
      "Syntax: openmw <options>\nAllowed options");

    desc.add_options()
      ("help", "print help message")
      ("data", boost::program_options::value<std::string>()->default_value ("data"),
        "set data directory")
      ("start", boost::program_options::value<std::string>()->default_value ("Beshara"),
        "set initial cell (only interior cells supported at the moment")
      ("master", boost::program_options::value<std::string>()->default_value ("Morrowind"),
        "master file")
      ;
  
    boost::program_options::variables_map variables;
    
    std::ifstream configFile ("openmw.cfg");

    boost::program_options::store (
      boost::program_options::parse_command_line (argc, argv, desc), variables);
    boost::program_options::notify (variables);

    if (configFile.is_open())
      boost::program_options::store (
        boost::program_options::parse_config_file (configFile, desc), variables);

    if (variables.count ("help"))
    {
      std::cout << desc << std::endl;
    }
    else
    {          
      maintest (variables["data"].as<std::string>(), variables["start"].as<std::string>(),
        variables["master"].as<std::string>());
    }  
  }
  catch(exception &e)
    {
      cout << "\nERROR: " << e.what() << endl;
      return 1;
    }
  return 0;
}
