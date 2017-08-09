#ifndef COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP
#define COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP

#include <map>

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
    const boost::filesystem::path& getGlobalPath() const;
    const boost::filesystem::path& getUserConfigPath() const;
    const boost::filesystem::path& getLocalPath() const;

    const boost::filesystem::path& getGlobalDataPath() const;
    const boost::filesystem::path& getUserDataPath() const;
    const boost::filesystem::path& getLocalDataPath() const;
    const boost::filesystem::path& getInstallPath() const;

    const boost::filesystem::path& getCachePath() const;

    const boost::filesystem::path& getLogPath() const;

    private:
        typedef Files::FixedPath<> FixedPathType;

        typedef const boost::filesystem::path& (FixedPathType::*path_type_f)() const;
        typedef std::map<std::string, path_type_f> TokensMappingContainer;

        bool loadConfig(const boost::filesystem::path& path,
            boost::program_options::variables_map& variables,
            boost::program_options::options_description& description);

        void setupTokensMapping();

        FixedPathType mFixedPath;

        boost::filesystem::path mLogPath;

        TokensMappingContainer mTokensMapping;

        bool mSilent;
};
} /* namespace Cfg */

#endif /* COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP */
