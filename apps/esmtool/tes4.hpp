#ifndef OPENMW_ESMTOOL_TES4_H
#define OPENMW_ESMTOOL_TES4_H

#include <iosfwd>
#include <memory>

#include <boost/filesystem/fstream.hpp>

namespace EsmTool
{
    struct Arguments;

    int loadTes4(const Arguments& info, std::unique_ptr<boost::filesystem::ifstream>&& stream);
}

#endif
