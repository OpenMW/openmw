#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include <boost/program_options.hpp>

#include <components/bsa/ba2dx10file.hpp>
#include <components/bsa/ba2gnrlfile.hpp>
#include <components/bsa/compressedbsafile.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/conversion.hpp>

#define BSATOOL_VERSION 1.1

// Create local aliases for brevity
namespace bpo = boost::program_options;

struct Arguments
{
    std::string mode;
    std::filesystem::path filename;
    std::filesystem::path extractfile;
    std::filesystem::path addfile;
    std::filesystem::path outdir;

    bool longformat;
    bool fullpath;
};

bool parseOptions(int argc, char** argv, Arguments& info)
{
    bpo::options_description desc(R"(Inspect and extract files from Bethesda BSA archives

Usages:
  bsatool list [-l] archivefile\n
      List the files presents in the input archive.

  bsatool extract [-f] archivefile [file_to_extract] [output_directory]
      Extract a file from the input archive.

  bsatool extractall archivefile [output_directory]
      Extract all files from the input archive.

  bsatool add [-a] archivefile file_to_add
      Add a file to the input archive.

  bsatool create [-c] archivefile
      Create an archive.
Allowed options)");

    auto addOption = desc.add_options();
    addOption("help,h", "print help message.");
    addOption("version,v", "print version information and quit.");
    addOption("long,l", "Include extra information in archive listing.");
    addOption("full-path,f", "Create directory hierarchy on file extraction (always true for extractall).");

    // input-file is hidden and used as a positional argument
    bpo::options_description hidden("Hidden Options");

    auto addHiddenOption = hidden.add_options();
    addHiddenOption("mode,m", bpo::value<std::string>(), "bsatool mode");
    addHiddenOption("input-file,i", bpo::value<Files::MaybeQuotedPathContainer>(), "input file");

    bpo::positional_options_description p;
    p.add("mode", 1).add("input-file", 3);

    // there might be a better way to do this
    bpo::options_description all;
    all.add(desc).add(hidden);

    bpo::variables_map variables;
    try
    {
        bpo::parsed_options validOpts = bpo::command_line_parser(argc, argv).options(all).positional(p).run();
        bpo::store(validOpts, variables);
    }
    catch (std::exception& e)
    {
        std::cout << "ERROR parsing arguments: " << e.what() << "\n\n" << desc << std::endl;
        return false;
    }

    bpo::notify(variables);

    if (variables.count("help"))
    {
        std::cout << desc << std::endl;
        return false;
    }
    if (variables.count("version"))
    {
        std::cout << "BSATool version " << BSATOOL_VERSION << std::endl;
        return false;
    }
    if (!variables.count("mode"))
    {
        std::cout << "ERROR: no mode specified!\n\n" << desc << std::endl;
        return false;
    }

    info.mode = variables["mode"].as<std::string>();
    if (!(info.mode == "list" || info.mode == "extract" || info.mode == "extractall" || info.mode == "add"
            || info.mode == "create"))
    {
        std::cout << std::endl << "ERROR: invalid mode \"" << info.mode << "\"\n\n" << desc << std::endl;
        return false;
    }

    if (!variables.count("input-file"))
    {
        std::cout << "\nERROR: missing BSA archive\n\n" << desc << std::endl;
        return false;
    }
    auto inputFiles = variables["input-file"].as<Files::MaybeQuotedPathContainer>();

    info.filename = inputFiles[0].u8string(); // This call to u8string is redundant, but required to build on MSVC 14.26
                                              // due to implementation bugs.

    // Default output to the working directory
    info.outdir = std::filesystem::current_path();

    if (info.mode == "extract")
    {
        if (inputFiles.size() < 2)
        {
            std::cout << "\nERROR: file to extract unspecified\n\n" << desc << std::endl;
            return false;
        }
        if (inputFiles.size() > 1)
            info.extractfile = inputFiles[1].u8string(); // This call to u8string is redundant, but required to build on
                                                         // MSVC 14.26 due to implementation bugs.
        if (inputFiles.size() > 2)
            info.outdir = inputFiles[2].u8string(); // This call to u8string is redundant, but required to build on
                                                    // MSVC 14.26 due to implementation bugs.
    }
    else if (info.mode == "add")
    {
        if (inputFiles.empty())
        {
            std::cout << "\nERROR: file to add unspecified\n\n" << desc << std::endl;
            return false;
        }
        if (inputFiles.size() > 1)
            info.addfile = inputFiles[1].u8string(); // This call to u8string is redundant, but required to build on
                                                     // MSVC 14.26 due to implementation bugs.
    }
    else if (inputFiles.size() > 1)
        info.outdir = inputFiles[1].u8string(); // This call to u8string is redundant, but required to build on
                                                // MSVC 14.26 due to implementation bugs.

    info.longformat = variables.count("long") != 0;
    info.fullpath = variables.count("full-path") != 0;

    return true;
}

template <typename File>
int list(std::unique_ptr<File>& bsa, Arguments& info)
{
    // List all files
    const auto& files = bsa->getList();
    for (const auto& file : files)
    {
        if (info.longformat)
        {
            // Long format
            std::ios::fmtflags f(std::cout.flags());
            std::cout << std::setw(50) << std::left << file.name();
            std::cout << std::setw(8) << std::left << std::dec << file.fileSize;
            std::cout << "@ 0x" << std::hex << file.offset << std::endl;
            std::cout.flags(f);
        }
        else
            std::cout << file.name() << std::endl;
    }

    return 0;
}

