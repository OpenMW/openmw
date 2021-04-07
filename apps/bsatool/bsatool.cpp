#include <iostream>
#include <iomanip>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <components/bsa/compressedbsafile.hpp>
#include <components/misc/stringops.hpp>

#define BSATOOL_VERSION 1.1

// Create local aliases for brevity
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

struct Arguments
{
    std::string mode;
    std::string filename;
    std::string extractfile;
    std::string addfile;
    std::string outdir;

    bool longformat;
    bool fullpath;
};

bool parseOptions (int argc, char** argv, Arguments &info)
{
    bpo::options_description desc("Inspect and extract files from Bethesda BSA archives\n\n"
            "Usages:\n"
            "  bsatool list [-l] archivefile\n"
            "      List the files presents in the input archive.\n\n"
            "  bsatool extract [-f] archivefile [file_to_extract] [output_directory]\n"
            "      Extract a file from the input archive.\n\n"
            "  bsatool extractall archivefile [output_directory]\n"
            "      Extract all files from the input archive.\n\n"
            "  bsatool add [-a] archivefile file_to_add\n"
            "      Add a file to the input archive.\n\n"
            "  bsatool create [-c] archivefile\n"
            "      Create an archive.\n\n"
            "Allowed options");

    desc.add_options()
        ("help,h", "print help message.")
        ("version,v", "print version information and quit.")
        ("long,l", "Include extra information in archive listing.")
        ("full-path,f", "Create directory hierarchy on file extraction "
         "(always true for extractall).")
        ;

    // input-file is hidden and used as a positional argument
    bpo::options_description hidden("Hidden Options");

    hidden.add_options()
        ( "mode,m", bpo::value<std::string>(), "bsatool mode")
        ( "input-file,i", bpo::value< std::vector<std::string> >(), "input file")
        ;

    bpo::positional_options_description p;
    p.add("mode", 1).add("input-file", 3);

    // there might be a better way to do this
    bpo::options_description all;
    all.add(desc).add(hidden);

    bpo::variables_map variables;
    try
    {
        bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv)
            .options(all).positional(p).run();
        bpo::store(valid_opts, variables);
    }
    catch(std::exception &e)
    {
        std::cout << "ERROR parsing arguments: " << e.what() << "\n\n"
            << desc << std::endl;
        return false;
    }

    bpo::notify(variables);

    if (variables.count ("help"))
    {
        std::cout << desc << std::endl;
        return false;
    }
    if (variables.count ("version"))
    {
        std::cout << "BSATool version " << BSATOOL_VERSION << std::endl;
        return false;
    }
    if (!variables.count("mode"))
    {
        std::cout << "ERROR: no mode specified!\n\n"
            << desc << std::endl;
        return false;
    }

    info.mode = variables["mode"].as<std::string>();
    if (!(info.mode == "list" || info.mode == "extract" || info.mode == "extractall" || info.mode == "add" || info.mode == "create"))
    {
        std::cout << std::endl << "ERROR: invalid mode \"" << info.mode << "\"\n\n"
            << desc << std::endl;
        return false;
    }

    if (!variables.count("input-file"))
    {
        std::cout << "\nERROR: missing BSA archive\n\n"
            << desc << std::endl;
        return false;
    }
    info.filename = variables["input-file"].as< std::vector<std::string> >()[0];

    // Default output to the working directory
    info.outdir = ".";

    if (info.mode == "extract")
    {
        if (variables["input-file"].as< std::vector<std::string> >().size() < 2)
        {
            std::cout << "\nERROR: file to extract unspecified\n\n"
                << desc << std::endl;
            return false;
        }
        if (variables["input-file"].as< std::vector<std::string> >().size() > 1)
            info.extractfile = variables["input-file"].as< std::vector<std::string> >()[1];
        if (variables["input-file"].as< std::vector<std::string> >().size() > 2)
            info.outdir = variables["input-file"].as< std::vector<std::string> >()[2];
    }
    else if (info.mode == "add")
    {
        if (variables["input-file"].as< std::vector<std::string> >().size() < 1)
        {
            std::cout << "\nERROR: file to add unspecified\n\n"
                << desc << std::endl;
            return false;
        }
        if (variables["input-file"].as< std::vector<std::string> >().size() > 1)
            info.addfile = variables["input-file"].as< std::vector<std::string> >()[1];
    }
    else if (variables["input-file"].as< std::vector<std::string> >().size() > 1)
        info.outdir = variables["input-file"].as< std::vector<std::string> >()[1];

    info.longformat = variables.count("long") != 0;
    info.fullpath = variables.count("full-path") != 0;

    return true;
}

