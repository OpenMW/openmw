#include "configurationmanager.hpp"

#include <string>
#include <fstream>
#include <iostream>
#include <functional>

namespace Files
{

static const char* const openmwCfgFile = "openmw.cfg";
static const char* const ogreCfgFile = "ogre.cfg";
static const char* const pluginsCfgFile = "plugins.cfg";

static const char* const mwDataToken = "?mw:data?";
static const char* const localDataToken = "?local:data?";
static const char* const userDataToken = "?user:data?";
static const char* const globalDataToken = "?global:data?";

ConfigurationManager::ConfigurationManager()
    : mFixedPath("openmw")
{
    setupTokensMapping();

    /**
     * According to task #168 plugins.cfg file shall be located in global
     * configuration path or in runtime configuration path.
     */
    mPluginsCfgPath = mFixedPath.getGlobalPath() / pluginsCfgFile;
    if (!boost::filesystem::is_regular_file(mPluginsCfgPath))
    {
        mPluginsCfgPath = mFixedPath.getLocalPath() / pluginsCfgFile;
        if (!boost::filesystem::is_regular_file(mPluginsCfgPath))
        {
            std::cerr << "Failed to find " << pluginsCfgFile << " file!" << std::endl;
            mPluginsCfgPath.clear();
        }
    }

    /**
     * According to task #168 ogre.cfg file shall be located only
     * in user configuration path.
     */
    mOgreCfgPath = mFixedPath.getUserPath() / ogreCfgFile;

    /**
     * FIXME: Logs shoudn't be stored in the same dir where configuration is placed.
     */
    mLogPath = mFixedPath.getUserPath();
}

ConfigurationManager::~ConfigurationManager()
{
}

void ConfigurationManager::setupTokensMapping()
{
    mTokensMapping.insert(std::make_pair(mwDataToken, &ConfigurationManager::getInstallPath));
    mTokensMapping.insert(std::make_pair(localDataToken, &ConfigurationManager::getLocalDataPath));
    mTokensMapping.insert(std::make_pair(userDataToken, &ConfigurationManager::getUserDataPath));
    mTokensMapping.insert(std::make_pair(globalDataToken, &ConfigurationManager::getGlobalDataPath));
}

void ConfigurationManager::readConfiguration(boost::program_options::variables_map& variables,
    boost::program_options::options_description& description)
{
    loadConfig(mFixedPath.getUserPath(), variables, description);
    boost::program_options::notify(variables);
    loadConfig(mFixedPath.getLocalPath(), variables, description);
    boost::program_options::notify(variables);
    loadConfig(mFixedPath.getGlobalPath(), variables, description);
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

const boost::filesystem::path& ConfigurationManager::getGlobalPath() const
{
    return mFixedPath.getGlobalPath();
}

const boost::filesystem::path& ConfigurationManager::getUserPath() const
{
    return mFixedPath.getUserPath();
}

const boost::filesystem::path& ConfigurationManager::getLocalPath() const
{
    return mFixedPath.getLocalPath();
}

const boost::filesystem::path& ConfigurationManager::getDataPath(const std::string& type) const
{
    TokensMappingContainer::const_iterator it = mTokensMapping.find(type);
    if (it != mTokensMapping.end())
    {
        return ((this)->*(it->second))();
    }

    return mFixedPath.getLocalDataPath();
}

const boost::filesystem::path& ConfigurationManager::getInstallPath() const
{
// TODO: It will be corrected later.
    static boost::filesystem::path p("./");
    return p;

    //return mFixedPath.getInstallPath();
}

const boost::filesystem::path& ConfigurationManager::getGlobalDataPath() const
{
// TODO: It will be corrected later.
    static boost::filesystem::path p("./");
    return p;

    //return mFixedPath.getGlobalDataPath();
}

const boost::filesystem::path& ConfigurationManager::getUserDataPath() const
{
// TODO: It will be corrected later.
    static boost::filesystem::path p("./");
    return p;

    //return mFixedPath.getUserDataPath();
}

const boost::filesystem::path& ConfigurationManager::getLocalDataPath() const
{
// TODO: It will be corrected later.
    static boost::filesystem::path p("./");
    return p;
    //return mFixedPath.getLocalDataPath();
}


const boost::filesystem::path& ConfigurationManager::getOgreConfigPath() const
{
    return mOgreCfgPath;
}

const boost::filesystem::path& ConfigurationManager::getPluginsConfigPath() const
{
    return mPluginsCfgPath;
}

const boost::filesystem::path& ConfigurationManager::getLogPath() const
{
    return mLogPath;
}

} /* namespace Cfg */
