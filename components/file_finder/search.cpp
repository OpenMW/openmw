#include "search.hpp"

#include <iostream>

void FileFinder::find(const boost::filesystem::path & dir_path, ReturnPath &ret, bool recurse)
{
    if (boost::filesystem::exists(dir_path))
    {
        if (!recurse)
        {
            boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
            for (boost::filesystem::directory_iterator itr(dir_path); itr != end_itr; ++itr)
            {
                if (!boost::filesystem::is_directory( *itr ))
                {
                    ret.add(*itr);
                }
            }
        }
        else
        {
            boost::filesystem::recursive_directory_iterator end_itr; // default construction yields past-the-end
            for (boost::filesystem::recursive_directory_iterator itr(dir_path); itr != end_itr; ++itr)
            {
                if (!boost::filesystem::is_directory(*itr))
                {
                    ret.add(*itr);
                }
            }
        }
    }
    else
    {
        std::cout << "Path " << dir_path << " not found" << std::endl;
    }
}
