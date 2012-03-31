#include "importer.hpp"

#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace bpo = boost::program_options;

int main(int argc, char *argv[]) {

    bpo::options_description desc("Syntax: mwiniimporter <options>\nAllowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("verbose,v", "verbose output")
        ("ini,i", bpo::value<std::string>(), "morrowind.ini file")
        ("cfg,c", bpo::value<std::string>(), "openmw.cfg file")
        ("output,o", bpo::value<std::string>()->default_value(""), "openmw.cfg file")
        ("game-files,g", "import esm and esp files")
        ;

    bpo::variables_map vm;
    try {
        bpo::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

        // parse help before calling notify because we dont want it to throw an error if help is set
        if(vm.count("help")) {
            std::cout << desc;
            return 0;
        }

        bpo::notify(vm);

    }
    catch(std::exception& e) {
        std::cerr << "Error:" << e.what() << std::endl;
        return -1;
    }
    catch(...) {
        std::cerr << "Error" << std::endl;
        return -2;
    }

    std::string iniFile = vm["ini"].as<std::string>();
    std::string cfgFile = vm["cfg"].as<std::string>();

    // if no output is given, write back to cfg file
    std::string outputFile(vm["output"].as<std::string>());
    if(vm["output"].defaulted()) {
        outputFile = vm["cfg"].as<std::string>();
    }

    if(!boost::filesystem::exists(iniFile)) {
        std::cerr << "ini file does not exist" << std::endl;
        return -3;
    }
    if(!boost::filesystem::exists(cfgFile)) {
        std::cerr << "cfg file does not exist" << std::endl;
        return -4;
    }

    MwIniImporter importer;
    importer.setVerbose(vm.count("verbose"));

    std::map<std::string, std::string>ini = importer.loadIniFile(iniFile);
    std::map<std::string, std::string>cfg = importer.loadCfgFile(cfgFile);

    importer.merge(cfg, ini);
    
    if(vm.count("game-files")) {
        importer.importGameFiles(cfg, ini);
    }

    std::cout << "write to: " << outputFile << std::endl;
    importer.writeToFile(outputFile, cfg);

    return 0;
}
