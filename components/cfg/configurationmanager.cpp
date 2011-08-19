#include "configurationmanager.hpp"

#include <string>
#include <fstream>
#include <iostream>

namespace Cfg
{

static const char* const openmwCfgFile = "openmw.cfg";
static const char* const ogreCfgFile = "ogre.cfg";
static const char* const pluginsCfgFile = "plugins.cfg";


ConfigurationManager::ConfigurationManager()
    : mPath("openmw")
{
}

ConfigurationManager::~ConfigurationManager()
{
}

void ConfigurationManager::readConfiguration(boost::program_options::variables_map& variables,
    boost::program_options::options_description& description)
{
    loadConfig(mPath.getGlobalConfigPath(), variables, description);
    loadConfig(mPath.getLocalConfigPath(), variables, description);
    loadConfig(mPath.getRuntimeConfigPath(), variables, description);
    boost::program_options::notify(variables);
}

void ConfigurationManager::loadConfig(const boost::filesystem::path& path,
    boost::program_options::variables_map& variables,
    boost::program_options::options_description& description)
{
    boost::filesystem::path cfgFile(path);
    cfgFile /= std::string(openmwCfgFile);
    if (boost::filesystem::is_regular_file(cfgFile))
    {
        std::cout << "Loading config file: " << cfgFile.string() << "... ";

        std::ifstream configFileStream(cfgFile.string().c_str());
        if (configFileStream.is_open())
        {
            boost::program_options::store(boost::program_options::parse_config_file(
                configFileStream, description), variables);

            std::cout << "done." << std::endl;
        }
        else
        {
            std::cout << "failed." << std::endl;
        }
    }
}

const boost::filesystem::path& ConfigurationManager::getGlobalConfigPath() const
{
    return mPath.getGlobalConfigPath();
}

void ConfigurationManager::setGlobalConfigPath(const boost::filesystem::path& newPath)
{
    mPath.setGlobalConfigPath(newPath);
}

const boost::filesystem::path& ConfigurationManager::getLocalConfigPath() const
{
    return mPath.getLocalConfigPath();
}

void ConfigurationManager::setLocalConfigPath(const boost::filesystem::path& newPath)
{
    mPath.setLocalConfigPath(newPath);
}

const boost::filesystem::path& ConfigurationManager::getRuntimeConfigPath() const
{
    return mPath.getRuntimeConfigPath();
}

void ConfigurationManager::setRuntimeConfigPath(const boost::filesystem::path& newPath)
{
    mPath.setRuntimeConfigPath(newPath);
}

const boost::filesystem::path& ConfigurationManager::getGlobalDataPath() const
{
    return mPath.getGlobalDataPath();
}

void ConfigurationManager::setGlobalDataPath(const boost::filesystem::path& newPath)
{
    mPath.setGlobalDataPath(newPath);
}

const boost::filesystem::path& ConfigurationManager::getLocalDataPath() const
{
    return mPath.getLocalDataPath();
}

void ConfigurationManager::setLocalDataPath(const boost::filesystem::path& newPath)
{
    mPath.setLocalDataPath(newPath);
}

const boost::filesystem::path& ConfigurationManager::getRuntimeDataPath() const
{
    return mPath.getRuntimeDataPath();
}

void ConfigurationManager::setRuntimeDataPath(const boost::filesystem::path& newPath)
{
    mPath.setRuntimeDataPath(newPath);
}

} /* namespace Cfg */
