#include "importer.hpp"

#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace bpo = boost::program_options;

int main(int argc, char *argv[]) {

    bpo::options_description desc("Syntax: mwiniimporter <options> inifile configfile\nAllowed options");
    bpo::positional_options_description p_desc;
    desc.add_options()
        ("help,h", "produce help message")
        ("verbose,v", "verbose output")
        ("ini,i", bpo::value<std::string>(), "morrowind.ini file")
        ("cfg,c", bpo::value<std::string>(), "openmw.cfg file")
        ("output,o", bpo::value<std::string>()->default_value(""), "openmw.cfg file")
        ("game-files,g", "import esm and esp files")
        ;
    p_desc.add("ini", 1).add("cfg", 1);

    bpo::variables_map vm;
    bpo::parsed_options parsed = bpo::command_line_parser(argc, argv)
        .options(desc)
        .positional(p_desc)
        .run();
    
    bpo::store(parsed, vm);

    if(vm.count("help") || !vm.count("ini") || !vm.count("cfg")) {
        std::cout << desc;
        return 0;
    }

    bpo::notify(vm);

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

    MwIniImporter::multistrmap ini = importer.loadIniFile(iniFile);
    MwIniImporter::multistrmap cfg = importer.loadCfgFile(cfgFile);

    importer.merge(cfg, ini);
    importer.mergeFallback(cfg, ini);

    if(vm.count("game-files")) {
        importer.importGameFiles(cfg, ini);
    }

    std::cout << "write to: " << outputFile << std::endl;
    boost::iostreams::stream<boost::iostreams::file_sink> file(outputFile);
    importer.writeToFile(file, cfg);

    return 0;
}
