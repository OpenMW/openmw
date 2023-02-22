#ifndef OPENMW_ESM_CUSTOMMARKERSTATE_H
#define OPENMW_ESM_CUSTOMMARKERSTATE_H

#include <components/esm/refid.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only
    struct CustomMarker
    {
        float mWorldX;
        float mWorldY;

        RefId mCell;

        std::string mNote;

        bool operator==(const CustomMarker& other) const
        {
            return mNote == other.mNote && mCell == other.mCell && mWorldX == other.mWorldX && mWorldY == other.mWorldY;
        }

        void load(ESMReader& reader);
        void save(ESMWriter& writer) const;
    };

}

#endif
