#include "configurationmanager.hpp"

#include <fstream>
#include <map>

#include <components/debug/debuglog.hpp>
#include <components/fallback/validate.hpp>
#include <components/files/configfileparser.hpp>
#include <components/misc/strings/conversion.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

/**
 * \namespace Files
 */
namespace Files
{

    namespace bpo = boost::program_options;

    namespace
    {
#if defined(_WIN32) || defined(__WINDOWS__)
        constexpr auto sApplicationName = "OpenMW";
#else
        constexpr auto sApplicationName = "openmw";
#endif

        using GetPath = const std::filesystem::path& (Files::FixedPath<>::*)() const;
        constexpr std::array<std::pair<std::u8string_view, GetPath>, 4> sTokenMappings = {
            std::make_pair(u8"?local?", &FixedPath<>::getLocalPath),
            std::make_pair(u8"?userconfig?", &FixedPath<>::getUserConfigPath),
            std::make_pair(u8"?userdata?", &FixedPath<>::getUserDataPath),
            std::make_pair(u8"?global?", &FixedPath<>::getGlobalDataPath),
        };
    }

    ConfigurationManager::ConfigurationManager(bool silent)
        : mFixedPath(sApplicationName)
        , mSilent(silent)
    {
        // Initialize with fixed paths, will be overridden in `readConfiguration`.
        mUserDataPath = mFixedPath.getUserDataPath();
        mScreenshotPath = mFixedPath.getUserDataPath() / "screenshots";
    }

    ConfigurationManager::~ConfigurationManager() = default;

    static bool hasReplaceConfig(const bpo::variables_map& variables)
    {
        if (variables["replace"].empty())
            return false;
        for (const std::string& var : variables["replace"].as<std::vector<std::string>>())
        {
            if (var == "config")
                return true;
        }
        return false;
    }

    void ConfigurationManager::readConfiguration(
        bpo::variables_map& variables, const bpo::options_description& description, bool quiet)
    {
        bool silent = mSilent;
        mSilent = quiet;

        // ensure defaults are present
        bpo::store(bpo::parsed_options(&description), variables);

        std::optional<bpo::variables_map> config = loadConfig(mFixedPath.getLocalPath(), description);
        if (config)
            mActiveConfigPaths.push_back(mFixedPath.getLocalPath());
        else if (!mFixedPath.getGlobalConfigPath().empty())
        {
            mActiveConfigPaths.push_back(mFixedPath.getGlobalConfigPath());
            config = loadConfig(mFixedPath.getGlobalConfigPath(), description);
        }
        if (!config)
        {
            if (!quiet)
                Log(Debug::Error) << "Neither local config nor global config are available.";
            mSilent = silent;
            return;
        }

        std::stack<std::filesystem::path> extraConfigDirs;
        addExtraConfigDirs(extraConfigDirs, variables);
        if (!hasReplaceConfig(variables))
            addExtraConfigDirs(extraConfigDirs, *config);

        std::vector<bpo::variables_map> parsedConfigs{ *std::move(config) };
        std::set<std::filesystem::path>
            alreadyParsedPaths; // needed to prevent infinite loop in case of a circular link
        alreadyParsedPaths.insert(mActiveConfigPaths.front());

        while (!extraConfigDirs.empty())
        {
            auto path = extraConfigDirs.top();
            extraConfigDirs.pop();
            if (alreadyParsedPaths.count(path) > 0)
            {
                if (!quiet)
                    Log(Debug::Warning) << "Repeated config dir: " << path;
                continue;
            }
            alreadyParsedPaths.insert(path);
            config = loadConfig(path, description);
            if (config && hasReplaceConfig(*config) && parsedConfigs.size() > 1)
            {
                mActiveConfigPaths.resize(1);
                parsedConfigs.resize(1);
                Log(Debug::Info) << "Skipping previous configs except " << (mActiveConfigPaths.front() / "openmw.cfg")
                                 << " due to replace=config in " << (path / "openmw.cfg");
            }
            mActiveConfigPaths.emplace_back(std::move(path));
            if (config)
            {
                addExtraConfigDirs(extraConfigDirs, *config);
                parsedConfigs.push_back(*std::move(config));
            }
        }

        for (auto it = parsedConfigs.rbegin(); it != parsedConfigs.rend(); ++it)
        {
            auto composingVariables = separateComposingVariables(variables, description);
            for (auto& [k, v] : *it)
            {
                auto variable = variables.find(k);
                if (variable == variables.end())
                    variables.insert({ k, v });
                else if (variable->second.defaulted())
                    variable->second = v;
            }
            mergeComposingVariables(variables, composingVariables, description);
        }

        mUserDataPath = variables["user-data"]
                            .as<Files::MaybeQuotedPath>()
                            .u8string(); // This call to u8string is redundant, but required to build on MSVC 14.26 due
                                         // to implementation bugs.
        if (mUserDataPath.empty())
        {
            if (!quiet)
                Log(Debug::Warning) << "Error: `user-data` is not specified";
            mUserDataPath = mFixedPath.getUserDataPath();
        }
        mScreenshotPath = mUserDataPath / "screenshots";

        std::filesystem::create_directories(getUserConfigPath());
        std::filesystem::create_directories(mScreenshotPath);

        // probably not necessary but validate the creation of the screenshots directory and fallback to the original
        // behavior if it fails
        if (!std::filesystem::is_directory(mScreenshotPath))
            mScreenshotPath = mUserDataPath;

        if (!quiet)
        {
            Log(Debug::Info) << "Logs dir: " << getUserConfigPath();
            Log(Debug::Info) << "User data dir: " << mUserDataPath;
            Log(Debug::Info) << "Screenshots dir: " << mScreenshotPath;
        }

        mSilent = silent;
    }

