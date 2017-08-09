#ifndef COMPONENTS_FILES_MACOSPATH_H
#define COMPONENTS_FILES_MACOSPATH_H

#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)

#include <boost/filesystem.hpp>

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
     * \return boost::filesystem::path
     */
    boost::filesystem::path getUserConfigPath() const;

    boost::filesystem::path getUserDataPath() const;

    /**
     * \brief Return path to the global (system) directory.
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getGlobalConfigPath() const;

    /**
     * \brief Return path to the runtime directory which is the
     * place where an application was started.
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getLocalPath() const;

    /**
     * \brief
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getCachePath() const;

    /**
     * \brief
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getGlobalDataPath() const;

    boost::filesystem::path getInstallPath() const;

    std::string mName;
};

} /* namespace Files */

#endif /* defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__) */

#endif /* COMPONENTS_FILES_MACOSPATH_H */
