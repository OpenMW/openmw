#ifndef COMPONENTS_FILES_LINUXPATH_H
#define COMPONENTS_FILES_LINUXPATH_H

#if defined(__linux__) || defined(__FreeBSD__)

#include <boost/filesystem.hpp>

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
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getUserPath() const;

    /**
     * \brief Return path to the global (system) directory where game files could be placed.
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getGlobalPath() const;

    /**
     * \brief Return path to the runtime configuration directory which is the
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
    boost::filesystem::path getGlobalDataPath() const;

    /**
     * \brief
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getCachePath() const;

    /**
     * \brief Gets the path of the installed Morrowind version if there is one.
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getInstallPath() const;

    std::string mName;
};

} /* namespace Files */

#endif /* defined(__linux__) || defined(__FreeBSD__) */

#endif /* COMPONENTS_FILES_LINUXPATH_H */
