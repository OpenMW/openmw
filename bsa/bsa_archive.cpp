/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (cpp_bsaarchive.cpp) is part of the OpenMW package.

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

#ifndef _BSA_ARCHIVE_H_
#define _BSA_ARCHIVE_H_


/* This file inserts an archive manager for .bsa files into the OGRE
   resource loading system.
*/

#include <OgreArchive.h>
#include "bsa_file.h"

// This archive does not cover one .bsa file, instead it interacts
// with the all the loaded bsas and treates them as one archive.
class BSAArchive : public Archive
{
  BSAFile arc;

public:
  BSAArchive(const String& name)
             : Archive(name, "BSA")
  { arc.open(name); }

  bool isCaseSensitive(void) const { return false; }

  // The archive is loaded in the constructor, and never unloaded.
  void load() {}
  void unload() {}

  // Open a file in the archive.
  DataStreamPtr open(const String& filename) const
  {
    // Open the file, and wrap it into an Ogre::DataStream.
    return DataStreamPtr(new MangleDataStream(getFile(filename.c_str()));
  }

  // This is never called as far as I can see.
  StringVectorPtr list(bool recursive = true, bool dirs = false)
  {
    //std::cout << "list(" << recursive << ", " << dirs << ")\n";
    StringVectorPtr ptr = StringVectorPtr(new StringVector());
    return ptr;
  }
  // Also never called.
  FileInfoListPtr listFileInfo(bool recursive = true, bool dirs = false)
  {
    //std::cout << "listFileInfo(" << recursive << ", " << dirs << ")\n";
    FileInfoListPtr ptr = FileInfoListPtr(new FileInfoList());
    return ptr;
  }

  time_t getModifiedTime(const String&) { return 0; }

  // After load() is called, find("*") is called once. It doesn't seem
  // to matter that we return an empty list, exists() gets called on
  // the correct files anyway.
  StringVectorPtr find(const String& pattern, bool recursive = true,
                       bool dirs = false)
  {
    //std::cout << "find(" << pattern << ", " << recursive
    //          << ", " << dirs << ")\n";
    StringVectorPtr ptr = StringVectorPtr(new StringVector());
    return ptr;
  }

  /* Gets called once for each of the ogre formats, *.program,
     *.material etc. We ignore all these.

     However, it's also called by MyGUI to find individual textures,
     and we can't ignore these since many of the GUI textures are
     located in BSAs. So instead we channel it through exists() and
     set up a single-element result list if the file is found.
  */
  FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true,
                               bool dirs = false)
  {
    /*
    std::cout << "findFileInfo(" << pattern << ", " << recursive
              << ", " << dirs << ")\n";
    */

    FileInfoListPtr ptr = FileInfoListPtr(new FileInfoList());

    // Check if the file exists (only works for single files - wild
    // cards and recursive search isn't implemented.)
    if(exists(pattern))
      {
        FileInfo fi;
        fi.archive = this;
        fi.filename = pattern;
        // It apparently doesn't matter that we return bogus
        // information
        fi.path = "";
        fi.compressedSize = fi.uncompressedSize = 0;

        ptr->push_back(fi);
      }

    return ptr;
  }

  // Check if the file exists.
  bool exists(const String& filename) { return arc.exists(filename.c_str()); }
};

// An archive factory for BSA archives
class BSAArchiveFactory : public ArchiveFactory
{
public:
  virtual ~BSAArchiveFactory() {}

  const String& getType() const
  {
    static String name = "BSA";
    return name;
  }

  Archive *createInstance( const String& name ) 
  {
    return new BSAArchive(name);
  }

  void destroyInstance( Archive* arch) { delete arch; }
};

BSAArchiveFactory mBSAFactory;

#endif