template <typename File>
int extract(std::unique_ptr<File>& bsa, Arguments& info)
{
    auto archivePath = info.extractfile.u8string();
    Misc::StringUtils::replaceAll(archivePath, u8"/", u8"\\");

    auto extractPath = info.extractfile.u8string();
    Misc::StringUtils::replaceAll(extractPath, u8"\\", u8"/");

    Files::IStreamPtr stream;
    // Get a stream for the file to extract
    for (auto it = bsa->getList().rbegin(); it != bsa->getList().rend(); ++it)
    {
        auto streamPath = Misc::StringUtils::stringToU8String(it->name());
        if (Misc::StringUtils::ciEqual(streamPath, archivePath) || Misc::StringUtils::ciEqual(streamPath, extractPath))
        {
            stream = bsa->getFile(&*it);
            break;
        }
    }
    if (!stream)
    {
        std::cout << "ERROR: file '" << Misc::StringUtils::u8StringToString(archivePath) << "' not found\n";
        std::cout << "In archive: " << Files::pathToUnicodeString(info.filename) << std::endl;
        return 3;
    }

    // Get the target path (the path the file will be extracted to)
    std::filesystem::path relPath(extractPath);

    std::filesystem::path target;
    if (info.fullpath)
        target = info.outdir / relPath;
    else
        target = info.outdir / relPath.filename();

    // Create the directory hierarchy
    std::filesystem::create_directories(target.parent_path());

    std::filesystem::file_status s = std::filesystem::status(target.parent_path());
    if (!std::filesystem::is_directory(s))
    {
        std::cout << "ERROR: " << Files::pathToUnicodeString(target.parent_path()) << " is not a directory."
                  << std::endl;
        return 3;
    }

    std::ofstream out(target, std::ios::binary);

    // Write the file to disk
    std::cout << "Extracting " << Files::pathToUnicodeString(info.extractfile) << " to "
              << Files::pathToUnicodeString(target) << std::endl;

    out << stream->rdbuf();
    out.close();

    return 0;
}

template <typename File>
int extractAll(std::unique_ptr<File>& bsa, Arguments& info)
{
    for (const auto& file : bsa->getList())
    {
        std::string extractPath(file.name());
        Misc::StringUtils::replaceAll(extractPath, "\\", "/");

        // Get the target path (the path the file will be extracted to)
        auto target = info.outdir;
        target /= Misc::StringUtils::stringToU8String(extractPath);

        // Create the directory hierarchy
        std::filesystem::create_directories(target.parent_path());

        std::filesystem::file_status s = std::filesystem::status(target.parent_path());
        if (!std::filesystem::is_directory(s))
        {
            std::cout << "ERROR: " << target.parent_path() << " is not a directory." << std::endl;
            return 3;
        }

        // Get a stream for the file to extract
        Files::IStreamPtr data = bsa->getFile(&file);
        std::ofstream out(target, std::ios::binary);

        // Write the file to disk
        std::cout << "Extracting " << Files::pathToUnicodeString(target) << std::endl;
        out << data->rdbuf();
        out.close();
    }

    return 0;
}

template <typename File>
int add(std::unique_ptr<File>& bsa, Arguments& info)
{
    std::fstream stream(info.addfile, std::ios_base::binary | std::ios_base::out | std::ios_base::in);
    bsa->addFile(Files::pathToUnicodeString(info.addfile), stream);

    return 0;
}

template <typename File>
int call(Arguments& info)
{
    std::unique_ptr<File> bsa = std::make_unique<File>();
    if (info.mode == "create")
    {
        bsa->open(info.filename);
        return 0;
    }

    bsa->open(info.filename);

    if (info.mode == "list")
        return list(bsa, info);
    else if (info.mode == "extract")
        return extract(bsa, info);
    else if (info.mode == "extractall")
        return extractAll(bsa, info);
    else if (info.mode == "add")
        return add(bsa, info);
    else
    {
        std::cout << "Unsupported mode. That is not supposed to happen." << std::endl;
        return 1;
    }
}

int main(int argc, char** argv)
{
    try
    {
        Arguments info;
        if (!parseOptions(argc, argv, info))
            return 1;

        // Open file

        // TODO: add a version argument for this mode after compressed BSA writing is a thing
        if (info.mode == "create")
            return call<Bsa::BSAFile>(info);

        Bsa::BsaVersion bsaVersion = Bsa::BSAFile::detectVersion(info.filename);

        switch (bsaVersion)
        {
            case Bsa::BsaVersion::Unknown:
                break;
            case Bsa::BsaVersion::Uncompressed:
                return call<Bsa::BSAFile>(info);
            case Bsa::BsaVersion::Compressed:
                return call<Bsa::CompressedBSAFile>(info);
            case Bsa::BsaVersion::BA2GNRL:
                return call<Bsa::BA2GNRLFile>(info);
            case Bsa::BsaVersion::BA2DX10:
                return call<Bsa::BA2DX10File>(info);
        }

        throw std::runtime_error("Unrecognised BSA archive");
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR reading BSA archive\nDetails:\n" << e.what() << std::endl;
        return 2;
    }
}
