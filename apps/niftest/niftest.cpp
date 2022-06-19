///Program to test .nif files both on the FileSystem and in BSA archives.

#include <iostream>
#include <filesystem>

#include <components/misc/strings/algorithm.hpp>
#include <components/nif/niffile.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/bsaarchive.hpp>
#include <components/vfs/filesystemarchive.hpp>

#include <boost/program_options.hpp>

// Create local aliases for brevity
namespace bpo = boost::program_options;

///See if the file has the named extension
bool hasExtension(const std::filesystem::path& filename, const std::string& extensionToFind)
{
    std::string extension = filename.extension().string(); //TODO(Project579): let's hope unicode characters are never used in these extensions on windows or this will break
    return Misc::StringUtils::ciEqual(extension, extensionToFind);
}

///See if the file has the "nif" extension.
bool isNIF(const std::filesystem::path &filename)
{
    return hasExtension(filename,"nif");
}
///See if the file has the "bsa" extension.
bool isBSA(const std::filesystem::path &filename)
{
    return hasExtension(filename,"bsa");
}

/// Check all the nif files in a given VFS::Archive
/// \note Can not read a bsa file inside of a bsa file.
void readVFS(std::unique_ptr<VFS::Archive>&& anArchive, const std::filesystem::path& archivePath = {})
{
    VFS::Manager myManager(true);
    myManager.addArchive(std::move(anArchive));
    myManager.buildIndex();

    for(const auto& name : myManager.getRecursiveDirectoryIterator("")) //TODO(Project579): This will probably break in windows with unicode paths
    {
        try{
            if(isNIF(name))
            {
            //           std::cout << "Decoding: " << name << std::endl;
                Nif::NIFFile temp_nif(myManager.get(name),archivePath / name);
            }
            else if(isBSA(name))
            {
                if(!archivePath.empty() && !isBSA(archivePath))
                {
//                     std::cout << "Reading BSA File: " << name << std::endl;
                    readVFS(std::make_unique<VFS::BsaArchive>(archivePath / name), archivePath / name);
//                     std::cout << "Done with BSA File: " << name << std::endl;
                }
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "ERROR, an exception has occurred:  " << e.what() << std::endl;
        }
    }
}

bool parseOptions (int argc, char** argv, std::vector<std::string>& files)
{
    bpo::options_description desc("Ensure that OpenMW can use the provided NIF and BSA files\n\n"
        "Usages:\n"
        "  niftool <nif files, BSA files, or directories>\n"
        "      Scan the file or directories for nif errors.\n\n"
        "Allowed options");
    desc.add_options()
        ("help,h", "print help message.")
        ("input-file", bpo::value< std::vector<std::filesystem::path> >(), "input file")
        ;

    //Default option if none provided
    bpo::positional_options_description p;
    p.add("input-file", -1);

    bpo::variables_map variables;
    try
    {
        bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv).
            options(desc).positional(p).run();
        bpo::store(valid_opts, variables);
        bpo::notify(variables);
        if (variables.count ("help"))
        {
            std::cout << desc << std::endl;
            return false;
        }
        if (variables.count("input-file"))
        {
            files = variables["input-file"].as< std::vector<std::string> >();
            return true;
        }
    }
    catch(std::exception &e)
    {
        std::cout << "ERROR parsing arguments: " << e.what() << "\n\n"
            << desc << std::endl;
        return false;
    }

    std::cout << "No input files or directories specified!" << std::endl;
    std::cout << desc << std::endl;
    return false;
}

int main(int argc, char **argv)
{
    std::vector<std::string> files;
    if(!parseOptions (argc, argv, files))
        return 1;

    Nif::NIFFile::setLoadUnsupportedFiles(true);
//     std::cout << "Reading Files" << std::endl;
    for(const auto& name : files)
    {
        try
        {
            const std::filesystem::path path(name); //TODO(Project579): This will probably break in windows with unicode paths

            if(isNIF(path))
            {
                //std::cout << "Decoding: " << name << std::endl;
                Nif::NIFFile temp_nif(Files::openConstrainedFileStream(path), path);
             }
             else if(isBSA(path))
             {
//                 std::cout << "Reading BSA File: " << name << std::endl;
                readVFS(std::make_unique<VFS::BsaArchive>(path));
             }
             else if(std::filesystem::is_directory(path))
             {
//                 std::cout << "Reading All Files in: " << name << std::endl;
                readVFS(std::make_unique<VFS::FileSystemArchive>(path), path);
             }
             else
             {
                 std::cerr << "ERROR:  \"" << path << "\" is not a nif file, bsa file, or directory!" << std::endl;
             }
        }
        catch (std::exception& e)
        {
            std::cerr << "ERROR, an exception has occurred:  " << e.what() << std::endl;
        }
     }
     return 0;
}
