#include "fileops.hpp"
#include <boost/filesystem.hpp>

namespace Files
{

bool isFile(const char *name)
{
    return boost::filesystem::exists(boost::filesystem::path(name));
}

}
