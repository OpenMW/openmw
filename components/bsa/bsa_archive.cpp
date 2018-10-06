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

#include <map>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <OgreFileSystem.h>
#include <OgreArchive.h>
#include <OgreArchiveFactory.h>
#include <OgreArchiveManager.h>
#include <OgreResourceGroupManager.h>
/*
 * This test for ogre version is not realy correct, because the change happened since
 * commit d5e05e9d97f47bce40aa41a2bf31c2b6c3fde5f3 (2014-02-24) on the default branch
 * rather than during an ogre version change event. However it should be good enough.
 */
#if OGRE_VERSION < 0x010a00
#define OGRE_CONST const
#else
#define OGRE_CONST
#endif

#include <extern/BSAOpt/hash.hpp>
#include "bsa_file.hpp"
#include "tes4bsa_file.hpp"

#include "../files/constrainedfiledatastream.hpp"

namespace
{
// Concepts from answer by Remy Lebeau
// https://stackoverflow.com/questions/15068475/recursive-hard-disk-search-with-findfirstfile-findnextfile-c
//
// Also see https://msdn.microsoft.com/en-us/library/aa365200%28VS.85%29.aspx
//
// From 34.5 sec down to 18.5 sec on laptop with many of the data files on an external USB drive
#if defined _WIN32 || defined _WIN64

#include <windows.h>
#include <memory> // auto_ptr

    // FIXME: not tested unicode path and filenames
    DWORD indexFiles(const std::string& rootDir, const std::string& subdir,
                     std::map<std::uint64_t, std::string>& files, std::map<std::string, std::string>& index)
    {
        HANDLE hFind = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA ffd;
        std::string path = rootDir + ((subdir == "") ? "" : "\\" +subdir);

        hFind = FindFirstFile((path + "\\*").c_str(), &ffd);
        if (INVALID_HANDLE_VALUE == hFind)
            return ERROR_INVALID_HANDLE;

        std::auto_ptr<std::vector<std::string> > subDirs;
        std::string filename;

        do
        {
            filename = std::string(ffd.cFileName);

            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (filename != "." && filename != "..")
                {
                    if (subDirs.get() == nullptr)
                        subDirs.reset(new std::vector<std::string>);

                    subDirs->push_back(filename);
                }
            }
            else
            {
                std::uint64_t folderHash = GenOBHash(subdir, filename);

                std::map<std::uint64_t, std::string>::iterator iter = files.find(folderHash);
                if (iter != files.end())
                    throw std::runtime_error ("duplicate hash found");

                files[folderHash] = path + "\\" + filename;

                std::string entry = ((subdir == "") ? "" : subdir + "\\") + filename;
                std::replace(entry.begin(), entry.end(), '\\', '/');
                index.insert(std::make_pair (entry, path + "/" + filename));
            }
        } while (FindNextFile(hFind, &ffd) != 0);

        FindClose(hFind);

        DWORD dwError = GetLastError();
        if (dwError != ERROR_NO_MORE_FILES)
            return dwError;

        if (subDirs.get() != nullptr)
        {
            for (size_t i = 0; i < subDirs->size(); ++i)
            {
                std::string dir = subDirs->at(i);
                boost::algorithm::to_lower(dir);
                // FIXME: ignoring errors for now
                dwError = indexFiles(rootDir, ((subdir == "") ? "" : subdir + "\\") + dir, files, index);
            }
        }

        return 0;
    }
#endif
}

using namespace Ogre;

static bool fsstrict = false;

static const std::ctype<char>& facet = std::use_facet<std::ctype<char> >(std::locale::classic());

static char strict_normalize_char(char ch)
{
    return ch == '\\' ? '/' : ch;
}

