/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

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
  http://www.gnu.org/licenses/ .

 */

#ifndef _BSA_FILE_H_
#define _BSA_FILE_H_

#include <libs/mangle/stream/stream.hpp>
#include <libs/platform/stdint.h>
#include <libs/platform/strings.h>
#include <string>
#include <vector>
#include <map>

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
    char* name;
    bool external; // Offset is set to 0 if this is true
  };

  typedef std::vector<FileStruct> FileList;

 private:

  /// The archive source
  Mangle::Stream::StreamPtr input;
  Mangle::Stream::StreamPtr external; // Used for opening an external file temporarily

  /// Table of files in this archive
  FileList files;

  /// Filename string buffer
  std::vector<char> stringBuf;

  /// True when an archive has been loaded
  bool isLoaded;

  /// Used for error messages
  std::string filename;
  std::string data_dir; // Where might 'filename' have newer files to load than what is in the BSA?

  /// Case insensitive string comparison
  struct iltstr
  {
    bool operator()(const char *s1, const char *s2) const
    { return strcasecmp(s1,s2) < 0; }
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
  int getIndex(const char *str);

 public:

  /* -----------------------------------
   * BSA management methods
   * -----------------------------------
   */

  BSAFile()
    : input(), isLoaded(false) {}

  /// Open an archive file.
  void open(const std::string &file, const std::string &data);

  /** Open an archive from a generic stream. The 'name' parameter is
      used for error messages.
  */
  void open(Mangle::Stream::StreamPtr inp, const std::string &name, const std::string &data);

  /* -----------------------------------
   * Archive file routines
   * -----------------------------------
   */

  /// Check if a file exists
  bool exists(const char *file) { return getIndex(file) != -1; }

  /** Open a file contained in the archive. Throws an exception if the
      file doesn't exist.

      NOTE: All files opened from one archive will share a common file
      handle. This is NOT thread safe.
   */
  Mangle::Stream::StreamPtr getFile(const char *file);

  /// Get a list of all files
  const FileList &getList() const
    { return files; }
};

#endif
