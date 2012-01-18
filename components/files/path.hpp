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

/** \file components/files/path.hpp */

#ifndef COMPONENTS_FILES_PATH_HPP
#define COMPONENTS_FILES_PATH_HPP

#include <string>
#include <boost/filesystem.hpp>

#if defined(__linux__) || defined(__FreeBSD__)
    #include <components/files/linuxpath.hpp>
    namespace Files { typedef LinuxPath TargetPathType; }

#elif defined(__WIN32) || defined(__WINDOWS__)
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
struct Path
{
    typedef P PathType;

    /**
     * \brief Path constructor.
     *
     * \param [in] application_name - Name of the application
     */
    Path(const std::string& application_name)
        : mPath()
        , mLocalConfigPath(mPath.getLocalConfigPath())
        , mGlobalConfigPath(mPath.getGlobalConfigPath())
        , mRuntimeConfigPath(mPath.getRuntimeConfigPath())
        , mLocalDataPath(mPath.getLocalDataPath())
        , mGlobalDataPath(mPath.getGlobalDataPath())
        , mRuntimeDataPath(mPath.getRuntimeDataPath())
    {
        if (!application_name.empty())
        {
            boost::filesystem::path suffix(application_name + std::string("/"));

            mLocalConfigPath /= suffix;
            mGlobalConfigPath /= suffix;

            mLocalDataPath /= suffix;
            mGlobalDataPath /= suffix;
        }
    }

    /**
     * \brief Return path pointing to the user local configuration directory.
     *
     * \return boost::filesystem::path
     */
    const boost::filesystem::path& getLocalConfigPath() const
    {
        return mLocalConfigPath;
    }

    /**
     * \brief Sets new local configuration path.
     *
     * \param [in] path - New path
     */
    void setLocalConfigPath(const boost::filesystem::path& path)
    {
        mLocalConfigPath = path;
    }

    /**
     * \brief Return path pointing to the global (system) configuration directory.
     *
     * \return boost::filesystem::path
     */
    const boost::filesystem::path& getGlobalConfigPath() const
    {
        return mGlobalConfigPath;
    }

    /**
     * \brief Sets new global configuration path.
     *
     * \param [in] path - New path
     */
    void setGlobalConfigPath(const boost::filesystem::path& path)
    {
        mGlobalConfigPath = path;
    }

    /**
     * \brief Return path pointing to the directory where application was started.
     *
     * \return boost::filesystem::path
     */
    const boost::filesystem::path& getRuntimeConfigPath() const
    {
        return mRuntimeConfigPath;
    }

    /**
     * \brief Sets new runtime configuration path.
     *
     * \param [in] path - New path
     */
    void setRuntimeConfigPath(const boost::filesystem::path& path)
    {
        mRuntimeConfigPath = path;
    }

    /**
     * \brief Return path pointing to the user local data directory.
     *
     * \return boost::filesystem::path
     */
    const boost::filesystem::path& getLocalDataPath() const
    {
        return mLocalDataPath;
    }

    /**
     * \brief Sets new local data path.
     *
     * \param [in] path - New path
     */
    void setLocalDataPath(const boost::filesystem::path& path)
    {
        mLocalDataPath = path;
    }

    /**
     * \brief Return path pointing to the global (system) data directory.
     *
     * \return boost::filesystem::path
     */
    const boost::filesystem::path& getGlobalDataPath() const
    {
        return mGlobalDataPath;
    }

    /**
     * \brief Sets new global (system) data directory.
     *
     * \param [in] path - New path
     */
    void setGlobalDataPath(const boost::filesystem::path& path)
    {
        mGlobalDataPath = path;
    }

    /**
     * \brief Return path pointing to the directory where application was started.
     *
     * \return boost::filesystem::path
     */
    const boost::filesystem::path& getRuntimeDataPath() const
    {
        return mRuntimeDataPath;
    }

    /**
     * \brief Sets new runtime data directory.
     *
     * \param [in] path - New path
     */
    void setRuntimeDataPath(const boost::filesystem::path& path)
    {
        mRuntimeDataPath = path;
    }

    private:
        PathType mPath;

        boost::filesystem::path mLocalConfigPath;        /**< User local path to the configuration files */
        boost::filesystem::path mGlobalConfigPath;       /**< Global path to the configuration files */
        boost::filesystem::path mRuntimeConfigPath;      /**< Runtime path to the configuration files.
                                                              By default it is the same directory where
                                                              application was run */

        boost::filesystem::path mLocalDataPath;          /**< User local application data path (user plugins / mods / etc.) */
        boost::filesystem::path mGlobalDataPath;         /**< Global application data path */
        boost::filesystem::path mRuntimeDataPath;        /**< Runtime path to the configuration files.
                                                              By default it is a 'data' directory in same
                                                              directory where application was run */

};


} /* namespace Files */

#endif /* COMPONENTS_FILES_PATH_HPP */
