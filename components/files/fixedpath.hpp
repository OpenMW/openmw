/**
 *  Open Morrowind - an opensource Elder Scrolls III: Morrowind
 *  engine implementation.
 *
 *  Copyright (C) 2011 Open Morrowind Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file components/files/fixedpath.hpp */

#ifndef COMPONENTS_FILES_FIXEDPATH_HPP
#define COMPONENTS_FILES_FIXEDPATH_HPP

#include <string>
#include <boost/filesystem.hpp>

#if defined(__linux__) || defined(__FreeBSD__)
    #include <components/files/linuxpath.hpp>
    namespace Files { typedef LinuxPath TargetPathType; }

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
        : mPath()
        , mUserPath(mPath.getUserPath())
        , mGlobalPath(mPath.getGlobalPath())
        , mLocalPath(mPath.getLocalPath())
        , mGlobalDataPath(mPath.getGlobalDataPath())
        , mInstallPath(mPath.getInstallPath())
        , mCachePath(mPath.getCachePath())
    {
        if (!application_name.empty())
        {
            boost::filesystem::path suffix(application_name + std::string("/"));

            mUserPath /= suffix;
            mGlobalPath /= suffix;
            mGlobalDataPath /= suffix;
            mCachePath /= suffix;
#ifdef _WIN32
            mCachePath /= "cache";
#endif
        }
    }

    /**
     * \brief Return path pointing to the user local configuration directory.
     *
     * \return boost::filesystem::path
     */
    const boost::filesystem::path& getUserPath() const
    {
        return mUserPath;
    }

    /**
     * \brief Return path pointing to the global (system) configuration directory.
     *
     * \return boost::filesystem::path
     */
    const boost::filesystem::path& getGlobalPath() const
    {
        return mGlobalPath;
    }

    /**
     * \brief Return path pointing to the directory where application was started.
     *
     * \return boost::filesystem::path
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

        boost::filesystem::path mUserPath;       /**< User path  */
        boost::filesystem::path mGlobalPath;     /**< Global path */
        boost::filesystem::path mLocalPath;      /**< It is the same directory where application was run */

        boost::filesystem::path mGlobalDataPath;        /**< Global application data path */

        boost::filesystem::path mCachePath;

        boost::filesystem::path mInstallPath;

};


} /* namespace Files */

#endif /* COMPONENTS_FILES_FIXEDPATH_HPP */
