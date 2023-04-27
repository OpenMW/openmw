#include "importer.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <cxxopts.hpp>

#include <components/files/configurationmanager.hpp>
#include <components/files/conversion.hpp>

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
        cxxopts::Options options("Syntax: openmw-iniimporter <options> inifile configfile\nAllowed options");

        // clang-format off
        options.add_options()
            ("h,help", "produce help message")
            ("v,verbose", "verbose output")
            ("i,ini", "morrowind.ini file", cxxopts::value<Files::MaybeQuotedPath>())
            ("c,cfg", "openmw.cfg file", cxxopts::value<Files::MaybeQuotedPath>())
            ("o,output", "openmw.cfg file", cxxopts::value<Files::MaybeQuotedPath>()->default_value(""))
            ("g,game-files", "import esm and esp files")
            ("f,fonts", "import bitmap fonts")
            ("A,no-archives", "disable bsa archives import")
            ("e,encoding",
                "Character encoding used in OpenMW game messages\n"
                "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, "
                "Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
                "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
                "\n\twin1252 - Western European (Latin) alphabet\n\n",
                cxxopts::value<std::string>()->default_value("win1252"))
        ;
        // clang-format on

        options.parse_positional({ "ini", "cfg" });

        auto result = options.parse(argc, argv);
        if (result.count("help") || !result.count("ini") || !result.count("cfg"))
        {
            std::cout << options.help() << std::endl;
            return 0;
        }

        std::filesystem::path iniFile(result["ini"].as<Files::MaybeQuotedPath>().u8string());
        std::filesystem::path cfgFile(result["cfg"].as<Files::MaybeQuotedPath>().u8string());
        std::filesystem::path outputFile = result["output"].as<Files::MaybeQuotedPath>().u8string();
        if (outputFile.empty())
        {
            outputFile = cfgFile;
        }

        if (!std::filesystem::exists(iniFile))
        {
            std::cerr << "ini file does not exist" << std::endl;
            return -3;
        }
        if (!std::filesystem::exists(cfgFile))
            std::cerr << "cfg file does not exist" << std::endl;

        MwIniImporter importer;
        importer.setVerbose(result["verbose"].as<bool>());

        // Font encoding settings
        std::string encoding(result["encoding"].as<std::string>());
        importer.setInputEncoding(ToUTF8::calculateEncoding(encoding));

        MwIniImporter::multistrmap ini = importer.loadIniFile(iniFile);
        MwIniImporter::multistrmap cfg = importer.loadCfgFile(cfgFile);

        if (!result["fonts"].as<bool>())
        {
            ini.erase("Fonts:Font 0");
            ini.erase("Fonts:Font 1");
            ini.erase("Fonts:Font 2");
        }

        importer.merge(cfg, ini);
        importer.mergeFallback(cfg, ini);

        if (result["game-files"].as<bool>())
        {
            importer.importGameFiles(cfg, ini, iniFile);
        }

        if (!result["no-archives"].as<bool>())
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