int list(std::unique_ptr<Bsa::BSAFile>& bsa, Arguments& info);
int extract(std::unique_ptr<Bsa::BSAFile>& bsa, Arguments& info);
int extractAll(std::unique_ptr<Bsa::BSAFile>& bsa, Arguments& info);
int add(std::unique_ptr<Bsa::BSAFile>& bsa, Arguments& info);

int main(int argc, char** argv)
{
    try
    {
        Arguments info;
        if(!parseOptions (argc, argv, info))
            return 1;

        // Open file
        std::unique_ptr<Bsa::BSAFile> bsa;

        Bsa::BsaVersion bsaVersion = Bsa::CompressedBSAFile::detectVersion(info.filename);

        if (bsaVersion == Bsa::BSAVER_COMPRESSED)
            bsa = std::make_unique<Bsa::CompressedBSAFile>(Bsa::CompressedBSAFile());
        else
            bsa = std::make_unique<Bsa::BSAFile>(Bsa::BSAFile());

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
    catch (std::exception& e)
    {
        std::cerr << "ERROR reading BSA archive\nDetails:\n" << e.what() << std::endl;
        return 2;
    }
}

int list(std::unique_ptr<Bsa::BSAFile>& bsa, Arguments& info)
{
    // List all files
    const Bsa::BSAFile::FileList &files = bsa->getList();
    for (const auto& file : files)
    {
        if(info.longformat)
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

int extract(std::unique_ptr<Bsa::BSAFile>& bsa, Arguments& info)
{
    std::string archivePath = info.extractfile;
    Misc::StringUtils::replaceAll(archivePath, "/", "\\");

    std::string extractPath = info.extractfile;
    Misc::StringUtils::replaceAll(extractPath, "\\", "/");

    if (!bsa->exists(archivePath.c_str()))
    {
        std::cout << "ERROR: file '" << archivePath << "' not found\n";
        std::cout << "In archive: " << info.filename << std::endl;
        return 3;
    }

    // Get the target path (the path the file will be extracted to)
    bfs::path relPath (extractPath);
    bfs::path outdir (info.outdir);

    bfs::path target;
    if (info.fullpath)
        target = outdir / relPath;
    else
        target = outdir / relPath.filename();

    // Create the directory hierarchy
    bfs::create_directories(target.parent_path());

    bfs::file_status s = bfs::status(target.parent_path());
    if (!bfs::is_directory(s))
    {
        std::cout << "ERROR: " << target.parent_path() << " is not a directory." << std::endl;
        return 3;
    }

    // Get a stream for the file to extract
    Files::IStreamPtr stream = bsa->getFile(archivePath.c_str());

    bfs::ofstream out(target, std::ios::binary);

    // Write the file to disk
    std::cout << "Extracting " << info.extractfile << " to " << target << std::endl;

    out << stream->rdbuf();
    out.close();

    return 0;
}

int extractAll(std::unique_ptr<Bsa::BSAFile>& bsa, Arguments& info)
{
    for (const auto &file : bsa->getList())
    {
        std::string extractPath(file.name());
        Misc::StringUtils::replaceAll(extractPath, "\\", "/");

        // Get the target path (the path the file will be extracted to)
        bfs::path target (info.outdir);
        target /= extractPath;

        // Create the directory hierarchy
        bfs::create_directories(target.parent_path());

        bfs::file_status s = bfs::status(target.parent_path());
        if (!bfs::is_directory(s))
        {
            std::cout << "ERROR: " << target.parent_path() << " is not a directory." << std::endl;
            return 3;
        }

        // Get a stream for the file to extract
        // (inefficient because getFile iter on the list again)
        Files::IStreamPtr data = bsa->getFile(file.name());
        bfs::ofstream out(target, std::ios::binary);

        // Write the file to disk
        std::cout << "Extracting " << target << std::endl;
        out << data->rdbuf();
        out.close();
    }

    return 0;
}

int add(std::unique_ptr<Bsa::BSAFile>& bsa, Arguments& info)
{
    boost::filesystem::fstream stream(info.addfile, std::ios_base::binary | std::ios_base::out | std::ios_base::in);
    bsa->addFile(info.addfile, stream);

    return 0;
}
