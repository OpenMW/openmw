#include "loadstat.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Static::sRecordId = REC_STAT;

    void Static::load(ESMReader &esm)
    {
        mPersistent = (esm.getRecordFlags() & 0x0400) != 0;

        mModel = esm.getHNString("MODL");
    }
    void Static::save(ESMWriter &esm) const
    {
        esm.writeHNCString("MODL", mModel);
    }

    void Static::blank()
    {
        mModel.clear();
    }
}
