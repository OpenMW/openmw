#include <iostream>
#include <vector>
#include <exception>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <components/bsa/bsa_file.hpp>

#define BSATOOL_VERSION 1.1

// Create local aliases for brevity
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

struct Arguments
{
    std::string mode;
    std::string filename;
    std::string extractfile;
    std::string outdir;

    bool longformat;
    bool fullpath;
};

void replaceAll(std::string& str, const std::string& needle, const std::string& substitute)
{
    int pos = str.find(needle);
    while(pos != -1)
    {
        str.replace(pos, needle.size(), substitute);
        pos = str.find(needle);
    }
}

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
            "Allowed options");

    desc.add_options()
        ("help,h", "print help message.")
        ("version,v", "print version information and quit.")
        ("long,l", "Include extra information in archive listing.")
        ("full-path,f", "Create diretory hierarchy on file extraction "
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
    if (!(info.mode == "list" || info.mode == "extract" || info.mode == "extractall"))
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
    else if (variables["input-file"].as< std::vector<std::string> >().size() > 1)
        info.outdir = variables["input-file"].as< std::vector<std::string> >()[1];

    info.longformat = variables.count("long");
    info.fullpath = variables.count("full-path");

    return true;
}

int list(Bsa::BSAFile& bsa, Arguments& info);
int extract(Bsa::BSAFile& bsa, Arguments& info);
int extractAll(Bsa::BSAFile& bsa, Arguments& info);

int main(int argc, char** argv)
{
    Arguments info;
    if(!parseOptions (argc, argv, info))
        return 1;

    // Open file
    Bsa::BSAFile bsa;
    try
    {
        bsa.open(info.filename);
    }
    catch(std::exception &e)
    {
        std::cout << "ERROR reading BSA archive '" << info.filename
            << "'\nDetails:\n" << e.what() << std::endl;
        return 2;
    }

    if (info.mode == "list")
        return list(bsa, info);
    else if (info.mode == "extract")
        return extract(bsa, info);
    else if (info.mode == "extractall")
        return extractAll(bsa, info);
    else
    {
        std::cout << "Unsupported mode. That is not supposed to happen." << std::endl;
        return 1;
    }
}

int list(Bsa::BSAFile& bsa, Arguments& info)
{
    // List all files
    const Bsa::BSAFile::FileList &files = bsa.getList();
    for(int i=0; i<files.size(); i++)
    {
        if(info.longformat)
        {
            // Long format
            std::cout << std::setw(50) << std::left << files[i].name;
            std::cout << std::setw(8) << std::left << std::dec << files[i].fileSize;
            std::cout << "@ 0x" << std::hex << files[i].offset << std::endl;
        }
        else
            std::cout << files[i].name << std::endl;
    }

    return 0;
}

int extract(Bsa::BSAFile& bsa, Arguments& info)
{
    std::string archivePath = info.extractfile;
    replaceAll(archivePath, "/", "\\");

    std::string extractPath = info.extractfile;
    replaceAll(extractPath, "\\", "/");

    if (!bsa.exists(archivePath.c_str()))
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
    Ogre::DataStreamPtr data = bsa.getFile(archivePath.c_str());
    bfs::ofstream out(target, std::ios::binary);

    // Write the file to disk
    std::cout << "Extracting " << info.extractfile << " to " << target << std::endl;
    out.write(data->getAsString().c_str(), data->size());
    out.close();

    return 0;
}

int extractAll(Bsa::BSAFile& bsa, Arguments& info)
{
    // Get the list of files present in the archive
    Bsa::BSAFile::FileList list = bsa.getList();

    // Iter on the list
    for(Bsa::BSAFile::FileList::iterator it = list.begin(); it != list.end(); ++it) {
        const char* archivePath = it->name;

        std::string extractPath (archivePath);
        replaceAll(extractPath, "\\", "/");

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
        Ogre::DataStreamPtr data = bsa.getFile(archivePath);
        bfs::ofstream out(target, std::ios::binary);

        // Write the file to disk
        std::cout << "Extracting " << target << std::endl;
        out.write(data->getAsString().c_str(), data->size());
        out.close();
    }

    return 0;
}
