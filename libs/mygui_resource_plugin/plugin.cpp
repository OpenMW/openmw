#include "plugin.hpp"

#include <MyGUI_LogManager.h>

#include <components/bsa/resources.hpp>
#include <components/files/configurationmanager.hpp>

namespace MyGUI
{

    const std::string& ResourcePlugin::getName() const
    {
        static const std::string name = "OpenMW resource plugin";
        return name;
    }

    void ResourcePlugin::install()
    {

    }
    void ResourcePlugin::uninstall()
    {

    }

    void ResourcePlugin::initialize()
    {
        MYGUI_LOGGING("OpenMW_Resource_Plugin", Info, "initialize");

        boost::program_options::variables_map variables;

        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
        ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken())
        ("data-local", boost::program_options::value<std::string>()->default_value(""))
        ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)->default_value(false))
        ("fallback-archive", boost::program_options::value<std::vector<std::string> >()->
            default_value(std::vector<std::string>(), "fallback-archive")->multitoken());

        boost::program_options::notify(variables);

        Files::ConfigurationManager cfgManager;
        cfgManager.readConfiguration(variables, desc);

        std::vector<std::string> archives = variables["fallback-archive"].as<std::vector<std::string> >();
        bool fsStrict = variables["fs-strict"].as<bool>();

        Files::PathContainer dataDirs, dataLocal;
        if (!variables["data"].empty()) {
            dataDirs = Files::PathContainer(variables["data"].as<Files::PathContainer>());
        }

        std::string local = variables["data-local"].as<std::string>();
        if (!local.empty()) {
            dataLocal.push_back(Files::PathContainer::value_type(local));
        }

        cfgManager.processPaths (dataDirs);
        cfgManager.processPaths (dataLocal, true);

        if (!dataLocal.empty())
            dataDirs.insert (dataDirs.end(), dataLocal.begin(), dataLocal.end());

        Files::Collections collections (dataDirs, !fsStrict);

        Bsa::registerResources(collections, archives, true, fsStrict);
    }

    void ResourcePlugin::shutdown()
    {
        /// \todo remove resource groups
        MYGUI_LOGGING("OpenMW_Resource_Plugin", Info, "shutdown");
    }

}
