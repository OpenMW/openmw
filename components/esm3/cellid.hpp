#ifndef OPENMW_ESM_CELLID_H
#define OPENMW_ESM_CELLID_H

#include <components/esm/refid.hpp>
#include <cstdint>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct CellId
    {
        struct CellIndex
        {
            int32_t mX;
            int32_t mY;
        };

        std::string mWorldspace;
        CellIndex mIndex;
        bool mPaged;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;

        // TODO tetramir: this probably shouldn't exist, needs it because some CellIds are saved on disk
        static CellId extractFromRefId(const ESM::RefId& id);
    };

    bool operator==(const CellId& left, const CellId& right);
    bool operator!=(const CellId& left, const CellId& right);
    bool operator<(const CellId& left, const CellId& right);
}

#endif