    void ConfigurationManager::addExtraConfigDirs(
        std::stack<std::filesystem::path>& dirs, const bpo::variables_map& variables) const
    {
        auto configIt = variables.find("config");
        if (configIt == variables.end())
            return;
        Files::PathContainer newDirs = asPathContainer(configIt->second.as<Files::MaybeQuotedPathContainer>());
        for (auto it = newDirs.rbegin(); it != newDirs.rend(); ++it)
            dirs.push(*it);
    }

    void ConfigurationManager::addCommonOptions(bpo::options_description& description)
    {
        auto addOption = description.add_options();
        addOption("config",
            bpo::value<Files::MaybeQuotedPathContainer>()
                ->default_value(Files::MaybeQuotedPathContainer(), "")
                ->multitoken()
                ->composing(),
            "additional config directories");
        addOption("replace",
            bpo::value<std::vector<std::string>>()
                ->default_value(std::vector<std::string>(), "")
                ->multitoken()
                ->composing(),
            "settings where the values from the current source should replace those from lower-priority sources "
            "instead of being appended");
        addOption("user-data", bpo::value<Files::MaybeQuotedPath>()->default_value(Files::MaybeQuotedPath(), ""),
            "set user data directory (used for saves, screenshots, etc)");
        addOption("resources",
            boost::program_options::value<Files::MaybeQuotedPath>()->default_value(
                Files::MaybeQuotedPath(), "resources"),
            "set resources directory");
    }

    bpo::variables_map separateComposingVariables(
        bpo::variables_map& variables, const bpo::options_description& description)
    {
        bpo::variables_map composingVariables;
        for (auto itr = variables.begin(); itr != variables.end();)
        {
            if (description.find(itr->first, false).semantic()->is_composing())
            {
                composingVariables.emplace(*itr);
                itr = variables.erase(itr);
            }
            else
                ++itr;
        }
        return composingVariables;
    }

