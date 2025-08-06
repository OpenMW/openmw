/// Program to test .nif files both on the FileSystem and in BSA archives.

#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <components/bgsm/file.hpp>
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

enum class FileType
{
    BSA,
    BA2,
    BGEM,
    BGSM,
    NIF,
    KF,
    BTO,
    BTR,
    RDT,
    PSA,
    Unknown,
};

enum class FileClass
{
    Archive,
    Material,
    NIF,
    Unknown,
};

std::pair<FileType, FileClass> classifyFile(const std::filesystem::path& filename)
{
    const std::string extension = Misc::StringUtils::lowerCase(Files::pathToUnicodeString(filename.extension()));
    if (extension == ".bsa")
        return { FileType::BSA, FileClass::Archive };
    if (extension == ".ba2")
        return { FileType::BA2, FileClass::Archive };
    if (extension == ".bgem")
        return { FileType::BGEM, FileClass::Material };
    if (extension == ".bgsm")
        return { FileType::BGSM, FileClass::Material };
    if (extension == ".nif")
        return { FileType::NIF, FileClass::NIF };
    if (extension == ".kf")
        return { FileType::KF, FileClass::NIF };
    if (extension == ".bto")
        return { FileType::BTO, FileClass::NIF };
    if (extension == ".btr")
        return { FileType::BTR, FileClass::NIF };
    if (extension == ".rdt")
        return { FileType::RDT, FileClass::NIF };
    if (extension == ".psa")
        return { FileType::PSA, FileClass::NIF };

    return { FileType::Unknown, FileClass::Unknown };
}

std::string getFileTypeName(FileType fileType)
{
    switch (fileType)
    {
        case FileType::BSA:
            return "BSA";
        case FileType::BA2:
            return "BA2";
        case FileType::BGEM:
            return "BGEM";
        case FileType::BGSM:
            return "BGSM";
        case FileType::NIF:
            return "NIF";
        case FileType::KF:
            return "KF";
        case FileType::BTO:
            return "BTO";
        case FileType::BTR:
            return "BTR";
        case FileType::RDT:
            return "RDT";
        case FileType::PSA:
            return "PSA";
        case FileType::Unknown:
        default:
            return {};
    }
}

bool isBSA(const std::filesystem::path& path)
{
    return classifyFile(path).second == FileClass::Archive;
}

std::unique_ptr<VFS::Archive> makeArchive(const std::filesystem::path& path)
{
    if (isBSA(path))
        return VFS::makeBsaArchive(path, nullptr);
    if (std::filesystem::is_directory(path))
        return std::make_unique<VFS::FileSystemArchive>(path);
    return nullptr;
}

bool readFile(
    const std::filesystem::path& source, const std::filesystem::path& path, const VFS::Manager* vfs, bool quiet)
{
    const auto [fileType, fileClass] = classifyFile(path);
    if (fileClass != FileClass::NIF && fileClass != FileClass::Material)
        return false;

    const std::string pathStr = Files::pathToUnicodeString(path);
    if (!quiet)
    {
        std::cout << "Reading " << getFileTypeName(fileType) << " file '" << pathStr << "'";
        if (!source.empty())
            std::cout << " from '" << Files::pathToUnicodeString(isBSA(source) ? source.filename() : source) << "'";
        std::cout << std::endl;
    }
    const std::filesystem::path fullPath = !source.empty() ? source / path : path;
    try
    {
        switch (fileClass)
        {
            case FileClass::NIF:
            {
                Nif::NIFFile file(VFS::Path::Normalized(Files::pathToUnicodeString(fullPath)));
                Nif::Reader reader(file, nullptr);
                if (vfs != nullptr)
                    reader.parse(vfs->get(pathStr));
                else
                    reader.parse(Files::openConstrainedFileStream(fullPath));
                break;
            }
            case FileClass::Material:
            {
                if (vfs != nullptr)
                    Bgsm::parse(vfs->get(pathStr));
                else
                    Bgsm::parse(Files::openConstrainedFileStream(fullPath));
                break;
            }
            default:
                break;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to read '" << pathStr << "':" << std::endl << e.what() << std::endl;
    }
    return true;
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

    for (const auto& name : vfs.getRecursiveDirectoryIterator())
    {
        readFile(archivePath, name.value(), &vfs, quiet);
    }

    if (!archivePath.empty() && !isBSA(archivePath))
    {
        const Files::Collections fileCollections({ archivePath });
        const Files::MultiDirCollection& bsaCol = fileCollections.getCollection(".bsa");
        const Files::MultiDirCollection& ba2Col = fileCollections.getCollection(".ba2");
        for (const Files::MultiDirCollection& collection : { bsaCol, ba2Col })
        {
            for (auto& file : collection)
            {
                try
                {
                    readVFS(VFS::makeBsaArchive(file.second, nullptr), file.second, quiet);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Failed to read archive file '" << Files::pathToUnicodeString(file.second)
                              << "': " << e.what() << std::endl;
                }
            }
        }
    }
}

bool parseOptions(int argc, char** argv, Files::PathContainer& files, Files::PathContainer& archives,
    bool& writeDebugLog, bool& quiet)
{
    bpo::options_description desc(
        R"(Ensure that OpenMW can use the provided NIF, KF, BTO/BTR, RDT, PSA, BGEM/BGSM and BSA/BA2 files

Usages:
  niftest <nif files, kf files, bto/btr files, rdt files, psa files, bgem/bgsm files, BSA/BA2 files, or directories>
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
        bpo::parsed_options validOpts = bpo::command_line_parser(argc, argv).options(desc).positional(p).run();
        bpo::store(validOpts, variables);
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
            const bool isFile = readFile({}, path, vfs.get(), quiet);
            if (!isFile)
            {
                if (auto archive = makeArchive(path))
                {
                    readVFS(std::move(archive), path, quiet);
                }
                else
                {
                    std::cerr << "Error: '" << pathStr << "' is not a NIF file, material file, archive, or directory"
                              << std::endl;
                }
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "Failed to read '" << pathStr << "':  " << e.what() << std::endl;
        }
    }
    return 0;
}
