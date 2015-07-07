#include "loadstat.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"
#include "util.hpp"

namespace ESM
{
    unsigned int Static::sRecordId = REC_STAT;

    void Static::load(ESMReader &esm)
    {
        mId = esm.getHNString("NAME");
        if (mIsDeleted = readDeleSubRecord(esm))
        {
            return;
        }

        mModel = esm.getHNString("MODL");
    }
    void Static::save(ESMWriter &esm) const
    {
        esm.writeHNCString("NAME", mId);
        if (mIsDeleted)
        {
            writeDeleSubRecord(esm);
            return;
        }

        esm.writeHNCString("MODL", mModel);
    }

    void Static::blank()
    {
        mModel.clear();
        mIsDeleted = false;
    }
}
