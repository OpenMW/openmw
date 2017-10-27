#ifndef COMPONENTS_FILES_WINDOWSPATH_HPP
#define COMPONENTS_FILES_WINDOWSPATH_HPP

#if defined(_WIN32) || defined(__WINDOWS__)

#include <experimental/filesystem>



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
     * \return std::experimental::filesystem::path
     */
    std::experimental::filesystem::path getUserConfigPath() const;

    std::experimental::filesystem::path getUserDataPath() const;

    /**
     * \brief Returns "X:\Program Files\"
     *
     * \return std::experimental::filesystem::path
     */
    std::experimental::filesystem::path getGlobalConfigPath() const;

    /**
     * \brief Return local path which is a location where
     * an application was started
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
     * \brief Return same path like getGlobalPath
     *
     * \return std::experimental::filesystem::path
     */
    std::experimental::filesystem::path getGlobalDataPath() const;

    /**
     * \brief Gets the path of the installed Morrowind version if there is one.
     *
     * \return std::experimental::filesystem::path
     */
    std::experimental::filesystem::path getInstallPath() const;

    std::string mName;
};

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */

#endif /* COMPONENTS_FILES_WINDOWSPATH_HPP */
