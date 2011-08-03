#ifndef FILE_FINDER_MAIN_H
#define FILE_FINDER_MAIN_H

#include "search.hpp"
#include "filename_less.hpp"
#include <map>

namespace FileFinder
{

template <typename LESS>
class FileFinderT
{
  std::map<std::string, std::string, LESS> table;

  struct Inserter : ReturnPath
  {
    FileFinderT<LESS> *owner;
    int cut;

    void add(const boost::filesystem::path &pth)
    {
        std::string file = pth.string();
        std::string key = file.substr(cut);
        owner->table[key] = file;
    }
  };

  Inserter inserter;

public:
  FileFinderT(const boost::filesystem::path &path, bool recurse=true)
  {
    inserter.owner = this;

    // Remember the original path length, so we can cut it away from
    // the relative paths used as keys
    std::string pstring = path.string();
    inserter.cut = pstring.size();

    // If the path does not end in a slash, then boost will add one
    // later, which means one more character we have to remove.
    char last = pstring[pstring.size()-1];
    if(last != '\\' && last != '/')
      inserter.cut++;

    // Fill the map
    find(path, inserter, recurse);
  }

  bool has(const std::string& file) const
  {
        return table.find(file) != table.end();
  }

  // Find the full path from a relative path.
  const std::string &lookup(const std::string& file) const
  {
        return table.find(file)->second;
  }
};

// The default is to use path_less for equality checks
typedef FileFinderT<path_less> FileFinder;
typedef FileFinderT<path_slash> FileFinderStrict;
}
#endif
