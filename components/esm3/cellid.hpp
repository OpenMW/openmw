#ifndef OPENMW_ESM_CELLID_H
#define OPENMW_ESM_CELLID_H

#include <components/esm/refid.hpp>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct CellId
    {
        struct CellIndex
        {
            int mX;
            int mY;
        };

        std::string mWorldspace;
        CellIndex mIndex;
        bool mPaged;

        static const std::string sDefaultWorldspace;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
        ESM::RefId getCellRefId() const;

        // TODO tetramir: this probably shouldn't exist, needs it because some CellIds are saved on disk
        static CellId extractFromRefId(const ESM::RefId& id);
    };

    bool operator==(const CellId& left, const CellId& right);
    bool operator!=(const CellId& left, const CellId& right);
    bool operator<(const CellId& left, const CellId& right);
}

#endif