    void mergeComposingVariables(
        bpo::variables_map& first, bpo::variables_map& second, const bpo::options_description& description)
    {
        // There are a few places this assumes all variables are present in second, but it's never crashed in the wild,
        // so it looks like that's guaranteed.
        std::set<std::string> replacedVariables;
        if (description.find_nothrow("replace", false))
        {
            auto replace = second["replace"];
            if (!replace.defaulted() && !replace.empty())
            {
                std::vector<std::string> replaceVector = replace.as<std::vector<std::string>>();
                replacedVariables.insert(replaceVector.begin(), replaceVector.end());
            }
        }
        for (const auto& option : description.options())
        {
            if (option->semantic()->is_composing())
            {
                std::string name = option->canonical_display_name();

                auto firstPosition = first.find(name);
                if (firstPosition == first.end())
                {
                    first.emplace(name, second[name]);
                    continue;
                }

                if (replacedVariables.count(name) || firstPosition->second.defaulted() || firstPosition->second.empty())
                {
                    firstPosition->second = second[name];
                    continue;
                }

                if (second[name].defaulted() || second[name].empty())
                    continue;

                boost::any& firstValue = firstPosition->second.value();
                const boost::any& secondValue = second[name].value();

                if (firstValue.type() == typeid(Files::MaybeQuotedPathContainer))
                {
                    auto& firstPathContainer = boost::any_cast<Files::MaybeQuotedPathContainer&>(firstValue);
                    const auto& secondPathContainer
                        = boost::any_cast<const Files::MaybeQuotedPathContainer&>(secondValue);
                    firstPathContainer.insert(
                        firstPathContainer.end(), secondPathContainer.begin(), secondPathContainer.end());
                }
                else if (firstValue.type() == typeid(std::vector<std::string>))
                {
                    auto& firstVector = boost::any_cast<std::vector<std::string>&>(firstValue);
                    const auto& secondVector = boost::any_cast<const std::vector<std::string>&>(secondValue);

                    firstVector.insert(firstVector.end(), secondVector.begin(), secondVector.end());
                }
                else if (firstValue.type() == typeid(Fallback::FallbackMap))
                {
                    auto& firstMap = boost::any_cast<Fallback::FallbackMap&>(firstValue);
                    const auto& secondMap = boost::any_cast<const Fallback::FallbackMap&>(secondValue);

                    std::map<std::string, std::string> tempMap(secondMap.mMap);
                    tempMap.merge(firstMap.mMap);
                    firstMap.mMap.swap(tempMap);
                }
                else
                    Log(Debug::Error)
                        << "Unexpected composing variable type. Curse boost and their blasted arcane templates.";
            }
        }
    }

    void ConfigurationManager::processPath(std::filesystem::path& path, const std::filesystem::path& basePath) const
    {
        const auto str = path.u8string();

        if (str.empty() || str[0] != u8'?')
        {
            if (!path.is_absolute())
                path = basePath / path;
            return;
        }

        const auto pos = str.find('?', 1);
        if (pos != std::u8string::npos && pos != 0)
        {
            std::u8string_view view(str);
            auto token = view.substr(0, pos + 1);
            auto found = std::find_if(
                sTokenMappings.begin(), sTokenMappings.end(), [&](const auto& item) { return item.first == token; });
            if (found != sTokenMappings.end())
            {
                auto tempPath(((mFixedPath).*(found->second))());
                if (!tempPath.empty() && pos < view.length() - 1)
                {
                    // There is something after the token, so we should
                    // append it to the path
                    tempPath /= view.substr(pos + 1, view.length() - pos);
                }

                path = std::move(tempPath);
            }
            else
            {
                if (!mSilent)
                    Log(Debug::Warning) << "Path starts with unknown token: " << path;
                path.clear();
            }
        }
    }

    void ConfigurationManager::processPaths(Files::PathContainer& dataDirs, const std::filesystem::path& basePath) const
    {
        for (auto& path : dataDirs)
            processPath(path, basePath);
    }

    void ConfigurationManager::processPaths(
        boost::program_options::variables_map& variables, const std::filesystem::path& basePath) const
    {
        for (auto& [name, var] : variables)
        {
            if (var.defaulted())
                continue;
            if (var.value().type() == typeid(MaybeQuotedPathContainer))
            {
                auto& pathContainer = boost::any_cast<MaybeQuotedPathContainer&>(var.value());
                for (MaybeQuotedPath& path : pathContainer)
                    processPath(path, basePath);
            }
            else if (var.value().type() == typeid(MaybeQuotedPath))
            {
                std::filesystem::path& path = boost::any_cast<Files::MaybeQuotedPath&>(var.value());
                processPath(path, basePath);
            }
        }
    }

    void ConfigurationManager::filterOutNonExistingPaths(Files::PathContainer& dataDirs) const
    {
        dataDirs.erase(std::remove_if(dataDirs.begin(), dataDirs.end(),
                           [this](const std::filesystem::path& p) {
                               bool exists = std::filesystem::is_directory(p);
                               if (!exists && !mSilent)
                                   Log(Debug::Warning) << "No such dir: " << p;
                               return !exists;
                           }),
            dataDirs.end());
    }

