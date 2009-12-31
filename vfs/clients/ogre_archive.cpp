#include "ogre_archive.h"

#include "../../stream/clients/ogre_datastream.h"

using namespace Mangle::VFS;
using namespace Mangle::Stream;

Ogre::DataStreamPtr MangleArchive::open(const Ogre::String& filename) const
{
  return Ogre::DataStreamPtr(new MangleDataStream
                             (filename, vfs->open(filename), true));
}

static void fill(Ogre::FileInfoList &out, FileInfoList &in)
{
  int size = in.size();
  out.resize(size);

  for(int i=0; i<size; i++)
    {
      out[i].filename = in[i].name;
      out[i].basename = in[i].basename;
      out[i].path = ""; // FIXME
      out[i].uncompressedSize = in[i].size;
      out[i].compressedSize = in[i].size;
    }
}

static void fill(Ogre::StringVector &out, FileInfoList &in)
{
  int size = in.size();
  out.resize(size);

  for(int i=0; i<size; i++)
    out[i] = in[i].name;
}

Ogre::StringVectorPtr MangleArchive::list(bool recursive, bool dirs)
{
  assert(vfs->hasList);
  FileInfoListPtr lst = vfs->list("", recursive, dirs);
  Ogre::StringVector *res = new Ogre::StringVector;

  fill(*res, *lst);

  return Ogre::StringVectorPtr(res);
}

Ogre::FileInfoListPtr MangleArchive::listFileInfo(bool recursive, bool dirs)
{
  assert(vfs->hasList);
  FileInfoListPtr lst = vfs->list("", recursive, dirs);
  Ogre::FileInfoList *res = new Ogre::FileInfoList;

  fill(*res, *lst);

  return Ogre::FileInfoListPtr(res);
}

// Find functions will only work if vfs->hasFind is set.
Ogre::StringVectorPtr MangleArchive::find(const Ogre::String& pattern,
                                bool recursive,
                                bool dirs)
{
  assert(vfs->hasFind);
  FileInfoListPtr lst = vfs->find(pattern, recursive, dirs);
  Ogre::StringVector *res = new Ogre::StringVector;

  fill(*res, *lst);

  return Ogre::StringVectorPtr(res);
}

Ogre::FileInfoListPtr MangleArchive::findFileInfo(const Ogre::String& pattern, 
                                        bool recursive,
                                        bool dirs)
{
  assert(vfs->hasFind);
  FileInfoListPtr lst = vfs->find(pattern, recursive, dirs);
  Ogre::FileInfoList *res = new Ogre::FileInfoList;

  fill(*res, *lst);

  return Ogre::FileInfoListPtr(res);
}
