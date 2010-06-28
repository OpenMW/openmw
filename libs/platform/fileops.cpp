#include "fileops.hpp"
#include <boost/filesystem.hpp>

namespace OMW { namespace Platform {

    bool isFile(const char *name)
    {
      boost::filesystem::path cfg_file_path(name);
      return boost::filesystem::exists(cfg_file_path);
    }
}}
