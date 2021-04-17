#include <iostream>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <components/files/configurationmanager.hpp>

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
            ("encoding", boost::program_options::value<std::string>()->default_value("win1252"), "encoding of the save file")
        ;
        p_desc.add("mwsave", 1).add("output", 1);

        bpo::variables_map variables;

        bpo::parsed_options parsed = bpo::command_line_parser(argc, argv)
            .options(desc)
            .positional(p_desc)
            .run();

        bpo::store(parsed, variables);

        if(variables.count("help") || !variables.count("mwsave") || !variables.count("output")) {
            std::cout << desc;
            return 0;
        }

        bpo::notify(variables);

        Files::ConfigurationManager cfgManager(true);
        cfgManager.readConfiguration(variables, desc);

        std::string essFile = variables["mwsave"].as<std::string>();
        std::string outputFile = variables["output"].as<std::string>();
        std::string encoding = variables["encoding"].as<std::string>();

        ESSImport::Importer importer(essFile, outputFile, encoding);

        if (variables.count("compare"))
            importer.compare();
        else
        {
            const std::string& ext = ".omwsave";
            if (bfs::exists(bfs::path(outputFile))
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
