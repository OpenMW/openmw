#include "configurationmanager.hpp"

#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace Files
{

static const char* const openmwCfgFile = "openmw.cfg";
static const char* const ogreCfgFile = "ogre.cfg";
static const char* const pluginsCfgFile = "plugins.cfg";

const char* const mwDataToken = "?mw:data?";
const char* const localDataToken = "?local:data?";
const char* const userDataToken = "?user:data?";
const char* const globalDataToken = "?global:data?";

ConfigurationManager::ConfigurationManager()
    : mFixedPath("openmw")
{
    setupTokensMapping();

    /**
     * According to task #168 plugins.cfg file shall be located in global
     * configuration path or in local configuration path.
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
    mTokensMapping.insert(std::make_pair(mwDataToken, &FixedPath<>::setInstallPath));
    mTokensMapping.insert(std::make_pair(localDataToken, &FixedPath<>::setLocalDataPath));
    mTokensMapping.insert(std::make_pair(userDataToken, &FixedPath<>::setUserDataPath));
    mTokensMapping.insert(std::make_pair(globalDataToken, &FixedPath<>::setGlobalDataPath));
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

struct EmptyPath : public std::unary_function<const boost::filesystem::path&, bool>
{
    bool operator()(const boost::filesystem::path& path) const
    {
        return path.empty();
    }
};

void ConfigurationManager::processPaths(Files::PathContainer& dataDirs)
{
    for (Files::PathContainer::iterator it = dataDirs.begin(); it != dataDirs.end(); ++it)
    {
        const std::string& path = it->string();
        if (!path.empty() && path[0] == '?')
        {
            std::string::size_type pos = path.find('?', 1);
            if (pos != std::string::npos)
            {
                ++pos;
                TokensMappingContainer::iterator tokenIt = mTokensMapping.find(path.substr(0, pos));

                if (tokenIt != mTokensMapping.end())
                {
                    boost::filesystem::path tempPath(path.substr(pos, path.length() - pos));

                    if (boost::filesystem::is_directory(tempPath))
                    {
                        ((mFixedPath).*(tokenIt->second))(tempPath);
                        (*it) = tempPath;
                    }
                    else
                    {
                        (*it).clear();
                    }
                }
            }
        }
    }

    dataDirs.erase(std::remove_if(dataDirs.begin(), dataDirs.end(), EmptyPath()), dataDirs.end());
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

const boost::filesystem::path& ConfigurationManager::getGlobalDataPath() const
{
    return mFixedPath.getGlobalDataPath();
}

const boost::filesystem::path& ConfigurationManager::getUserDataPath() const
{
    return mFixedPath.getUserDataPath();
}

const boost::filesystem::path& ConfigurationManager::getLocalDataPath() const
{
    return mFixedPath.getLocalDataPath();
}

const boost::filesystem::path& ConfigurationManager::getInstallPath() const
{
    return mFixedPath.getInstallPath();
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
