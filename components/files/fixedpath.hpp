#ifndef COMPONENTS_FILES_FIXEDPATH_HPP
#define COMPONENTS_FILES_FIXEDPATH_HPP

#include <filesystem>
#include <string>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)
#ifndef ANDROID
#include <components/files/linuxpath.hpp>
namespace Files
{
    typedef LinuxPath TargetPathType;
}
#else
#include <components/files/androidpath.hpp>
namespace Files
{
    typedef AndroidPath TargetPathType;
}
#endif
#elif defined(__WIN32) || defined(__WINDOWS__) || defined(_WIN32)
#include <components/files/windowspath.hpp>
namespace Files
{
    typedef WindowsPath TargetPathType;
}

#elif defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)
#include <components/files/macospath.hpp>
namespace Files
{
    typedef MacOsPath TargetPathType;
}

#else
#error "Unknown platform!"
#endif

/**
 * \namespace Files
 */
namespace Files
{

    /**
     * \struct Path
     *
     * \tparam P - Path strategy class type (depends on target system)
     *
     */
    template <class P = TargetPathType>
    struct FixedPath
    {
        typedef P PathType;

        /**
         * \brief Path constructor.
         *
         * \param [in] applicationName - Name of the application
         */
        FixedPath(const std::string& applicationName)
            : mPath(applicationName + "/")
            , mUserConfigPath(mPath.getUserConfigPath())
            , mUserDataPath(mPath.getUserDataPath())
            , mGlobalConfigPath(mPath.getGlobalConfigPath())
            , mLocalPath(mPath.getLocalPath())
            , mGlobalDataPath(mPath.getGlobalDataPath())
            , mCachePath(mPath.getCachePath())
            , mInstallPath(mPath.getInstallPath())
        {
        }

        /**
         * \brief Return path pointing to the user local configuration directory.
         */
        const std::filesystem::path& getUserConfigPath() const { return mUserConfigPath; }

        const std::filesystem::path& getUserDataPath() const { return mUserDataPath; }

        /**
         * \brief Return path pointing to the global (system) configuration directory.
         */
        const std::filesystem::path& getGlobalConfigPath() const { return mGlobalConfigPath; }

        /**
         * \brief Return path pointing to the directory where application was started.
         */
        const std::filesystem::path& getLocalPath() const { return mLocalPath; }

        const std::filesystem::path& getInstallPath() const { return mInstallPath; }

        const std::filesystem::path& getGlobalDataPath() const { return mGlobalDataPath; }

        const std::filesystem::path& getCachePath() const { return mCachePath; }

    private:
        PathType mPath;

        std::filesystem::path mUserConfigPath; /**< User path  */
        std::filesystem::path mUserDataPath;
        std::filesystem::path mGlobalConfigPath; /**< Global path */
        std::filesystem::path mLocalPath; /**< It is the same directory where application was run */

        std::filesystem::path mGlobalDataPath; /**< Global application data path */

        std::filesystem::path mCachePath;

        std::filesystem::path mInstallPath;
    };

} /* namespace Files */

#endif /* COMPONENTS_FILES_FIXEDPATH_HPP */
