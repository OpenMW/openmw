#ifndef COMPONENTS_FILES_ANDROIDPATH_H
#define COMPONENTS_FILES_ANDROIDPATH_H

#if defined(__ANDROID__)

#include <experimental/filesystem>
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
    std::experimental::filesystem::path getUserConfigPath() const;

    std::experimental::filesystem::path getUserDataPath() const;

    /**
     * \brief Return path to the global (system) directory where config files can be placed.
     */
    std::experimental::filesystem::path getGlobalConfigPath() const;

    /**
     * \brief Return path to the runtime configuration directory which is the
     * place where an application was started.
     */
    std::experimental::filesystem::path getLocalPath() const;

    /**
     * \brief Return path to the global (system) directory where game files can be placed.
     */
    std::experimental::filesystem::path getGlobalDataPath() const;

    /**
     * \brief
     */
    std::experimental::filesystem::path getCachePath() const;

    std::experimental::filesystem::path getInstallPath() const;

    std::string mName;
};

} /* namespace Files */

#endif /* defined(__Android__) */

#endif /* COMPONENTS_FILES_ANDROIDPATH_H */
