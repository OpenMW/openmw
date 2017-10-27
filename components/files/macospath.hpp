#ifndef COMPONENTS_FILES_MACOSPATH_H
#define COMPONENTS_FILES_MACOSPATH_H

#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)

#include <experimental/filesystem>

namespace sfs = std::experimental::filesystem;

/**
 * \namespace Files
 */
namespace Files
{

/**
 * \struct MacOsPath
 */
struct MacOsPath
{
    MacOsPath(const std::string& application_name);

    /**
     * \brief Return path to the local directory.
     *
     * \return sfs::path
     */
    sfs::path getUserConfigPath() const;

    sfs::path getUserDataPath() const;

    /**
     * \brief Return path to the global (system) directory.
     *
     * \return sfs::path
     */
    sfs::path getGlobalConfigPath() const;

    /**
     * \brief Return path to the runtime directory which is the
     * place where an application was started.
     *
     * \return sfs::path
     */
    sfs::path getLocalPath() const;

    /**
     * \brief
     *
     * \return sfs::path
     */
    sfs::path getCachePath() const;

    /**
     * \brief
     *
     * \return sfs::path
     */
    sfs::path getGlobalDataPath() const;

    sfs::path getInstallPath() const;

    std::string mName;
};

} /* namespace Files */

#endif /* defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__) */

#endif /* COMPONENTS_FILES_MACOSPATH_H */
