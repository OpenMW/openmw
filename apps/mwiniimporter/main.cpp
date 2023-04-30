#include "importer.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "CLI/CLI.hpp"

#include <components/files/conversion.hpp>

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
        CLI::App app("Syntax: openmw-iniimporter inifile configfile");

        app.get_formatter()->column_width(40);
        app.get_formatter()->label("ARGUMENTS", "ARGUMENTS");

        std::filesystem::path iniFile;
        app.add_option("-i, --ini", iniFile, "morrowind.ini file")->required()->check(CLI::ExistingFile);

        std::filesystem::path cfgFile;
        app.add_option("-c, --cfg", cfgFile, "openmw.cfg file")->required()->check(CLI::NonexistentPath);

        std::filesystem::path outputFile = "";
        app.add_option("-o,--output", outputFile, "openmw.cfg file")->default_str("")->check(CLI::NonexistentPath);

        bool gameFiles = false;
        app.add_flag("-g,--game-files", gameFiles, "import esm and esp files");

        bool fonts = false;
        app.add_flag("-f,--fonts", fonts, "import bitmap fonts");

        bool noArchives = false;
        app.add_flag("-A,--no-archives", noArchives, "disable bsa archives import");

        std::string gameEncoding = "win1252";
        app.add_option("-e,--encoding", gameEncoding,
               "Character encoding used in OpenMW game messages.\n"
               "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, "
               "Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
               "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
               "\n\twin1252 - Western European (Latin) alphabet, used by default")
            ->default_str("win1252")
            ->check(CLI::IsMember({ "win1250", "win1251", "win1252" }, CLI::ignore_case));

        bool verbose = false;
        app.add_flag("-v,--verbose", verbose, "verbose output");

        try
        {
            CLI11_PARSE(app, argc, argv)
        }
        catch (const CLI::ParseError& e)
        {
            std::cerr << e.what() << std::endl;
            std::cout << app.help() << std::endl;
            return app.exit(e);
        }

        if (outputFile.empty())
        {
            outputFile = cfgFile;
        }

        MwIniImporter importer;
        importer.setVerbose(verbose);

        // Font encoding settings
        importer.setInputEncoding(ToUTF8::calculateEncoding(gameEncoding));

        MwIniImporter::multistrmap ini = importer.loadIniFile(iniFile);
        MwIniImporter::multistrmap cfg = importer.loadCfgFile(cfgFile);

        if (!fonts)
        {
            ini.erase("Fonts:Font 0");
            ini.erase("Fonts:Font 1");
            ini.erase("Fonts:Font 2");
        }

        importer.merge(cfg, ini);
        importer.mergeFallback(cfg, ini);

        if (gameFiles)
        {
            importer.importGameFiles(cfg, ini, iniFile);
        }

        if (!noArchives)
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
