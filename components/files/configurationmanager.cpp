#include "configurationmanager.hpp"

#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/algorithm/string/erase.hpp>

/**
 * \namespace Files
 */
namespace Files
{

static const char* const openmwCfgFile = "openmw.cfg";

const char* const mwToken = "?mw?";
const char* const localToken = "?local?";
const char* const userToken = "?user?";
const char* const globalToken = "?global?";

ConfigurationManager::ConfigurationManager()
    : mFixedPath("openmw")
{
    setupTokensMapping();

    boost::filesystem::create_directories(mFixedPath.getUserPath());

    mLogPath = mFixedPath.getUserPath();
}

ConfigurationManager::~ConfigurationManager()
{
}

void ConfigurationManager::setupTokensMapping()
{
    mTokensMapping.insert(std::make_pair(mwToken, &FixedPath<>::getInstallPath));
    mTokensMapping.insert(std::make_pair(localToken, &FixedPath<>::getLocalPath));
    mTokensMapping.insert(std::make_pair(userToken, &FixedPath<>::getUserPath));
    mTokensMapping.insert(std::make_pair(globalToken, &FixedPath<>::getGlobalDataPath));
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

void ConfigurationManager::processPaths(Files::PathContainer& dataDirs)
{
    std::string path;
    for (Files::PathContainer::iterator it = dataDirs.begin(); it != dataDirs.end(); ++it)
    {
        path = it->string();
        boost::erase_all(path, "\"");
        *it = boost::filesystem::path(path);

        // Check if path contains a token
        if (!path.empty() && *path.begin() == '?')
        {
            std::string::size_type pos = path.find('?', 1);
            if (pos != std::string::npos && pos != 0)
            {
                TokensMappingContainer::iterator tokenIt = mTokensMapping.find(path.substr(0, pos + 1));
                if (tokenIt != mTokensMapping.end())
                {
                    boost::filesystem::path tempPath(((mFixedPath).*(tokenIt->second))());
                    if (pos < path.length() - 1)
                    {
                        // There is something after the token, so we should
                        // append it to the path
                        tempPath /= path.substr(pos + 1, path.length() - pos);
                    }

                    *it = tempPath;
                }
                else
                {
                    // Clean invalid / unknown token, it will be removed outside the loop
                    (*it).clear();
                }
            }
        }

        if (!boost::filesystem::is_directory(*it))
        {
            (*it).clear();
        }
    }

    dataDirs.erase(std::remove_if(dataDirs.begin(), dataDirs.end(),
        boost::bind(&boost::filesystem::path::empty, _1)), dataDirs.end());
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
                configFileStream, description, true), variables);

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

const boost::filesystem::path& ConfigurationManager::getInstallPath() const
{
    return mFixedPath.getInstallPath();
}

const boost::filesystem::path& ConfigurationManager::getLogPath() const
{
    return mLogPath;
}

} /* namespace Cfg */