    std::optional<bpo::variables_map> ConfigurationManager::loadConfig(
        const std::filesystem::path& path, const bpo::options_description& description) const
    {
        std::filesystem::path cfgFile(path);
        cfgFile /= openmwCfgFile;
        if (std::filesystem::is_regular_file(cfgFile))
        {
            if (!mSilent)
                Log(Debug::Info) << "Loading config file: " << cfgFile;

            std::ifstream configFileStream(cfgFile);

            if (configFileStream.is_open())
            {
                bpo::variables_map variables;
                bpo::store(Files::parse_config_file(configFileStream, description, true), variables);
                processPaths(variables, path);
                return variables;
            }
            else if (!mSilent)
                Log(Debug::Error) << "Loading failed.";
        }
        return std::nullopt;
    }

    const std::filesystem::path& ConfigurationManager::getGlobalPath() const
    {
        return mFixedPath.getGlobalConfigPath();
    }

    const std::filesystem::path& ConfigurationManager::getUserConfigPath() const
    {
        if (mActiveConfigPaths.empty())
            return mFixedPath.getUserConfigPath();
        else
            return mActiveConfigPaths.back();
    }

    const std::filesystem::path& ConfigurationManager::getUserDataPath() const
    {
        return mUserDataPath;
    }

    const std::filesystem::path& ConfigurationManager::getLocalPath() const
    {
        return mFixedPath.getLocalPath();
    }

    const std::filesystem::path& ConfigurationManager::getCachePath() const
    {
        return mFixedPath.getCachePath();
    }

    const std::vector<std::filesystem::path>& ConfigurationManager::getInstallPaths() const
    {
        return mFixedPath.getInstallPaths();
    }

    const std::filesystem::path& ConfigurationManager::getScreenshotPath() const
    {
        return mScreenshotPath;
    }

    void parseArgs(
        int argc, const char* const argv[], bpo::variables_map& variables, const bpo::options_description& description)
    {
        bpo::store(bpo::command_line_parser(argc, argv).options(description).allow_unregistered().run(), variables);
    }

    void parseConfig(std::istream& stream, bpo::variables_map& variables, const bpo::options_description& description)
    {
        bpo::store(Files::parse_config_file(stream, description, true), variables);
    }

    std::istream& operator>>(std::istream& istream, MaybeQuotedPath& value)
    {
        // If the stream starts with a double quote, read from stream using boost::filesystem::path rules (which are not
        // the same as std::filesystem::path rules), then discard anything remaining. This prevents
        // boost::program_options getting upset that we've not consumed the whole stream. If it doesn't start with a
        // double quote, read the whole thing verbatim.

        // This function's implementation and comments have a history of being changed by well-meaning but incorrect
        // OpenMW developers. Please be very careful you aren't joining them. If you're insufficiently apprehensive,
        // here's a 737-word GitLab comment to scare you:
        // https://gitlab.com/OpenMW/openmw/-/merge_requests/2979#note_1371082428.
        if (istream.peek() == '"')
        {
            std::string intermediate;
            // std::filesystem::path would use '"', '\' here.
            // We use & because it's easier when copying and pasting Windows paths, which have many backslashes and few
            // ampersands, and because it's backwards-compatible with the previous format, which used
            // boost::filesystem::path's operator>>.
            istream >> std::quoted(intermediate, '"', '&');
            static_cast<std::filesystem::path&>(value) = Misc::StringUtils::stringToU8String(intermediate);
            if (istream && !istream.eof() && istream.peek() != EOF)
            {
                std::string remainder{ std::istreambuf_iterator(istream), {} };
                Log(Debug::Warning) << "Trailing data in path setting. Used '" << value << "' but '" << remainder
                                    << "' remained";
            }
        }
        else
        {
            std::string intermediate{ std::istreambuf_iterator(istream), {} };
            static_cast<std::filesystem::path&>(value) = Misc::StringUtils::stringToU8String(intermediate);
        }
        return istream;
    }

    PathContainer asPathContainer(const MaybeQuotedPathContainer& value)
    {
        PathContainer res;
        res.reserve(value.size());
        for (const auto& maybeQuotedPath : value)
        {
            res.emplace_back(maybeQuotedPath.u8string()); // This call to u8string is redundant, but required to build
                                                          // on MSVC 14.26 due to implementation bugs.
        }
        return res;
    }

} /* namespace Files */
