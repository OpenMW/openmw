#ifndef OPENMW_COMPONENTS_FILES_OPENFILE_H
#define OPENMW_COMPONENTS_FILES_OPENFILE_H

#include <iosfwd>
#include <memory>
#include <string>

#include <boost/filesystem/fstream.hpp>

namespace Files
{
    std::unique_ptr<boost::filesystem::ifstream> openBinaryInputFileStream(const std::string& path);
}

#endif
