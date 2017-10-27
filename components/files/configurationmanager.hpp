#ifndef COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP
#define COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP

#include <map>
#include <experimental/filesystem>

#include <boost/program_options.hpp>

#include <components/files/fixedpath.hpp>
#include <components/files/collections.hpp>



/**
 * \namespace Files
 */
namespace Files
{

/**
 * \struct ConfigurationManager
 */
struct ConfigurationManager
{
    ConfigurationManager(bool silent=false); /// @param silent Emit log messages to cout?
    virtual ~ConfigurationManager();

    void readConfiguration(boost::program_options::variables_map& variables,
        boost::program_options::options_description& description, bool quiet=false);

    void processPaths(Files::PathContainer& dataDirs, bool create = false);
    ///< \param create Try creating the directory, if it does not exist.

    /**< Fixed paths */
    const std::experimental::filesystem::path& getGlobalPath() const;
    const std::experimental::filesystem::path& getUserConfigPath() const;
    const std::experimental::filesystem::path& getLocalPath() const;

    const std::experimental::filesystem::path& getGlobalDataPath() const;
    const std::experimental::filesystem::path& getUserDataPath() const;
    const std::experimental::filesystem::path& getLocalDataPath() const;
    const std::experimental::filesystem::path& getInstallPath() const;

    const std::experimental::filesystem::path& getCachePath() const;

    const std::experimental::filesystem::path& getLogPath() const;

    private:
        typedef Files::FixedPath<> FixedPathType;

        typedef const std::experimental::filesystem::path& (FixedPathType::*path_type_f)() const;
        typedef std::map<std::string, path_type_f> TokensMappingContainer;

        bool loadConfig(const std::experimental::filesystem::path& path,
            boost::program_options::variables_map& variables,
            boost::program_options::options_description& description);

        void setupTokensMapping();

        FixedPathType mFixedPath;

        std::experimental::filesystem::path mLogPath;

        TokensMappingContainer mTokensMapping;

        bool mSilent;
};
} /* namespace Cfg */

#endif /* COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP */
