#include "configurationmanager.hpp"

#include <string>
#include <iostream>
#include <algorithm>
#include <ctype.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/replace.hpp>
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

    loadConfig(mFixedPath.getUserConfigPath(), variables, description);
    boost::program_options::notify(variables);

    // read either local or global config depending on type of installation
    bool loaded = loadConfig(mFixedPath.getLocalPath(), variables, description);
    boost::program_options::notify(variables);
    if (!loaded)
    {
        loadConfig(mFixedPath.getGlobalConfigPath(), variables, description);
        boost::program_options::notify(variables);
    }

    mSilent = silent;
}

void ConfigurationManager::processPaths(Files::PathContainer& dataDirs, bool create)
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
        boost::bind(&boost::filesystem::path::empty, _1)), dataDirs.end());
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
            std::cout << "Loading config file: " << cfgFile.string() << "... ";

        boost::filesystem::ifstream configFileStreamUnfiltered(cfgFile);
        boost::iostreams::filtering_istream configFileStream;
        configFileStream.push(escape_hash_filter());
        configFileStream.push(configFileStreamUnfiltered);
        if (configFileStreamUnfiltered.is_open())
        {
            boost::program_options::store(boost::program_options::parse_config_file(
                configFileStream, description, true), variables);

            if (!mSilent)
                std::cout << "done." << std::endl;
            return true;
        }
        else
        {
            if (!mSilent)
                std::cout << "failed." << std::endl;
            return false;
        }
    }
    return false;
}

const int escape_hash_filter::sEscape = '@';
const int escape_hash_filter::sEscapeIdentifier = 'a';
const int escape_hash_filter::sHashIdentifier = 'h';

escape_hash_filter::escape_hash_filter() : mNext(), mSeenNonWhitespace(false), mFinishLine(false)
{
}

escape_hash_filter::~escape_hash_filter()
{
}

template <typename Source>
int escape_hash_filter::get(Source & src)
{
    if (mNext.empty())
    {
        int character = boost::iostreams::get(src);
        bool record = true;
        if (character == boost::iostreams::WOULD_BLOCK)
        {
            mNext.push(character);
            record = false;
        }
        else if (character == EOF)
        {
            mSeenNonWhitespace = false;
            mFinishLine = false;
            mNext.push(character);
        }
        else if (character == '\n')
        {
            mSeenNonWhitespace = false;
            mFinishLine = false;
            mNext.push(character);
        }
        else if (mFinishLine)
        {
            mNext.push(character);
        }
        else if (character == '#')
        {
            if (mSeenNonWhitespace)
            {
                mNext.push(sEscape);
                mNext.push(sHashIdentifier);
            }
            else
            {
                //it's fine being interpreted by Boost as a comment, and so is anything afterwards
                mNext.push(character);
                mFinishLine = true;
            }
        }
        else if (mPrevious == sEscape)
        {
            mNext.push(sEscape);
            mNext.push(sEscapeIdentifier);
        }
        else
        {
            mNext.push(character);
        }
        if (!mSeenNonWhitespace && !isspace(character))
            mSeenNonWhitespace = true;
        if (record)
            mPrevious = character;
    }
    int retval = mNext.front();
    mNext.pop();
    return retval;
}

std::string EscapeHashString::processString(const std::string & str)
{
    std::string temp = boost::replace_all_copy<std::string>(str, std::string() + (char)escape_hash_filter::sEscape + (char)escape_hash_filter::sHashIdentifier, "#");
    boost::replace_all(temp, std::string() + (char)escape_hash_filter::sEscape + (char)escape_hash_filter::sEscapeIdentifier, std::string((char) escape_hash_filter::sEscape, 1));
    return temp;
}

EscapeHashString::EscapeHashString() : mData()
{
}

EscapeHashString::EscapeHashString(const std::string & str) : mData(EscapeHashString::processString(str))
{
}

EscapeHashString::EscapeHashString(const std::string & str, size_t pos, size_t len) : mData(EscapeHashString::processString(str), pos, len)
{
}

EscapeHashString::EscapeHashString(const char * s) : mData(EscapeHashString::processString(std::string(s)))
{
}

EscapeHashString::EscapeHashString(const char * s, size_t n) : mData(EscapeHashString::processString(std::string(s)), 0, n)
{
}

EscapeHashString::EscapeHashString(size_t n, char c) : mData(n, c)
{
}

template <class InputIterator>
EscapeHashString::EscapeHashString(InputIterator first, InputIterator last) : mData(EscapeHashString::processString(std::string(first, last)))
{
}

std::string EscapeHashString::toStdString() const
{
    return std::string(mData);
}

std::istream & operator>> (std::istream & is, EscapeHashString & eHS)
{
    std::string temp;
    is >> temp;
    eHS = EscapeHashString(temp);
    return is;
}

std::ostream & operator<< (std::ostream & os, const EscapeHashString & eHS)
{
    os << eHS.mData;
    return os;
}

EscapeStringVector::EscapeStringVector() : mVector()
{
}

EscapeStringVector::~EscapeStringVector()
{
}

std::vector<std::string> EscapeStringVector::toStdStringVector() const
{
    std::vector<std::string> temp = std::vector<std::string>();
    for (std::vector<EscapeHashString>::const_iterator it = mVector.begin(); it != mVector.end(); ++it)
    {
        temp.push_back(it->toStdString());
    }
    return temp;
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
