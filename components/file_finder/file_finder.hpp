#ifndef FILE_FINDER_MAIN_H
#define FILE_FINDER_MAIN_H

#include <map>

#include "search.hpp"
#include "filename_less.hpp"
#include <components/files/multidircollection.hpp>

namespace FileFinder
{

template <typename LESS>
class FileFinderT
{
  typedef std::map<std::string, std::string, LESS> TableContainer;
  TableContainer table;

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
    const std::string& pstring = path.string();
    inserter.cut = pstring.size();

    // If the path does not end in a slash, then boost will add one
    // later, which means one more character we have to remove.
    char last = *pstring.rbegin();
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
    static std::string empty;
    typename TableContainer::const_iterator it = table.find(file);
    return (it != table.end()) ? it->second : empty;
  }
};

template
<
    class LESS
>
struct TreeFileFinder
{
    typedef TreeFileFinder<LESS> finder_t;

    TreeFileFinder(const Files::PathContainer& paths, bool recurse = true)
    {
        struct : ReturnPath
        {
            finder_t *owner;
            int cut;

            void add(const boost::filesystem::path &pth)
            {
                std::string file = pth.string();
                std::string key = file.substr(cut);
                owner->mTable[key] = file;
            }
        } inserter;

        inserter.owner = this;

        for (Files::PathContainer::const_iterator it = paths.begin(); it != paths.end(); ++it)
        {

            // Remember the original path length, so we can cut it away from
            // the relative paths used as keys
            const std::string& pstring = it->string();
            inserter.cut = pstring.size();

            // If the path does not end in a slash, then boost will add one
            // later, which means one more character we have to remove.
            char last = *pstring.rbegin();
            if (last != '\\' && last != '/')
            {
              inserter.cut++;
            }

            // Fill the map
            find(*it, inserter, recurse);
        }
    }

    bool has(const std::string& file) const
    {
        return mTable.find(file) != mTable.end();
    }

    const std::string& lookup(const std::string& file) const
    {
        static std::string empty;
        typename TableContainer::const_iterator it = mTable.find(file);
        return (it != mTable.end()) ? it->second : empty;
    }

    private:
        typedef std::map<std::string, std::string, LESS> TableContainer;
        TableContainer mTable;

//        Inserter inserter;
};


// The default is to use path_less for equality checks
typedef FileFinderT<path_less> FileFinder;
typedef FileFinderT<path_slash> FileFinderStrict;

typedef TreeFileFinder<path_less> LessTreeFileFinder;
typedef TreeFileFinder<path_slash> StrictTreeFileFinder;

} /* namespace FileFinder */
#endif /* FILE_FINDER_MAIN_H */
