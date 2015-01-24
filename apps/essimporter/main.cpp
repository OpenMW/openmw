#include <iostream>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include "importer.hpp"

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;



int main(int argc, char** argv)
{
    try
    {
        bpo::options_description desc("Syntax: openmw-essimporter <options> infile.ess outfile.omwsave\nAllowed options");
        bpo::positional_options_description p_desc;
        desc.add_options()
            ("help,h", "produce help message")
            ("mwsave,m", bpo::value<std::string>(), "morrowind .ess save file")
            ("output,o", bpo::value<std::string>(), "output file (.omwsave)")
            ("compare,c", "compare two .ess files")
        ;
        p_desc.add("mwsave", 1).add("output", 1);

        bpo::variables_map vm;

        bpo::parsed_options parsed = bpo::command_line_parser(argc, argv)
            .options(desc)
            .positional(p_desc)
            .run();

        bpo::store(parsed, vm);

        if(vm.count("help") || !vm.count("mwsave") || !vm.count("output")) {
            std::cout << desc;
            return 0;
        }

        bpo::notify(vm);

        std::string essFile = vm["mwsave"].as<std::string>();
        std::string outputFile = vm["output"].as<std::string>();

        ESSImport::Importer importer(essFile, outputFile);

        if (vm.count("compare"))
            importer.compare();
        else
        {
            const std::string& ext = ".omwsave";
            if (boost::filesystem::exists(boost::filesystem::path(outputFile))
                    && (outputFile.size() < ext.size() || outputFile.substr(outputFile.size()-ext.size()) != ext))
            {
                throw std::runtime_error("Output file already exists and does not end in .omwsave. Did you mean to use --compare?");
            }
            importer.run();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
