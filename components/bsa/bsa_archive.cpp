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

#include "bsa_archive.hpp"

#include <OgreFileSystem.h>
#include <OgreArchive.h>
#include <OgreArchiveFactory.h>
#include <OgreArchiveManager.h>
#include "bsa_file.hpp"
#include <libs/mangle/stream/clients/ogre_datastream.hpp>

using namespace Ogre;
using namespace Mangle::Stream;

/// An OGRE Archive wrapping a BSAFile archive
class DirArchive: public Ogre::FileSystemArchive
{
   //FileSystemArchive* arc;
    public:

    DirArchive(const String& name)
             : FileSystemArchive(name, "Dir")
  { mType = "Dir";}

    bool isCaseSensitive() const { return false; }

  // The archive is loaded in the constructor, and never unloaded.
    void load() {}
    void unload() {}

    DataStreamPtr open(const String& filename, bool readonly = true) const
  {
      std::string copy = filename;
      return FileSystemArchive::open(copy, readonly);
  }

};

class BSAArchive : public Archive
{
  BSAFile arc;

public:
  BSAArchive(const String& name)
             : Archive(name, "BSA")
  { arc.open(name); }

  bool isCaseSensitive() const { return false; }

  // The archive is loaded in the constructor, and never unloaded.
  void load() {}
  void unload() {}

  DataStreamPtr open(const String& filename, bool recursive = true) const
  {
    // Get a non-const reference to arc. This is a hack and it's all
    // OGRE's fault. You should NOT expect an open() command not to
    // have any side effects on the archive, and hence this function
    // should not have been declared const in the first place.
    BSAFile *narc = (BSAFile*)&arc;

    // Open the file
    StreamPtr strm = narc->getFile(filename.c_str());

    // Wrap it into an Ogre::DataStream.
    return DataStreamPtr(new Mangle2OgreStream(strm));
  }

  // Check if the file exists.
  bool exists(const String& filename) { return arc.exists(filename.c_str()); }
  time_t getModifiedTime(const String&) { return 0; }

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
};

// An archive factory for BSA archives
class BSAArchiveFactory : public ArchiveFactory
{
public:
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

class DirArchiveFactory : public FileSystemArchiveFactory
{
public:
  const String& getType() const
  {
    static String name = "Dir";
    return name;
  }

  Archive *createInstance( const String& name )
  {
    return new DirArchive(name);
  }

  void destroyInstance( Archive* arch) { delete arch; }
};


static bool init = false;
static bool init2 = false;
static void insertBSAFactory()
{
  if(!init)
    {
      ArchiveManager::getSingleton().addArchiveFactory( new BSAArchiveFactory );
      init = true;
    }
}

static void insertDirFactory()
{
  if(!init2)
    {
      ArchiveManager::getSingleton().addArchiveFactory( new DirArchiveFactory );
      init = true;
    }
}

// The function below is the only publicly exposed part of this file

void addBSA(const std::string& name, const std::string& group)
{
  insertBSAFactory();
  ResourceGroupManager::getSingleton().
    addResourceLocation(name, "BSA", group);
}
void addDir(const std::string& name, const std::string& group)
{
    insertDirFactory();
  ResourceGroupManager::getSingleton().
    addResourceLocation(name, "Dir", group);
}
