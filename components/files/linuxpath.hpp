#ifndef COMPONENTS_FILES_LINUXPATH_H
#define COMPONENTS_FILES_LINUXPATH_H

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

#include <filesystem>

/**
 * \namespace Files
 */
namespace Files
{

    /**
     * \struct LinuxPath
     */
    struct LinuxPath
    {
        LinuxPath(const std::string& application_name);

        /**
         * \brief Return path to the user directory.
         */
        std::filesystem::path getUserConfigPath() const;

        std::filesystem::path getUserDataPath() const;

        /**
         * \brief Return path to the global (system) directory where config files can be placed.
         */
        std::filesystem::path getGlobalConfigPath() const;

        /**
         * \brief Return path to the runtime configuration directory which is the
         * place where an application was started.
         */
        std::filesystem::path getLocalPath() const;

        /**
         * \brief Return path to the global (system) directory where game files can be placed.
         */
        std::filesystem::path getGlobalDataPath() const;

        /**
         * \brief
         */
        std::filesystem::path getCachePath() const;

        /**
         * \brief Gets the path of the installed Morrowind version if there is one.
         */
        std::filesystem::path getInstallPath() const;

        std::string mName;
    };

} /* namespace Files */

#endif /* defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) */

#endif /* COMPONENTS_FILES_LINUXPATH_H */
