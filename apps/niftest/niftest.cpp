/// Program to test .nif files both on the FileSystem and in BSA archives.

#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <components/files/configurationmanager.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/nif/niffile.hpp>
#include <components/vfs/archive.hpp>
#include <components/vfs/bsaarchive.hpp>
#include <components/vfs/filesystemarchive.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/recursivedirectoryiterator.hpp>

#include <boost/program_options.hpp>

// Create local aliases for brevity
namespace bpo = boost::program_options;

/// See if the file has the named extension
bool hasExtension(const std::filesystem::path& filename, const std::string& extensionToFind)
{
    const auto extension = Files::pathToUnicodeString(filename.extension());
    return Misc::StringUtils::ciEqual(extension, extensionToFind);
}

/// See if the file has the "nif" extension.
bool isNIF(const std::filesystem::path& filename)
{
    return hasExtension(filename, ".nif") || hasExtension(filename, ".kf");
}
/// See if the file has the "bsa" extension.
bool isBSA(const std::filesystem::path& filename)
{
    return hasExtension(filename, ".bsa") || hasExtension(filename, ".ba2");
}

std::unique_ptr<VFS::Archive> makeBsaArchive(const std::filesystem::path& path)
{
    switch (Bsa::BSAFile::detectVersion(path))
    {
        case Bsa::BSAVER_COMPRESSED:
            return std::make_unique<VFS::ArchiveSelector<Bsa::BSAVER_COMPRESSED>::type>(path);
        case Bsa::BSAVER_BA2_GNRL:
            return std::make_unique<VFS::ArchiveSelector<Bsa::BSAVER_BA2_GNRL>::type>(path);
        case Bsa::BSAVER_BA2_DX10:
            return std::make_unique<VFS::ArchiveSelector<Bsa::BSAVER_BA2_DX10>::type>(path);
        case Bsa::BSAVER_UNCOMPRESSED:
            return std::make_unique<VFS::ArchiveSelector<Bsa::BSAVER_UNCOMPRESSED>::type>(path);
        case Bsa::BSAVER_UNKNOWN:
        default:
            std::cerr << "'" << Files::pathToUnicodeString(path) << "' is not a recognized BSA archive" << std::endl;
            return nullptr;
    }
}

std::unique_ptr<VFS::Archive> makeArchive(const std::filesystem::path& path)
{
    if (isBSA(path))
        return makeBsaArchive(path);
    if (std::filesystem::is_directory(path))
        return std::make_unique<VFS::FileSystemArchive>(path);
    return nullptr;
}

void readNIF(
    const std::filesystem::path& source, const std::filesystem::path& path, const VFS::Manager* vfs, bool quiet)
{
    const std::string pathStr = Files::pathToUnicodeString(path);
    if (!quiet)
    {
        if (hasExtension(path, ".kf"))
            std::cout << "Reading KF file '" << pathStr << "'";
        else
            std::cout << "Reading NIF file '" << pathStr << "'";
        if (!source.empty())
            std::cout << " from '" << Files::pathToUnicodeString(isBSA(source) ? source.filename() : source) << "'";
        std::cout << std::endl;
    }
    std::filesystem::path fullPath = !source.empty() ? source / path : path;
    try
    {
        Nif::NIFFile file(fullPath);
        Nif::Reader reader(file, nullptr);
        if (vfs != nullptr)
            reader.parse(vfs->get(pathStr));
        else
            reader.parse(Files::openConstrainedFileStream(fullPath));
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to read '" << pathStr << "':" << std::endl << e.what() << std::endl;
    }
}

/// Check all the nif files in a given VFS::Archive
/// \note Can not read a bsa file inside of a bsa file.
void readVFS(std::unique_ptr<VFS::Archive>&& archive, const std::filesystem::path& archivePath, bool quiet)
{
    if (archive == nullptr)
        return;

    if (!quiet)
        std::cout << "Reading data source '" << Files::pathToUnicodeString(archivePath) << "'" << std::endl;

    VFS::Manager vfs;
    vfs.addArchive(std::move(archive));
    vfs.buildIndex();

    for (const auto& name : vfs.getRecursiveDirectoryIterator(""))
    {
        if (isNIF(name))
        {
            readNIF(archivePath, name, &vfs, quiet);
        }
    }

    if (!archivePath.empty() && !isBSA(archivePath))
    {
        Files::PathContainer dataDirs = { archivePath };
        const Files::Collections fileCollections = Files::Collections(dataDirs);
        const Files::MultiDirCollection& bsaCol = fileCollections.getCollection(".bsa");
        const Files::MultiDirCollection& ba2Col = fileCollections.getCollection(".ba2");
        for (auto& file : bsaCol)
        {
            readVFS(makeBsaArchive(file.second), file.second, quiet);
        }
        for (auto& file : ba2Col)
        {
            readVFS(makeBsaArchive(file.second), file.second, quiet);
        }
    }
}

