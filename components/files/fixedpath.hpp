#ifndef COMPONENTS_FILES_FIXEDPATH_HPP
#define COMPONENTS_FILES_FIXEDPATH_HPP

#include <string>
#include <boost/filesystem.hpp>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)
#ifndef ANDROID
    #include <components/files/linuxpath.hpp>
    namespace Files { typedef LinuxPath TargetPathType; }
#else
    #include <components/files/androidpath.hpp>
    namespace Files { typedef AndroidPath TargetPathType; }
#endif
#elif defined(__WIN32) || defined(__WINDOWS__) || defined(_WIN32)
    #include <components/files/windowspath.hpp>
    namespace Files { typedef WindowsPath TargetPathType; }

#elif defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)
    #include <components/files/macospath.hpp>
    namespace Files { typedef MacOsPath TargetPathType; }

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
template
<
    class P = TargetPathType
>
struct FixedPath
{
    typedef P PathType;

    /**
     * \brief Path constructor.
     *
     * \param [in] application_name - Name of the application
     */
    FixedPath(const std::string& application_name)
        : mPath(application_name + "/")
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
    const boost::filesystem::path& getUserConfigPath() const
    {
        return mUserConfigPath;
    }

    const boost::filesystem::path& getUserDataPath() const
    {
        return mUserDataPath;
    }

    /**
     * \brief Return path pointing to the global (system) configuration directory.
     */
    const boost::filesystem::path& getGlobalConfigPath() const
    {
        return mGlobalConfigPath;
    }

    /**
     * \brief Return path pointing to the directory where application was started.
     */
    const boost::filesystem::path& getLocalPath() const
    {
        return mLocalPath;
    }


    const boost::filesystem::path& getInstallPath() const
    {
        return mInstallPath;
    }

    const boost::filesystem::path& getGlobalDataPath() const
    {
        return mGlobalDataPath;
    }

    const boost::filesystem::path& getCachePath() const
    {
        return mCachePath;
    }

    private:
        PathType mPath;

        boost::filesystem::path mUserConfigPath;       /**< User path  */
        boost::filesystem::path mUserDataPath;
        boost::filesystem::path mGlobalConfigPath;     /**< Global path */
        boost::filesystem::path mLocalPath;      /**< It is the same directory where application was run */

        boost::filesystem::path mGlobalDataPath;        /**< Global application data path */

        boost::filesystem::path mCachePath;

        boost::filesystem::path mInstallPath;

};


} /* namespace Files */

#endif /* COMPONENTS_FILES_FIXEDPATH_HPP */
