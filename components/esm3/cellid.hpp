#ifndef OPENMW_ESM_CELLID_H
#define OPENMW_ESM_CELLID_H

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

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

    bool operator== (const CellId& left, const CellId& right);
    bool operator!= (const CellId& left, const CellId& right);
    bool operator< (const CellId& left, const CellId& right);
}

#endif
