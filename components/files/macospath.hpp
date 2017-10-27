#ifndef COMPONENTS_FILES_MACOSPATH_H
#define COMPONENTS_FILES_MACOSPATH_H

#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)

#include <experimental/filesystem>



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
     * \return std::experimental::filesystem::path
     */
    std::experimental::filesystem::path getUserConfigPath() const;

    std::experimental::filesystem::path getUserDataPath() const;

    /**
     * \brief Return path to the global (system) directory.
     *
     * \return std::experimental::filesystem::path
     */
    std::experimental::filesystem::path getGlobalConfigPath() const;

    /**
     * \brief Return path to the runtime directory which is the
     * place where an application was started.
     *
     * \return std::experimental::filesystem::path
     */
    std::experimental::filesystem::path getLocalPath() const;

    /**
     * \brief
     *
     * \return std::experimental::filesystem::path
     */
    std::experimental::filesystem::path getCachePath() const;

    /**
     * \brief
     *
     * \return std::experimental::filesystem::path
     */
    std::experimental::filesystem::path getGlobalDataPath() const;

    std::experimental::filesystem::path getInstallPath() const;

    std::string mName;
};

} /* namespace Files */

#endif /* defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__) */

#endif /* COMPONENTS_FILES_MACOSPATH_H */
