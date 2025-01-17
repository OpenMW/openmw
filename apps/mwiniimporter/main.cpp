#include "importer.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include <components/files/configurationmanager.hpp>
#include <components/files/conversion.hpp>

namespace bpo = boost::program_options;
namespace sfs = std::filesystem;

#ifndef _WIN32
int main(int argc, char* argv[])
{
#else

// Include on Windows only
#include <codecvt>
#include <locale>

class utf8argv
{
public:
    utf8argv(int argc, wchar_t* wargv[])
    {
        args.reserve(argc);
        argv = new const char*[argc];

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        for (int i = 0; i < argc; ++i)
        {
            args.push_back(converter.to_bytes(wargv[i]));
            argv[i] = args.back().c_str();
        }
    }

    ~utf8argv() { delete[] argv; }
    char** get() const { return const_cast<char**>(argv); }

private:
    utf8argv(const utf8argv&);
    utf8argv& operator=(const utf8argv&);

    const char** argv;
    std::vector<std::string> args;
};

/*  The only way to pass Unicode on Winodws with CLI is to use wide
    characters interface which presents UTF-16 encoding. The rest of
    OpenMW application stack assumes UTF-8 encoding, therefore this
    conversion.
*/
int wmain(int argc, wchar_t* wargv[])
{
    utf8argv converter(argc, wargv);
    char** argv = converter.get();
#endif

    try
    {
        bpo::options_description desc("Syntax: openmw-iniimporter <options> inifile configfile\nAllowed options");
        bpo::positional_options_description p_desc;
        auto addOption = desc.add_options();
        addOption("help,h", "produce help message");
        addOption("verbose,v", "verbose output");
        addOption("ini,i", bpo::value<Files::MaybeQuotedPath>(), "morrowind.ini file");
        addOption("cfg,c", bpo::value<Files::MaybeQuotedPath>(), "openmw.cfg file");
        addOption("output,o", bpo::value<Files::MaybeQuotedPath>()->default_value({}), "openmw.cfg file");
        addOption("game-files,g", "import esm and esp files");
        addOption("fonts,f", "import bitmap fonts");
        addOption("no-archives,A", "disable bsa archives import");
        addOption("encoding,e", bpo::value<std::string>()->default_value("win1252"),
            "Character encoding used in OpenMW game messages:\n"
            "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, "
            "Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
            "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
            "\n\twin1252 - Western European (Latin) alphabet, used by default");
        ;
        p_desc.add("ini", 1).add("cfg", 1);

        bpo::variables_map vm;

        bpo::parsed_options parsed = bpo::command_line_parser(argc, argv).options(desc).positional(p_desc).run();
        bpo::store(parsed, vm);

        if (vm.count("help") || !vm.count("ini") || !vm.count("cfg"))
        {
            std::cout << desc;
            return 0;
        }

        bpo::notify(vm);

        std::filesystem::path iniFile(
            vm["ini"].as<Files::MaybeQuotedPath>().u8string()); // This call to u8string is redundant, but required to
                                                                // build on MSVC 14.26 due to implementation bugs.
        std::filesystem::path cfgFile(
            vm["cfg"].as<Files::MaybeQuotedPath>().u8string()); // This call to u8string is redundant, but required to
                                                                // build on MSVC 14.26 due to implementation bugs.

        // if no output is given, write back to cfg file
        std::filesystem::path outputFile = vm["output"]
                                               .as<Files::MaybeQuotedPath>()
                                               .u8string(); // This call to u8string is redundant, but required to build
                                                            // on MSVC 14.26 due to implementation bugs.
        if (vm["output"].defaulted())
        {
            outputFile = vm["cfg"]
                             .as<Files::MaybeQuotedPath>()
                             .u8string(); // This call to u8string is redundant, but required to build on MSVC 14.26 due
                                          // to implementation bugs.
        }

        if (!std::filesystem::exists(iniFile))
        {
            std::cerr << "ini file does not exist" << std::endl;
            return -3;
        }
        if (!std::filesystem::exists(cfgFile))
            std::cerr << "cfg file does not exist" << std::endl;

        MwIniImporter importer;
        importer.setVerbose(vm.count("verbose") != 0);

        MwIniImporter::multistrmap cfg = importer.loadCfgFile(cfgFile);

        // Font encoding settings
        std::string encoding;
        if (vm["encoding"].defaulted() && cfg.contains("encoding") && !cfg["encoding"].empty())
            encoding = cfg["encoding"].back();
        else
        {
            encoding = vm["encoding"].as<std::string>();
            cfg["encoding"] = { encoding };
        }
        importer.setInputEncoding(ToUTF8::calculateEncoding(encoding));

        MwIniImporter::multistrmap ini = importer.loadIniFile(iniFile);

        if (!vm.count("fonts"))
        {
            ini.erase("Fonts:Font 0");
            ini.erase("Fonts:Font 1");
            ini.erase("Fonts:Font 2");
        }

        importer.merge(cfg, ini);
        importer.mergeFallback(cfg, ini);

        if (vm.count("game-files"))
        {
            importer.importGameFiles(cfg, ini, iniFile);
        }

        if (!vm.count("no-archives"))
        {
            importer.importArchives(cfg, ini);
        }

        std::cout << "write to: " << Files::pathToUnicodeString(outputFile) << std::endl;
        std::ofstream file(outputFile);
        importer.writeToFile(file, cfg);
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
    return 0;
}
