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

struct PathPatternMatcher
{
    PathPatternMatcher (char const * pattern) : pattern (pattern)
    {
    }

    bool operator () (char const * input)
    {
        char const * p = pattern;
        char const * i = input;

        while (*p && *i)
        {
            if (*p == '*')
            {
                ++p;

                while (*i && *i != *p && *i != '/' && *i != '\\')
                    ++i;
            }
            else
            if (*p == '?')
            {
                if (*i == '/' || *i == '\\')
                    break;

                ++i, ++p;
            }
            if (*p == '/' || *p == '\\')
            {
                if (*i != '/' && *i != '\\')
                    break;

                ++i, ++p;
            }
            else
            {
                if (*i != *p)
                    break;

                ++i, ++p;
            }
        }

        return *p == 0 && *i == 0;
    }

private:
    char const * pattern;
};

struct FileNameGatherer
{
    StringVectorPtr ptr;

    FileNameGatherer (StringVectorPtr ptr) : ptr (ptr)
    {
    }

    void operator () (std::string const & filename) const
    {
        ptr->push_back (filename);
    }
};

struct FileInfoGatherer
{
    Archive const * archive;
    FileInfoListPtr ptr;

    FileInfoGatherer (Archive const * archive, FileInfoListPtr ptr) :
        archive (archive), ptr (ptr)
    {
    }

    void operator () (std::string filename) const
    {
        FileInfo fi;

        std::size_t pt = filename.rfind ('/');
        if (pt == std::string::npos)
            pt = 0;

        fi.archive = const_cast <Archive *> (archive);
        fi.path = filename.substr (0, pt);
        fi.filename = filename.substr (pt);
        fi.compressedSize = fi.uncompressedSize = 0;

        ptr->push_back(fi);
    }
};

template <typename file_iterator, typename filename_extractor, typename match_handler>
void matchFiles (bool recursive, std::string const & pattern, file_iterator begin, file_iterator end, filename_extractor filenameExtractor, match_handler matchHandler)
{
    if (recursive && pattern == "*")
    {
        for (file_iterator  i = begin; i != end; ++i)
            matchHandler (filenameExtractor (*i));
    }
    else
    {
        PathPatternMatcher matcher (pattern.c_str ());

        if (recursive)
        {
            for (file_iterator  i = begin; i != end; ++i)
            {
                char const * filename = filenameExtractor (*i);
                char const * matchable_part = filename;

                for (char const * j = matchable_part; *j; ++j)
                {
                    if (*j == '/' || *j == '\\')
                        matchable_part = j + 1;
                }

                if (matcher (matchable_part))
                    matchHandler (filename);
            }
        }
        else
        {
            for (file_iterator  i = begin; i != end; ++i)
            {
                char const * filename = filenameExtractor (*i);

                if (matcher (filename))
                    matchHandler (filename);
            }
        }
    }
}



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

    static char const * extractFilename (index::value_type const & entry)
    {
        return entry.first.c_str ();
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
        return find ("*", recursive, dirs);
    }

    FileInfoListPtr listFileInfo(bool recursive = true, bool dirs = false)
    {
        return findFileInfo ("*", recursive, dirs);
    }

    StringVectorPtr find(const String& pattern, bool recursive = true,
                        bool dirs = false)
    {
        std::string normalizedPattern = normalize_path (pattern.begin (), pattern.end ());
        StringVectorPtr ptr = StringVectorPtr(new StringVector());
        matchFiles (recursive, normalizedPattern, mIndex.begin (), mIndex.end (), extractFilename, FileNameGatherer (ptr));
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
        FileInfoGatherer gatherer (this, ptr);

        std::string normalizedPattern = normalize_path (pattern.begin (), pattern.end ());

        index::const_iterator i = mIndex.find (normalizedPattern);

        if (i != mIndex.end ())
        {
            gatherer (i->first);
        }
        else
        {

            matchFiles (recursive, normalizedPattern, mIndex.begin (), mIndex.end (), extractFilename, gatherer);
        }

        return ptr;
    }
};

class BSAArchive : public Archive
{
  BSAFile arc;

  static char const * extractFilename (BSAFile::FileStruct const & entry)
  {
      return entry.name;
  }

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

  time_t getModifiedTime(const String&) { return 0; }

  // This is never called as far as I can see.
  StringVectorPtr list(bool recursive = true, bool dirs = false)
  {
    return find ("*", recursive, dirs);
  }

  // Also never called.
  FileInfoListPtr listFileInfo(bool recursive = true, bool dirs = false)
  {
    return findFileInfo ("*", recursive, dirs);
  }

  // After load() is called, find("*") is called once. It doesn't seem
  // to matter that we return an empty list, exists() gets called on
  // the correct files anyway.
  StringVectorPtr find(const String& pattern, bool recursive = true,
                       bool dirs = false)
  {
        StringVectorPtr ptr = StringVectorPtr(new StringVector());
        matchFiles (recursive, pattern, arc.getList ().begin (), arc.getList ().end (), extractFilename, FileNameGatherer (ptr));
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
    matchFiles (recursive, pattern, arc.getList ().begin (), arc.getList ().end (), extractFilename, FileInfoGatherer (this, ptr));
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
