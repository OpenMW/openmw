#ifndef COMPONENTS_FILES_LINUXPATH_H
#define COMPONENTS_FILES_LINUXPATH_H

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

#include <experimental/filesystem>

namespace sfs = std::experimental::filesystem;

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

    /**
     * \brief Gets the path of the installed Morrowind version if there is one.
     */
    sfs::path getInstallPath() const;

    std::string mName;
};

} /* namespace Files */

#endif /* defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) */

#endif /* COMPONENTS_FILES_LINUXPATH_H */
