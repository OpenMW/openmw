#include "importer.hpp"

#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>

namespace bpo = boost::program_options;

#ifndef _WIN32
int main(int argc, char *argv[]) {
#else

class utf8argv
{
public:

    utf8argv(int argc, wchar_t *wargv[])
    {
        args.reserve(argc);
        argv = new const char *[argc];

        for (int i = 0; i < argc; ++i) {
            args.push_back(boost::locale::conv::utf_to_utf<char>(wargv[i]));
            argv[i] = args.back().c_str();
        }
    }

    ~utf8argv() { delete[] argv; }
    const char * const *get() const { return argv; }

private:

    const char **argv;
    std::vector<std::string> args;
};

int wmain(int argc, wchar_t *wargv[]) {
    utf8argv converter(argc, wargv);
    char **argv = converter.get();
    boost::filesystem::path::imbue(boost::locale::generator().generate(""));
#endif
    bpo::options_description desc("Syntax: mwiniimporter <options> inifile configfile\nAllowed options");
    bpo::positional_options_description p_desc;
    desc.add_options()
        ("help,h", "produce help message")
        ("verbose,v", "verbose output")
        ("ini,i", bpo::value<std::string>(), "morrowind.ini file")
        ("cfg,c", bpo::value<std::string>(), "openmw.cfg file")
        ("output,o", bpo::value<std::string>()->default_value(""), "openmw.cfg file")
        ("game-files,g", "import esm and esp files")
        ("no-archives,A", "disable bsa archives import")
        ("encoding,e", bpo::value<std::string>()-> default_value("win1252"),
            "Character encoding used in OpenMW game messages:\n"
            "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
            "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
            "\n\twin1252 - Western European (Latin) alphabet, used by default")
        ;
    p_desc.add("ini", 1).add("cfg", 1);

    bpo::variables_map vm;
    
    try
    {
        bpo::parsed_options parsed = bpo::command_line_parser(argc, argv)
            .options(desc)
            .positional(p_desc)
            .run();

        bpo::store(parsed, vm);
    }
    catch(boost::program_options::unknown_option & x)
    {
        std::cerr << "ERROR: " << x.what() << std::endl;
        return false;
    }
    catch(boost::program_options::invalid_command_line_syntax & x)
    {
        std::cerr << "ERROR: " << x.what() << std::endl;
        return false;
    }

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
    if(!boost::filesystem::exists(cfgFile))
        std::cerr << "cfg file does not exist" << std::endl;

    MwIniImporter importer;
    importer.setVerbose(vm.count("verbose"));

    // Font encoding settings
    std::string encoding(vm["encoding"].as<std::string>());
    importer.setInputEncoding(ToUTF8::calculateEncoding(encoding));

    MwIniImporter::multistrmap ini = importer.loadIniFile(iniFile);
    MwIniImporter::multistrmap cfg = importer.loadCfgFile(cfgFile);

    importer.merge(cfg, ini);
    importer.mergeFallback(cfg, ini);

    if(vm.count("game-files")) {
        importer.importGameFiles(cfg, ini);
    }

    if(!vm.count("no-archives")) {
        importer.importArchives(cfg, ini);
    }

    std::cout << "write to: " << outputFile << std::endl;
    boost::iostreams::stream<boost::iostreams::file_sink> file(outputFile);
    importer.writeToFile(file, cfg);

    return 0;
}
