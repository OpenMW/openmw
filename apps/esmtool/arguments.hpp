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
        bool quiet_given;
        bool loadcells_given;
        bool plain_given;

        std::string mode;
        std::string encoding;
        std::string filename;
        std::string outname;

        std::vector<std::string> types;
        std::string name;
    };
}

#endif
