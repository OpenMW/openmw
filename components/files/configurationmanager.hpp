#ifndef COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP
#define COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP

#include <map>
#include <experimental/filesystem>

#include <boost/program_options.hpp>

#include <components/files/fixedpath.hpp>
#include <components/files/collections.hpp>

namespace sfs = std::experimental::filesystem;

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
    const sfs::path& getGlobalPath() const;
    const sfs::path& getUserConfigPath() const;
    const sfs::path& getLocalPath() const;

    const sfs::path& getGlobalDataPath() const;
    const sfs::path& getUserDataPath() const;
    const sfs::path& getLocalDataPath() const;
    const sfs::path& getInstallPath() const;

    const sfs::path& getCachePath() const;

    const sfs::path& getLogPath() const;

    private:
        typedef Files::FixedPath<> FixedPathType;

        typedef const sfs::path& (FixedPathType::*path_type_f)() const;
        typedef std::map<std::string, path_type_f> TokensMappingContainer;

        bool loadConfig(const sfs::path& path,
            boost::program_options::variables_map& variables,
            boost::program_options::options_description& description);

        void setupTokensMapping();

        FixedPathType mFixedPath;

        sfs::path mLogPath;

        TokensMappingContainer mTokensMapping;

        bool mSilent;
};
} /* namespace Cfg */

#endif /* COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP */
