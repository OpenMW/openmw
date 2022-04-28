#ifndef COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP
#define COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP

#include <map>
#include <optional>
#include <stack>

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
        const boost::program_options::options_description& description, bool quiet=false);

    void filterOutNonExistingPaths(Files::PathContainer& dataDirs) const;

    // Replaces tokens (`?local?`, `?global?`, etc.) in paths. Adds `basePath` prefix for relative paths.
    void processPath(boost::filesystem::path& path, const boost::filesystem::path& basePath) const;
    void processPaths(Files::PathContainer& dataDirs, const boost::filesystem::path& basePath) const;
    void processPaths(boost::program_options::variables_map& variables, const boost::filesystem::path& basePath) const;

    /**< Fixed paths */
    const boost::filesystem::path& getGlobalPath() const;
    const boost::filesystem::path& getLocalPath() const;

    const boost::filesystem::path& getGlobalDataPath() const;
    const boost::filesystem::path& getUserConfigPath() const;
    const boost::filesystem::path& getUserDataPath() const;
    const boost::filesystem::path& getLocalDataPath() const;
    const boost::filesystem::path& getInstallPath() const;
    const std::vector<boost::filesystem::path>& getActiveConfigPaths() const { return mActiveConfigPaths; }

    const boost::filesystem::path& getCachePath() const;

    const boost::filesystem::path& getLogPath() const { return getUserConfigPath(); }
    const boost::filesystem::path& getScreenshotPath() const;

    static void addCommonOptions(boost::program_options::options_description& description);

    private:
        typedef Files::FixedPath<> FixedPathType;

        typedef const boost::filesystem::path& (FixedPathType::*path_type_f)() const;
        typedef std::map<std::string, path_type_f> TokensMappingContainer;

        std::optional<boost::program_options::variables_map> loadConfig(
            const boost::filesystem::path& path,
            const boost::program_options::options_description& description) const;

        void addExtraConfigDirs(std::stack<boost::filesystem::path>& dirs,
            const boost::program_options::variables_map& variables) const;

        void setupTokensMapping();

        std::vector<boost::filesystem::path> mActiveConfigPaths;

        FixedPathType mFixedPath;

        boost::filesystem::path mUserDataPath;
        boost::filesystem::path mScreenshotPath;

        TokensMappingContainer mTokensMapping;

        bool mSilent;
};

boost::program_options::variables_map separateComposingVariables(boost::program_options::variables_map& variables,
    const boost::program_options::options_description& description);

void mergeComposingVariables(boost::program_options::variables_map& first, boost::program_options::variables_map& second,
    const boost::program_options::options_description& description);

void parseArgs(int argc, const char* const argv[], boost::program_options::variables_map& variables,
    const boost::program_options::options_description& description);

void parseConfig(std::istream& stream, boost::program_options::variables_map& variables,
    const boost::program_options::options_description& description);

class MaybeQuotedPath : public boost::filesystem::path
{
};

std::istream& operator>> (std::istream& istream, MaybeQuotedPath& MaybeQuotedPath);

typedef std::vector<MaybeQuotedPath> MaybeQuotedPathContainer;

PathContainer asPathContainer(const MaybeQuotedPathContainer& MaybeQuotedPathContainer);

} /* namespace Files */

#endif /* COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP */
