/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (bsa_file.h) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  https://www.gnu.org/licenses/ .

 */

#ifndef BSA_BSA_FILE_H
#define BSA_BSA_FILE_H

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

#include <components/misc/stringops.hpp>

#include <components/files/constrainedfilestream.hpp>


namespace Bsa
{

/**
   This class is used to read "Bethesda Archive Files", or BSAs.
 */
class BSAFile
{
public:
    /// Represents one file entry in the archive
    struct FileStruct
    {
        // File size and offset in file. We store the offset from the
        // beginning of the file, not the offset into the data buffer
        // (which is what is stored in the archive.)
        uint32_t fileSize, offset;

        // Zero-terminated file name
        const char *name;
    };
    typedef std::vector<FileStruct> FileList;

private:
    /// Table of files in this archive
    FileList files;

    /// Filename string buffer
    std::vector<char> stringBuf;

    /// True when an archive has been loaded
    bool isLoaded;

    /// Used for error messages
    std::string filename;

    /// Case insensitive string comparison
    struct iltstr
    {
        bool operator()(const char *s1, const char *s2) const
        { return Misc::StringUtils::ciLess(s1, s2); }
    };

    /** A map used for fast file name lookup. The value is the index into
        the files[] vector above. The iltstr ensures that file name
        checks are case insensitive.
    */
    typedef std::map<const char*, int, iltstr> Lookup;
    Lookup lookup;

    /// Error handling
    void fail(const std::string &msg);

    /// Read header information from the input source
    void readHeader();

    /// Get the index of a given file name, or -1 if not found
    /// @note Thread safe.
    int getIndex(const char *str) const;

public:
    /* -----------------------------------
     * BSA management methods
     * -----------------------------------
     */

    BSAFile()
      : isLoaded(false)
    { }

    /// Open an archive file.
    void open(const std::string &file);

    /* -----------------------------------
     * Archive file routines
     * -----------------------------------
     */

    /// Check if a file exists
    bool exists(const char *file) const
    { return getIndex(file) != -1; }

    /** Open a file contained in the archive. Throws an exception if the
        file doesn't exist.
     * @note Thread safe.
    */
    Files::IStreamPtr getFile(const char *file);

    /** Open a file contained in the archive.
     * @note Thread safe.
    */
    Files::IStreamPtr getFile(const FileStruct* file);

    /// Get a list of all files
    /// @note Thread safe.
    const FileList &getList() const
    { return files; }
};

}

#endif
