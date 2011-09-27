#ifndef COMPONENTS_CFG_CONFIGURATIONMANAGER_HPP
#define COMPONENTS_CFG_CONFIGURATIONMANAGER_HPP

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <components/files/path.hpp>

/**
 * \namespace Cfg
 */
namespace Cfg
{

/**
 * \struct ConfigurationManager
 */
struct ConfigurationManager
{
    ConfigurationManager();
    virtual ~ConfigurationManager();

    void readConfiguration(boost::program_options::variables_map& variables,
        boost::program_options::options_description& description);

    const boost::filesystem::path& getGlobalConfigPath() const;
    void setGlobalConfigPath(const boost::filesystem::path& newPath);

    const boost::filesystem::path& getLocalConfigPath() const;
    void setLocalConfigPath(const boost::filesystem::path& newPath);

    const boost::filesystem::path& getRuntimeConfigPath() const;
    void setRuntimeConfigPath(const boost::filesystem::path& newPath);

    const boost::filesystem::path& getGlobalDataPath() const;
    void setGlobalDataPath(const boost::filesystem::path& newPath);

    const boost::filesystem::path& getLocalDataPath() const;
    void setLocalDataPath(const boost::filesystem::path& newPath);

    const boost::filesystem::path& getRuntimeDataPath() const;
    void setRuntimeDataPath(const boost::filesystem::path& newPath);

    const boost::filesystem::path& getOgreConfigPath() const;
    const boost::filesystem::path& getPluginsConfigPath() const;
    const boost::filesystem::path& getLogPath() const;

    private:
        void loadConfig(const boost::filesystem::path& path,
            boost::program_options::variables_map& variables,
            boost::program_options::options_description& description);

        Files::Path<> mPath;

        boost::filesystem::path mOgreCfgPath;
        boost::filesystem::path mPluginsCfgPath;
        boost::filesystem::path mLogPath;
};

} /* namespace Cfg */

#endif /* COMPONENTS_CFG_CONFIGURATIONMANAGER_HPP */
