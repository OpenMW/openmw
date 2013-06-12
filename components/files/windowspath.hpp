#ifndef COMPONENTS_FILES_WINDOWSPATH_HPP
#define COMPONENTS_FILES_WINDOWSPATH_HPP

#if defined(_WIN32) || defined(__WINDOWS__)

#include <boost/filesystem.hpp>

/**
 * \namespace Files
 */
namespace Files
{

/**
 * \struct WindowsPath
 */
struct WindowsPath
{
    /**
     * \brief WindowsPath constructor.
     *
     * \param [in] application_name - The name of the application.
     */
    WindowsPath(const std::string& application_name);

    /**
     * \brief Returns user path i.e.:
     * "X:\Documents And Settings\<User name>\My Documents\My Games\"
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getUserPath() const;

    /**
     * \brief Returns "X:\Program Files\"
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getGlobalPath() const;

    /**
     * \brief Return local path which is a location where
     * an application was started
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
     * \brief Return same path like getGlobalPath
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getGlobalDataPath() const;

    /**
     * \brief Gets the path of the installed Morrowind version if there is one.
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getInstallPath() const;

    std::string mName;
};

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */

#endif /* COMPONENTS_FILES_WINDOWSPATH_HPP */
