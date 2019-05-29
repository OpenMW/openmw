#ifndef COMPONENTS_FILES_SWITCHPATH_H
#define COMPONENTS_FILES_SWITCHPATH_H

#if defined(__SWITCH__)

#include <boost/filesystem.hpp>

/**
 * \namespace Files
 */
namespace Files
{

/**
 * \struct SwitchPath
 */
struct SwitchPath
{
    SwitchPath(const std::string& application_name);

    /**
     * \brief Return path to the user directory.
     */
    boost::filesystem::path getUserConfigPath() const;

    boost::filesystem::path getUserDataPath() const;

    /**
     * \brief Return path to the global (system) directory where config files can be placed.
     */
    boost::filesystem::path getGlobalConfigPath() const;

    /**
     * \brief Return path to the runtime configuration directory which is the
     * place where an application was started.
     */
    boost::filesystem::path getLocalPath() const;

    /**
     * \brief Return path to the global (system) directory where game files can be placed.
     */
    boost::filesystem::path getGlobalDataPath() const;

    /**
     * \brief
     */
    boost::filesystem::path getCachePath() const;

    /**
     * \brief Gets the path of the installed Morrowind version if there is one.
     */
    boost::filesystem::path getInstallPath() const;

    std::string mName;
};

} /* namespace Files */

#endif /* defined(__SWITCH__) */

#endif /* COMPONENTS_FILES_SWITCHPATH_H */