bool parseOptions(int argc, char** argv, Files::PathContainer& files, Files::PathContainer& archives,
    bool& writeDebugLog, bool& quiet)
{
    bpo::options_description desc(R"(Ensure that OpenMW can use the provided NIF, KF and BSA/BA2 files

Usages:
  niftest <nif files, kf files, BSA/BA2 files, or directories>
      Scan the file or directories for NIF errors.

Allowed options)");
    auto addOption = desc.add_options();
    addOption("help,h", "print help message.");
    addOption("write-debug-log,v", "write debug log for unsupported nif files");
    addOption("quiet,q", "do not log read archives/files");
    addOption("archives", bpo::value<Files::MaybeQuotedPathContainer>(), "path to archive files to provide files");
    addOption("input-file", bpo::value<Files::MaybeQuotedPathContainer>(), "input file");

    // Default option if none provided
    bpo::positional_options_description p;
    p.add("input-file", -1);

    bpo::variables_map variables;
    try
    {
        bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv).options(desc).positional(p).run();
        bpo::store(valid_opts, variables);
        bpo::notify(variables);
        if (variables.count("help"))
        {
            std::cout << desc << std::endl;
            return false;
        }
        writeDebugLog = variables.count("write-debug-log") > 0;
        quiet = variables.count("quiet") > 0;
        if (variables.count("input-file"))
        {
            files = asPathContainer(variables["input-file"].as<Files::MaybeQuotedPathContainer>());
            if (const auto it = variables.find("archives"); it != variables.end())
                archives = asPathContainer(it->second.as<Files::MaybeQuotedPathContainer>());
            return true;
        }
    }
    catch (std::exception& e)
    {
        std::cout << "Error parsing arguments: " << e.what() << "\n\n" << desc << std::endl;
        return false;
    }

    std::cout << "No input files or directories specified!" << std::endl;
    std::cout << desc << std::endl;
    return false;
}

int main(int argc, char** argv)
{
    Files::PathContainer files, sources;
    bool writeDebugLog = false;
    bool quiet = false;
    if (!parseOptions(argc, argv, files, sources, writeDebugLog, quiet))
        return 1;

    Nif::Reader::setLoadUnsupportedFiles(true);
    Nif::Reader::setWriteNifDebugLog(writeDebugLog);

    std::unique_ptr<VFS::Manager> vfs;
    if (!sources.empty())
    {
        vfs = std::make_unique<VFS::Manager>();
        for (const std::filesystem::path& path : sources)
        {
            const std::string pathStr = Files::pathToUnicodeString(path);
            if (!quiet)
                std::cout << "Adding data source '" << pathStr << "'" << std::endl;

            try
            {
                if (auto archive = makeArchive(path))
                    vfs->addArchive(std::move(archive));
                else
                    std::cerr << "Error: '" << pathStr << "' is not an archive or directory" << std::endl;
            }
            catch (std::exception& e)
            {
                std::cerr << "Failed to add data source '" << pathStr << "':  " << e.what() << std::endl;
            }
        }

        vfs->buildIndex();
    }

    for (const auto& path : files)
    {
        const std::string pathStr = Files::pathToUnicodeString(path);
        try
        {
            if (isNIF(path))
            {
                readNIF({}, path, vfs.get(), quiet);
            }
            else if (auto archive = makeArchive(path))
            {
                readVFS(std::move(archive), path, quiet);
            }
            else
            {
                std::cerr << "Error: '" << pathStr << "' is not a NIF/KF file, BSA/BA2 archive, or directory"
                          << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "Failed to read '" << pathStr << "':  " << e.what() << std::endl;
        }
    }
    return 0;
}
