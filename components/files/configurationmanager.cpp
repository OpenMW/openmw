#include "configurationmanager.hpp"

#include <components/debug/debuglog.hpp>
#include <components/files/escape.hpp>

#include <boost/filesystem/fstream.hpp>
/**
 * \namespace Files
 */
namespace Files
{

static const char* const openmwCfgFile = "openmw.cfg";

#if defined(_WIN32) || defined(__WINDOWS__)
static const char* const applicationName = "OpenMW";
#else
static const char* const applicationName = "openmw";
#endif

const char* const localToken = "?local?";
const char* const userDataToken = "?userdata?";
const char* const globalToken = "?global?";

ConfigurationManager::ConfigurationManager(bool silent)
    : mFixedPath(applicationName)
    , mSilent(silent)
{
    setupTokensMapping();

    boost::filesystem::create_directories(mFixedPath.getUserConfigPath());
    boost::filesystem::create_directories(mFixedPath.getUserDataPath());

    mLogPath = mFixedPath.getUserConfigPath();
}

ConfigurationManager::~ConfigurationManager()
{
}

void ConfigurationManager::setupTokensMapping()
{
    mTokensMapping.insert(std::make_pair(localToken, &FixedPath<>::getLocalPath));
    mTokensMapping.insert(std::make_pair(userDataToken, &FixedPath<>::getUserDataPath));
    mTokensMapping.insert(std::make_pair(globalToken, &FixedPath<>::getGlobalDataPath));
}

void ConfigurationManager::readConfiguration(boost::program_options::variables_map& variables,
    boost::program_options::options_description& description, bool quiet)
{
    bool silent = mSilent;
    mSilent = quiet;

    // read either local or global config depending on type of installation
    bool loaded = loadConfig(mFixedPath.getLocalPath(), variables, description);
    boost::program_options::notify(variables);
    if (!loaded)
    {
        loadConfig(mFixedPath.getGlobalConfigPath(), variables, description);
        boost::program_options::notify(variables);
    }

    // User config has the highest priority.
    loadConfig(mFixedPath.getUserConfigPath(), variables, description);
    boost::program_options::notify(variables);

    mSilent = silent;
}

void ConfigurationManager::processPaths(Files::PathContainer& dataDirs, bool create)
{
    std::string path;
    for (Files::PathContainer::iterator it = dataDirs.begin(); it != dataDirs.end(); ++it)
    {
        path = it->string();

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
            if (create)
            {
                try
                {
                    boost::filesystem::create_directories (*it);
                }
                catch (...) {}

                if (boost::filesystem::is_directory(*it))
                    continue;
            }

            (*it).clear();
        }
    }

    dataDirs.erase(std::remove_if(dataDirs.begin(), dataDirs.end(),
        std::bind(&boost::filesystem::path::empty, std::placeholders::_1)), dataDirs.end());
}

bool ConfigurationManager::loadConfig(const boost::filesystem::path& path,
    boost::program_options::variables_map& variables,
    boost::program_options::options_description& description)
{
    boost::filesystem::path cfgFile(path);
    cfgFile /= std::string(openmwCfgFile);
    if (boost::filesystem::is_regular_file(cfgFile))
    {
        if (!mSilent)
            Log(Debug::Info) << "Loading config file: " << cfgFile.string();

        boost::filesystem::ifstream configFileStreamUnfiltered(cfgFile);
        boost::iostreams::filtering_istream configFileStream;
        configFileStream.push(escape_hash_filter());
        configFileStream.push(configFileStreamUnfiltered);
        if (configFileStreamUnfiltered.is_open())
        {
            boost::program_options::store(boost::program_options::parse_config_file(
                configFileStream, description, true), variables);

            return true;
        }
        else
        {
            if (!mSilent)
                Log(Debug::Error) << "Loading failed.";

            return false;
        }
    }
    return false;
}

const boost::filesystem::path& ConfigurationManager::getGlobalPath() const
{
    return mFixedPath.getGlobalConfigPath();
}

const boost::filesystem::path& ConfigurationManager::getUserConfigPath() const
{
    return mFixedPath.getUserConfigPath();
}

const boost::filesystem::path& ConfigurationManager::getUserDataPath() const
{
    return mFixedPath.getUserDataPath();
}

const boost::filesystem::path& ConfigurationManager::getLocalPath() const
{
    return mFixedPath.getLocalPath();
}

const boost::filesystem::path& ConfigurationManager::getGlobalDataPath() const
{
    return mFixedPath.getGlobalDataPath();
}

const boost::filesystem::path& ConfigurationManager::getCachePath() const
{
    return mFixedPath.getCachePath();
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
