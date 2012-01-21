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

/** \file components/files/windowspath.hpp */

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
     * \brief Returns "X:\Documents And Settings\<User name>\My Documents\My Games\"
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
     * \brief Return same path like getUserConfigPath
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getUserDataPath() const;

    /**
     * \brief Return same path like getGlobalConfigPath
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getGlobalDataPath() const;

    /**
     * \brief Return runtime data path which is a location where
     * an application was started with 'data' suffix.
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getLocalDataPath() const;

    boost::filesystem::path getInstallPath() const;
};

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */

#endif /* COMPONENTS_FILES_WINDOWSPATH_HPP */
