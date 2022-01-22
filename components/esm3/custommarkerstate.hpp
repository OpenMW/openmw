#ifndef OPENMW_ESM_CUSTOMMARKERSTATE_H
#define OPENMW_ESM_CUSTOMMARKERSTATE_H

#include "cellid.hpp"

namespace ESM
{

// format 0, saved games only
struct CustomMarker
{
    float mWorldX;
    float mWorldY;

    ESM::CellId mCell;

    std::string mNote;

    bool operator == (const CustomMarker& other) const
    {
        return mNote == other.mNote && mCell == other.mCell && mWorldX == other.mWorldX && mWorldY == other.mWorldY;
    }

    void load (ESM::ESMReader& reader);
    void save (ESM::ESMWriter& writer) const;
};

}

#endif