static char nonstrict_normalize_char(char ch)
{
    return ch == '\\' ? '/' : facet.tolower(ch);
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

    std::map <std::uint64_t, std::string> mFiles;

    index::const_iterator lookup_filename (std::string const & filename) const
    {
        std::string normalized = normalize_path (filename.begin (), filename.end ());
        return mIndex.find (normalized);
    }

public:

    DirArchive(const String& name)
        : Archive(name, "Dir")
    {
#if defined _WIN32 || defined _WIN64
        indexFiles(name, "", mFiles, mIndex);
#else

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
#endif
    }

    bool isCaseSensitive() const { return fsstrict; }

  // The archive is loaded in the constructor, and never unloaded.
    void load() {}
    void unload() {}

    virtual DataStreamPtr open(const String& filename, bool readonly = true) OGRE_CONST
    {
#if defined _WIN32 || defined _WIN64
        boost::filesystem::path p(filename);
        std::string file = p.filename().string();

        p.remove_filename();
        std::string dir = p.string();
        boost::algorithm::to_lower(dir);
        std::replace(dir.begin(), dir.end(), '/', '\\');

        std::uint64_t hash = GenOBHash(dir, file);

        std::map<std::uint64_t, std::string>::const_iterator it = mFiles.find(hash);
        if (it == mFiles.end())
        {
            std::ostringstream os;
            os << "The file '" << filename << "' could not be found.";
            throw std::runtime_error (os.str());
        }

        return openConstrainedFileDataStream (it->second.c_str());
#else
        index::const_iterator i = lookup_filename (filename);

        if (i == mIndex.end ())
        {
            std::ostringstream os;
            os << "The file '" << filename << "' could not be found.";
            throw std::runtime_error (os.str ());
        }

        return openConstrainedFileDataStream (i->second.c_str ());
#endif
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
        for(index::const_iterator iter = mIndex.begin();iter != mIndex.end();++iter)
        {
            if(Ogre::StringUtil::match(iter->first, normalizedPattern) ||
               (recursive && Ogre::StringUtil::match(iter->first, "*/"+normalizedPattern)))
                ptr->push_back(iter->first);
        }
        return ptr;
    }

    bool exists(const String& filename)
    {
#if defined _WIN32 || defined _WIN64
        boost::filesystem::path p(filename);
        std::string file = p.filename().string();

        p.remove_filename();
        std::string dir = p.string();
        boost::algorithm::to_lower(dir);
        std::replace(dir.begin(), dir.end(), '/', '\\');

        std::uint64_t hash = GenOBHash(dir, file);

        std::map<std::uint64_t, std::string>::const_iterator it = mFiles.find(hash);
        if (it == mFiles.end())
            return false;

        return true;
#else
        return lookup_filename(filename) != mIndex.end ();
#endif
    }

    time_t getModifiedTime(const String&) { return 0; }

    virtual FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true,
                            bool dirs = false) OGRE_CONST
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
            for(index::const_iterator iter = mIndex.begin();iter != mIndex.end();++iter)
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

  BSAArchive(const String& name, const std::string& type) : Archive(name, type) {}

  bool isCaseSensitive() const { return false; }

  // The archive is loaded in the constructor, and never unloaded.
  void load() {}
  void unload() {}

  virtual DataStreamPtr open(const String& filename, bool readonly = true) OGRE_CONST
  {
    // Get a non-const reference to arc. This is a hack and it's all
    // OGRE's fault. You should NOT expect an open() command not to
    // have any side effects on the archive, and hence this function
    // should not have been declared const in the first place.
    Bsa::BSAFile *narc = const_cast<Bsa::BSAFile*>(&arc);

    // Open the file
    return narc->getFile(filename.c_str());
  }

  virtual bool exists(const String& filename) {
    return arc.exists(filename.c_str());
  }

  time_t getModifiedTime(const String&) { return 0; }

  // This is never called as far as I can see. (actually called from CSMWorld::Resources ctor)
  StringVectorPtr list(bool recursive = true, bool dirs = false)
  {
    return find ("*", recursive, dirs);
  }

  // Also never called.
  FileInfoListPtr listFileInfo(bool recursive = true, bool dirs = false)
  {
    return findFileInfo ("*", recursive, dirs);
  }

    virtual StringVectorPtr find(const String& pattern, bool recursive = true,
                         bool dirs = false)
    {
        std::string normalizedPattern = normalize_path(pattern.begin(), pattern.end());
        const Bsa::BSAFile::FileList &filelist = arc.getList();
        StringVectorPtr ptr = StringVectorPtr(new StringVector());
        for(Bsa::BSAFile::FileList::const_iterator iter = filelist.begin();iter != filelist.end();++iter)
        {
            std::string ent = normalize_path(iter->name, iter->name+std::strlen(iter->name));
            if(Ogre::StringUtil::match(ent, normalizedPattern) ||
               (recursive && Ogre::StringUtil::match(ent, "*/"+normalizedPattern)))
                ptr->push_back(iter->name);
        }
        return ptr;
    }

    virtual FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true,
                                bool dirs = false) OGRE_CONST
    {
        std::string normalizedPattern = normalize_path(pattern.begin(), pattern.end());
        FileInfoListPtr ptr = FileInfoListPtr(new FileInfoList());
        const Bsa::BSAFile::FileList &filelist = arc.getList();

        for(Bsa::BSAFile::FileList::const_iterator iter = filelist.begin();iter != filelist.end();++iter)
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

class TES4BSAArchive : public BSAArchive
{
  Bsa::TES4BSAFile arc;

public:
  TES4BSAArchive::TES4BSAArchive(const String& name) : BSAArchive(name, "TES4BSA") { arc.open(name); }

  virtual DataStreamPtr open(const String& filename, bool readonly = true)
  {
    return arc.getFile(filename);
  }

  virtual bool exists(const String& filename)
  {
    return arc.exists(filename);
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

class DirArchiveFactory : public ArchiveFactory
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

    virtual Archive* createInstance(const String& name, bool readOnly)
    {
      return new DirArchive(name);
    }

    void destroyInstance( Archive* arch) { delete arch; }
};

class TES4BSAArchiveFactory : public ArchiveFactory
{
public:
  const String& getType() const
  {
    static String name = "TES4BSA";
    return name;
  }

  Archive *createInstance( const String& name )
  {
    return new TES4BSAArchive(name);
  }

  virtual Archive* createInstance(const String& name, bool readOnly)
  {
    return new TES4BSAArchive(name);
  }

  void destroyInstance( Archive* arch) { delete arch; }
};


static bool init = false;
static bool init2 = false;
static bool init3 = false;

static void insertBSAFactory()
{
  if(!init)
    {
      ArchiveManager::getSingleton().addArchiveFactory( new BSAArchiveFactory );
      init = true;
    }
}

static void insertTES4BSAFactory()
{
  if(!init3)
    {
      ArchiveManager::getSingleton().addArchiveFactory( new TES4BSAArchiveFactory );
      init3 = true;
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

void addTES4BSA(const std::string& name, const std::string& group)
{
  insertTES4BSAFactory();
  ResourceGroupManager::getSingleton().
    addResourceLocation(name, "TES4BSA", group, true);
}

void addDir(const std::string& name, const bool& fs, const std::string& group)
{
    fsstrict = fs;
    insertDirFactory();

    ResourceGroupManager::getSingleton().
    addResourceLocation(name, "Dir", group, true);
}

}
