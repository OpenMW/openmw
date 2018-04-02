#ifndef COMPONENTS_FILES_ANDROIDPATH_H
#define COMPONENTS_FILES_ANDROIDPATH_H

#if defined(__ANDROID__)

#include <experimental/filesystem>
/**
 * \namespace Files
 */

namespace sfs = std::experimental::filesystem;

namespace Files
{

struct AndroidPath
{
    AndroidPath(const std::string& application_name);
    

    /**
     * \brief Return path to the user directory.
     */
    sfs::path getUserConfigPath() const;

    sfs::path getUserDataPath() const;

    /**
     * \brief Return path to the global (system) directory where config files can be placed.
     */
    sfs::path getGlobalConfigPath() const;

    /**
     * \brief Return path to the runtime configuration directory which is the
     * place where an application was started.
     */
    sfs::path getLocalPath() const;

    /**
     * \brief Return path to the global (system) directory where game files can be placed.
     */
    sfs::path getGlobalDataPath() const;

    /**
     * \brief
     */
    sfs::path getCachePath() const;

    sfs::path getInstallPath() const;

    std::string mName;
};

} /* namespace Files */

#endif /* defined(__Android__) */

#endif /* COMPONENTS_FILES_ANDROIDPATH_H */
