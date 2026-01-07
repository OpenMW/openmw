#ifndef COMPONENTS_FILES_LINUXPATH_H
#define COMPONENTS_FILES_LINUXPATH_H

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

#include <filesystem>
#include <vector>

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
        explicit LinuxPath(const std::string& applicationName);

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
         * \brief Gets the paths of any installed Morrowind versions we can find.
         */
        std::vector<std::filesystem::path> getInstallPaths() const;

        std::string mName;
    };

} /* namespace Files */

#endif /* defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) */

#endif /* COMPONENTS_FILES_LINUXPATH_H */
