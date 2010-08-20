#include "search.hpp"

#include <iostream>

using namespace std;
using namespace boost::filesystem;

void FileFinder::find(const path & dir_path, ReturnPath &ret, bool recurse)
{
  if ( !exists( dir_path ) )
    {
      cout << "Path " << dir_path << " not found\n";
      return;
    }

  directory_iterator end_itr; // default construction yields past-the-end
  for ( directory_iterator itr(dir_path);
        itr != end_itr;
        ++itr )
    {
      if ( is_directory( *itr ) )
        {
          if(recurse) find(*itr, ret);
        }
      else
        ret.add(*itr);
    }
}
