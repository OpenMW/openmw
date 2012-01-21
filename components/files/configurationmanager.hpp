#ifndef COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP
#define COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP

#include <tr1/unordered_map>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <components/files/fixedpath.hpp>

/**
 * \namespace Files
 */
namespace Files
{

extern const char* const mwDataToken;
extern const char* const localDataToken;
extern const char* const userDataToken;
extern const char* const globalDataToken;


/**
 * \struct ConfigurationManager
 */
struct ConfigurationManager
{
    ConfigurationManager();
    virtual ~ConfigurationManager();

    void readConfiguration(boost::program_options::variables_map& variables,
        boost::program_options::options_description& description);

    /**< Fixed paths */
    const boost::filesystem::path& getGlobalPath() const;
    const boost::filesystem::path& getUserPath() const;
    const boost::filesystem::path& getLocalPath() const ;

    const boost::filesystem::path& getDataPath(const std::string& type) const;

    const boost::filesystem::path& getOgreConfigPath() const;
    const boost::filesystem::path& getPluginsConfigPath() const;
    const boost::filesystem::path& getLogPath() const;

    private:
        typedef Files::FixedPath<> FixedPathType;

        typedef const boost::filesystem::path& (FixedPathType::*path_type_f)() const;
        typedef std::tr1::unordered_map<std::string, path_type_f> TokensMappingContainer;

        void loadConfig(const boost::filesystem::path& path,
            boost::program_options::variables_map& variables,
            boost::program_options::options_description& description);

        void setupTokensMapping();

        FixedPathType mFixedPath;

        boost::filesystem::path mOgreCfgPath;
        boost::filesystem::path mPluginsCfgPath;
        boost::filesystem::path mLogPath;

        TokensMappingContainer mTokensMapping;
};

} /* namespace Cfg */

#endif /* COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP */
