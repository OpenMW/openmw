#include <components/bsa/bsa_file.hpp>

#include "bsatool_cmd.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <exception>

int main(int argc, char** argv)
{
    gengetopt_args_info info;

    if(cmdline_parser(argc, argv, &info) != 0)
        return 1;

    if(info.inputs_num != 1)
    {
        if(info.inputs_num == 0)
            std::cout << "ERROR: missing BSA file\n\n";
        else
            std::cout << "ERROR: more than one BSA file specified\n\n";
        cmdline_parser_print_help();
        return 1;
    }

    // Open file
    Bsa::BSAFile bsa;
    char *arcname = info.inputs[0];
    try { bsa.open(arcname); }
    catch(std::exception &e)
    {
        std::cout << "ERROR reading BSA archive '" << arcname
            << "'\nDetails:\n" << e.what() << std::endl;
        return 2;
    }

    if(info.extract_given)
    {
        char *file = info.extract_arg;

        if(!bsa.exists(file))
        {
            std::cout << "ERROR: file '" << file << "' not found\n";
            std::cout << "In archive: " << arcname << std::endl;
            return 3;
        }

        // Find the base name of the file
        int pos = strlen(file);
        while(pos > 0 && file[pos] != '\\') pos--;
        char *base = file+pos+1;

        // TODO: We might add full directory name extraction later. We
        // could also allow automatic conversion from / to \ in
        // parameter file names.

        // Load the file into a memory buffer
        Ogre::DataStreamPtr data = bsa.getFile(file);

        // Write the file to disk
        std::ofstream out(base, std::ios::binary);
        out.write(data->getAsString().c_str(), data->size());
        out.close();

        return 0;
    }

    // List all files
    const Bsa::BSAFile::FileList &files = bsa.getList();
    for(int i=0; i<files.size(); i++)
    {
        if(info.long_given)
        {
            // Long format
            std::cout << std::setw(50) << std::left << files[i].name;
            std::cout << std::setw(8) << std::left << std::dec << files[i].fileSize;
            std::cout << "@ 0x" << std::hex << files[i].offset << std::endl;
        }
        else
            std::cout << files[i].name << std::endl;
    }

    // Done!
    return 0;
}
