#ifndef OPENMW_COMPONENTS_FILES_OPENFILE_H
#define OPENMW_COMPONENTS_FILES_OPENFILE_H

#include <iosfwd>
#include <memory>
#include <string>

namespace Files
{
    std::unique_ptr<std::ifstream> openBinaryInputFileStream(const std::string& path);
}

#endif
