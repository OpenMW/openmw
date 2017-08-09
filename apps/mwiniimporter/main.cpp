#include "importer.hpp"

#include <iostream>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

#ifndef _WIN32
int main(int argc, char *argv[]) {
#else

// Include on Windows only
#include <boost/locale.hpp>

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
    char **get() const { return const_cast<char **>(argv); }

private:
    utf8argv(const utf8argv&);
    utf8argv& operator=(const utf8argv&);

    const char **argv;
    std::vector<std::string> args;
};

/*  The only way to pass Unicode on Winodws with CLI is to use wide
    characters interface which presents UTF-16 encoding. The rest of
    OpenMW application stack assumes UTF-8 encoding, therefore this
    conversion.

    For boost::filesystem::path::imbue see components/files/windowspath.cpp
*/
int wmain(int argc, wchar_t *wargv[]) {
    utf8argv converter(argc, wargv);
    char **argv = converter.get();
    boost::filesystem::path::imbue(boost::locale::generator().generate(""));
#endif

    try
    {
        bpo::options_description desc("Syntax: openmw-iniimporter <options> inifile configfile\nAllowed options");
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

        boost::filesystem::path iniFile(vm["ini"].as<std::string>());
        boost::filesystem::path cfgFile(vm["cfg"].as<std::string>());

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
        importer.setVerbose(vm.count("verbose") != 0);

        // Font encoding settings
        std::string encoding(vm["encoding"].as<std::string>());
        importer.setInputEncoding(ToUTF8::calculateEncoding(encoding));

        MwIniImporter::multistrmap ini = importer.loadIniFile(iniFile);
        MwIniImporter::multistrmap cfg = importer.loadCfgFile(cfgFile);

        importer.merge(cfg, ini);
        importer.mergeFallback(cfg, ini);

        if(vm.count("game-files")) {
            importer.importGameFiles(cfg, ini, iniFile);
        }

        if(!vm.count("no-archives")) {
            importer.importArchives(cfg, ini);
        }

        std::cout << "write to: " << outputFile << std::endl;
        bfs::ofstream file((bfs::path(outputFile)));
        importer.writeToFile(file, cfg);
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
    return 0;
}
