#ifndef OPENMW_ESMTOOL_ARGUMENTS_H
#define OPENMW_ESMTOOL_ARGUMENTS_H

#include <vector>
#include <optional>

#include <components/esm/format.hpp>

namespace EsmTool
{
    struct Arguments
    {
        std::optional<ESM::Format> mRawFormat;
        bool quiet_given = false;
        bool loadcells_given = false;
        bool plain_given = false;

        std::string mode;
        std::string encoding;
        std::string filename;
        std::string outname;

        std::vector<std::string> types;
        std::string name;
    };
}

#endif
