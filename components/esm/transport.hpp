#ifndef OPENMW_COMPONENTS_ESM_TRANSPORT_H
#define OPENMW_COMPONENTS_ESM_TRANSPORT_H

#include <string>
#include <vector>

#include "defs.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /// List of travel service destination. Shared by CREA and NPC_ records.
    struct Transport
    {

        struct Dest
        {
            Position    mPos;
            std::string mCellName;
        };

        std::vector<Dest> mList;

        /// Load one destination, assumes the subrecord name was already read
        void add(ESMReader &esm);

        void save(ESMWriter &esm) const;

    };

}

#endif
