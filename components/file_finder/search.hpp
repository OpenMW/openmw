#ifndef FILE_FINDER_SEARCH_H
#define FILE_FINDER_SEARCH_H

#include <boost/filesystem.hpp>
#include <string>

namespace FileFinder
{
  struct ReturnPath
  {
    virtual void add(const boost::filesystem::path &pth) = 0;
  };

  /** Search the given path and return all file paths through 'ret'. If
      recurse==true, all files in subdirectories are returned as well.
  */
  void find(const boost::filesystem::path & dir_path, ReturnPath &ret, bool recurse=true);
}

#endif
