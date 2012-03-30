#include "main.hpp"
#include "importer.hpp"

int main(int argc, char *argv[]) {

    bpo::options_description desc("Syntax: mwiniimporter <options>\nAllowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("in,i", bpo::value<std::string>()->required(), "morrowind.ini input file")
        ("out,o", bpo::value<std::string>()->required(), "openmw.cfg output file")
        ;
    
    bpo::variables_map vm;
    try {
        bpo::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        bpo::notify(vm);
        
    }
    catch(std::exception& e) {
        std::cout << "Error:" << e.what() << std::endl;
        return -1;
    }
    catch(...) {
        std::cout << "Error" << std::endl;
        return -1;
    }
    
    if(vm.count("help")) {
        std::cout << desc;
        return 0;
    }
    
    std::cout << "in:" << vm["in"].as<std::string>() << std::endl;
    
    MwIniImporter importer;
    importer.test();
    
    return 0;
}
