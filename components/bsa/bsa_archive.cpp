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

#include "../files/constrainedfiledatastream.hpp"

namespace
{

using namespace Ogre;
using namespace Bsa;

struct ciLessBoost : std::binary_function<std::string, std::string, bool>
{
    bool operator() (const std::string & s1, const std::string & s2) const {
                                               //case insensitive version of is_less
        return boost::ilexicographical_compare(s1, s2);
    }
};

struct pathComparer
{
private:
    std::string find;

public:
    pathComparer(const std::string& toFind) : find(toFind) { }

    bool operator() (const std::string& other)
    {
        return boost::iequals(find, other);
    }
};

static bool fsstrict = false;

/// An OGRE Archive wrapping a BSAFile archive
class DirArchive: public Ogre::Archive
{
    typedef std::map <std::string, std::string> index;

    index mIndex;

    static char strict_normalize_char(char ch)
    {
        return ch == '\\' ? '/' : ch;
    }

    static char nonstrict_normalize_char(char ch)
    {
        return ch == '\\' ? '/' : std::tolower (ch);
    }

    static std::string normalize_path (std::string::const_iterator begin, std::string::const_iterator end)
    {
        std::string normalized;
        normalized.reserve (end-begin);
        char (*normalize_char) (char) = fsstrict ? &strict_normalize_char : &nonstrict_normalize_char;
        std::transform (begin, end, std::back_inserter (normalized), normalize_char);
        return normalized;
    }

    index::const_iterator lookup_filename (std::string const & filename) const
    {
        std::string normalized = normalize_path (filename.begin (), filename.end ());

        return mIndex.find (normalized);
    }

public:

    DirArchive(const String& name)
        : Archive(name, "Dir")
    {
        typedef boost::filesystem::recursive_directory_iterator directory_iterator;

        directory_iterator end;

        size_t prefix = name.size ();

        if (name.size () > 0 && name [prefix - 1] != '\\' && name [prefix - 1] != '/')
            ++prefix;

        for (directory_iterator i (name); i != end; ++i)
        {
            if(boost::filesystem::is_directory (*i))
                continue;

            std::string proper = i->path ().string ();

            std::string searchable = normalize_path (proper.begin () + prefix, proper.end ());

            mIndex.insert (std::make_pair (std::move (searchable), std::move (proper)));
        }
    }

    bool isCaseSensitive() const { return fsstrict; }

  // The archive is loaded in the constructor, and never unloaded.
    void load() {}
    void unload() {}

    DataStreamPtr open(const String& filename, bool readonly = true) const
    {
        index::const_iterator i = lookup_filename (filename);

        if (i == mIndex.end ())
        {
            std::ostringstream os;
            os << "The file '" << filename << "' could not be found.";
            throw std::runtime_error (os.str ());
        }

        return openConstrainedFileDataStream (i->second.c_str ());
    }

    StringVectorPtr list(bool recursive = true, bool dirs = false)
    {
        StringVectorPtr ptr = StringVectorPtr(new StringVector());
        std::cout << "DirArchive<" << getName () << ">::list" << std::endl;
        return ptr;
    }

    FileInfoListPtr listFileInfo(bool recursive = true, bool dirs = false)
    {
        FileInfoListPtr ptr = FileInfoListPtr(new FileInfoList());
        std::cout << "DirArchive<" << getName () << ">::listFileInfo" << std::endl;
        return ptr;
    }

    StringVectorPtr find(const String& pattern, bool recursive = true,
                        bool dirs = false)
    {
        StringVectorPtr ptr = StringVectorPtr(new StringVector());

        if (pattern == "*")
            for (index::const_iterator i = mIndex.begin (); i != mIndex.end (); ++i)
                ptr->push_back (i->first);

        return ptr;
    }

    bool exists(const String& filename)
    {
        return lookup_filename (filename) != mIndex.end ();
    }

    time_t getModifiedTime(const String&) { return 0; }

    FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true,
                            bool dirs = false) const
    {
        FileInfoListPtr ptr = FileInfoListPtr(new FileInfoList());

        index::const_iterator i = lookup_filename (pattern);

        if (i != mIndex.end ())
        {
            FileInfo fi;

            std::size_t npos = i->first.rfind ('/');

            fi.archive = this;
            fi.filename = npos != -1 ? i->first.substr (npos) : i->first;
            fi.path = npos != -1 ? i->first.substr (0, npos) : "";
            fi.compressedSize = fi.uncompressedSize = 0;

            ptr->push_back(fi);
        }

        return ptr;
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
    BSAFile *narc = const_cast<BSAFile*>(&arc);

    // Open the file
    return narc->getFile(filename.c_str());
  }

  bool exists(const String& filename) {
    return arc.exists(filename.c_str());
  }

  bool cexists(const String& filename) const {
    return arc.exists(filename.c_str());
  }

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

    BSAFile::FileList const & files = arc.getList ();

    if (pattern == "*")
        for (BSAFile::FileList::const_iterator i = files.begin (); i != files.end (); ++i)
            ptr->push_back (i->name);

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
                               bool dirs = false) const
  {
      FileInfoListPtr ptr = FileInfoListPtr(new FileInfoList());

    // Check if the file exists (only works for single files - wild
    // cards and recursive search isn't implemented.)
    if(cexists(pattern))
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

  FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true,
                               bool dirs = false)
  {
    FileInfoListPtr ptr = FileInfoListPtr(new FileInfoList());

    // Check if the file exists (only works for single files - wild
    // cards and recursive search isn't implemented.)
    if(cexists(pattern))
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
      init2 = true;
    }
}

}

namespace Bsa
{

// The function below is the only publicly exposed part of this file

void addBSA(const std::string& name, const std::string& group)
{
  insertBSAFactory();
  ResourceGroupManager::getSingleton().
    addResourceLocation(name, "BSA", group, true);
}

void addDir(const std::string& name, const bool& fs, const std::string& group)
{
    fsstrict = fs;
    insertDirFactory();

    ResourceGroupManager::getSingleton().
    addResourceLocation(name, "Dir", group, true);
}

}
