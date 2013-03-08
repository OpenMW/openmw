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

using namespace Ogre;

static bool fsstrict = false;

static char strict_normalize_char(char ch)
{
    return ch == '\\' ? '/' : ch;
}

static char nonstrict_normalize_char(char ch)
{
    return ch == '\\' ? '/' : std::tolower(ch);
}

template<typename T1, typename T2>
static std::string normalize_path(T1 begin, T2 end)
{
    std::string normalized;
    normalized.reserve(std::distance(begin, end));
    char (*normalize_char)(char) = fsstrict ? &strict_normalize_char : &nonstrict_normalize_char;
    std::transform(begin, end, std::back_inserter(normalized), normalize_char);
    return normalized;
}

/// An OGRE Archive wrapping a BSAFile archive
class DirArchive: public Ogre::Archive
{
    typedef std::map <std::string, std::string> index;

    index mIndex;

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

            mIndex.insert (std::make_pair (searchable, proper));
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
        std::string normalizedPattern = normalize_path(pattern.begin(), pattern.end());
        StringVectorPtr ptr = StringVectorPtr(new StringVector());
        for(index::const_iterator iter = mIndex.begin();iter != mIndex.end();iter++)
        {
            if(Ogre::StringUtil::match(iter->first, normalizedPattern) ||
               (recursive && Ogre::StringUtil::match(iter->first, "*/"+normalizedPattern)))
                ptr->push_back(iter->first);
        }
        return ptr;
    }

    bool exists(const String& filename)
    {
        return lookup_filename(filename) != mIndex.end ();
    }

    time_t getModifiedTime(const String&) { return 0; }

    FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true,
                            bool dirs = false) const
    {
        std::string normalizedPattern = normalize_path(pattern.begin(), pattern.end());
        FileInfoListPtr ptr = FileInfoListPtr(new FileInfoList());

        index::const_iterator i = mIndex.find(normalizedPattern);
        if(i != mIndex.end())
        {
            std::string::size_type pt = i->first.rfind('/');
            if(pt == std::string::npos)
                pt = 0;

            FileInfo fi;
            fi.archive = const_cast<DirArchive*>(this);
            fi.path = i->first.substr(0, pt);
            fi.filename = i->first.substr((i->first[pt]=='/') ? pt+1 : pt);
            fi.compressedSize = fi.uncompressedSize = 0;

            ptr->push_back(fi);
        }
        else
        {
            for(index::const_iterator iter = mIndex.begin();iter != mIndex.end();iter++)
            {
                if(Ogre::StringUtil::match(iter->first, normalizedPattern) ||
                   (recursive && Ogre::StringUtil::match(iter->first, "*/"+normalizedPattern)))
                {
                    std::string::size_type pt = iter->first.rfind('/');
                    if(pt == std::string::npos)
                        pt = 0;

                    FileInfo fi;
                    fi.archive = const_cast<DirArchive*>(this);
                    fi.path = iter->first.substr(0, pt);
                    fi.filename = iter->first.substr((iter->first[pt]=='/') ? pt+1 : pt);
                    fi.compressedSize = fi.uncompressedSize = 0;

                    ptr->push_back(fi);
                }
            }
        }

        return ptr;
    }
};

class BSAArchive : public Archive
{
  Bsa::BSAFile arc;

  static const char *extractFilename(const Bsa::BSAFile::FileStruct &entry)
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

  DataStreamPtr open(const String& filename, bool readonly = true) const
  {
    // Get a non-const reference to arc. This is a hack and it's all
    // OGRE's fault. You should NOT expect an open() command not to
    // have any side effects on the archive, and hence this function
    // should not have been declared const in the first place.
    Bsa::BSAFile *narc = const_cast<Bsa::BSAFile*>(&arc);

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

    StringVectorPtr find(const String& pattern, bool recursive = true,
                         bool dirs = false)
    {
        std::string normalizedPattern = normalize_path(pattern.begin(), pattern.end());
        const Bsa::BSAFile::FileList &filelist = arc.getList();
        StringVectorPtr ptr = StringVectorPtr(new StringVector());
        for(Bsa::BSAFile::FileList::const_iterator iter = filelist.begin();iter != filelist.end();iter++)
        {
            std::string ent = normalize_path(iter->name, iter->name+std::strlen(iter->name));
            if(Ogre::StringUtil::match(ent, normalizedPattern) ||
               (recursive && Ogre::StringUtil::match(ent, "*/"+normalizedPattern)))
                ptr->push_back(iter->name);
        }
        return ptr;
    }

    FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true,
                                bool dirs = false) const
    {
        std::string normalizedPattern = normalize_path(pattern.begin(), pattern.end());
        FileInfoListPtr ptr = FileInfoListPtr(new FileInfoList());
        const Bsa::BSAFile::FileList &filelist = arc.getList();

        for(Bsa::BSAFile::FileList::const_iterator iter = filelist.begin();iter != filelist.end();iter++)
        {
            std::string ent = normalize_path(iter->name, iter->name+std::strlen(iter->name));
            if(Ogre::StringUtil::match(ent, normalizedPattern) ||
                (recursive && Ogre::StringUtil::match(ent, "*/"+normalizedPattern)))
            {
                std::string::size_type pt = ent.rfind('/');
                if(pt == std::string::npos)
                    pt = 0;

                FileInfo fi;
                fi.archive = const_cast<BSAArchive*>(this);
                fi.path = std::string(iter->name, pt);
                fi.filename = std::string(iter->name + ((ent[pt]=='/') ? pt+1 : pt));
                fi.compressedSize = fi.uncompressedSize = iter->fileSize;

                ptr->push_back(fi);
            }
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

  virtual Archive* createInstance(const String& name, bool readOnly)
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
