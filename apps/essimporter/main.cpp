#include <filesystem>
#include <iostream>

#include <boost/program_options.hpp>

#include <components/files/configurationmanager.hpp>

#include "importer.hpp"

namespace bpo = boost::program_options;

int main(int argc, char** argv)
{
    try
    {
        bpo::options_description desc(R"(Syntax: openmw-essimporter <options> infile.ess outfile.omwsave
Allowed options)");
        bpo::positional_options_description p_desc;
        auto addOption = desc.add_options();
        addOption("help,h", "produce help message");
        addOption("mwsave,m", bpo::value<Files::MaybeQuotedPath>(), "morrowind .ess save file");
        addOption("output,o", bpo::value<Files::MaybeQuotedPath>(), "output file (.omwsave)");
        addOption("compare,c", "compare two .ess files");
        addOption("encoding", boost::program_options::value<std::string>()->default_value("win1252"),
            "encoding of the save file");
        p_desc.add("mwsave", 1).add("output", 1);
        Files::ConfigurationManager::addCommonOptions(desc);

        bpo::variables_map variables;

        bpo::parsed_options parsed = bpo::command_line_parser(argc, argv).options(desc).positional(p_desc).run();
        bpo::store(parsed, variables);

        if (variables.count("help") || !variables.count("mwsave") || !variables.count("output"))
        {
            std::cout << desc;
            return 0;
        }

        bpo::notify(variables);

        Files::ConfigurationManager cfgManager(true);
        cfgManager.readConfiguration(variables, desc);

        const auto& essFile = variables["mwsave"].as<Files::MaybeQuotedPath>();
        const auto& outputFile = variables["output"].as<Files::MaybeQuotedPath>();
        std::string encoding = variables["encoding"].as<std::string>();

        ESSImport::Importer importer(essFile, outputFile, encoding);

        if (variables.count("compare"))
            importer.compare();
        else
        {
            static constexpr std::u8string_view ext{ u8".omwsave" };
            const auto length = outputFile.native().size();
            if (std::filesystem::exists(outputFile)
                && (length < ext.size() || outputFile.u8string().substr(length - ext.size()) != ext))
            {
                throw std::runtime_error(
                    "Output file already exists and does not end in .omwsave. Did you mean to use --compare?");
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
