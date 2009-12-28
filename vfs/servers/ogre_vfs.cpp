#include "ogre_vfs.h"
#include "../../stream/servers/ogre_datastream.h"

using namespace Mangle::VFS;

OgreVFS::OgreVFS(const std::string &_group)
  : group(_group)
{
  hasList = true;
  hasFind = true;
  isCaseSensitive = true;

  // Get the group manager once
  gm = Ogre::ResourceGroupManager::getSingletonPtr();

  // Use the default group if none was specified
  if(group.empty())
    group = gm->getWorldResourceGroupName();
}

Mangle::Stream::Stream *OgreVFS::open(const std::string &name)
{
  Ogre::DataStreamPtr data = gm->openResource(name, group);
  return new Stream::OgreStream(data);
}

static void fill(FileInfoList &out, Ogre::FileInfoList &in, bool dirs)
{
  int size = in.size();
  out.resize(size);

  for(int i=0; i<size; i++)
    {
      out[i].name = in[i].filename;
      out[i].basename = in[i].basename;
      out[i].size = in[i].uncompressedSize;
      out[i].isDir = dirs;
      out[i].time = 0;
    }
}

FileInfoList OgreVFS::list(const std::string& dir,
                            bool recurse,
                            bool dirs) const
{
  Ogre::FileInfoListPtr olist = gm->listResourceFileInfo(group, dirs);
  FileInfoList res;
  fill(res, *olist, dirs);
  return res;
}

FileInfoList OgreVFS::find(const std::string& pattern,
                            bool recursive,
                            bool dirs) const
{
  Ogre::FileInfoListPtr olist = gm->findResourceFileInfo(group, pattern, dirs);
  FileInfoList res;
  fill(res, *olist, dirs);
  return res;
}
