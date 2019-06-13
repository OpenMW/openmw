#ifndef COMPONENTS_FILES_ANDROIDPATH_H
#define COMPONENTS_FILES_ANDROIDPATH_H

#if defined(__ANDROID__)

#include <boost/filesystem.hpp>
/**
 * \namespace Files
 */


namespace Files
{

struct AndroidPath
{
    AndroidPath(const std::string& application_name);
    

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

    boost::filesystem::path getInstallPath() const;
};

} /* namespace Files */

#endif /* defined(__Android__) */

#endif /* COMPONENTS_FILES_ANDROIDPATH_H */
