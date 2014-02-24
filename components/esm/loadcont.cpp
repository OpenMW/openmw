#include "loadcont.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{

void InventoryList::load(ESMReader &esm)
{
    ContItem ci;
    while (esm.isNextSub("NPCO"))
    {
        esm.getHT(ci, 36);
        mList.push_back(ci);
    }
}

void InventoryList::save(ESMWriter &esm) const
{
    for (std::vector<ContItem>::const_iterator it = mList.begin(); it != mList.end(); ++it)
    {
        esm.writeHNT("NPCO", *it, 36);
    }
}

    unsigned int Container::sRecordId = REC_CONT;

void Container::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mWeight, "CNDT", 4);
    esm.getHNT(mFlags, "FLAG", 4);

    if (mFlags & 0xf4)
        esm.fail("Unknown flags");
    if (!(mFlags & 0x8))
        esm.fail("Flag 8 not set");

    mScript = esm.getHNOString("SCRI");

    mInventory.load(esm);
}

void Container::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("CNDT", mWeight, 4);
    esm.writeHNT("FLAG", mFlags, 4);

    esm.writeHNOCString("SCRI", mScript);

    mInventory.save(esm);
}

    void Container::blank()
    {
        mName.clear();
        mModel.clear();
        mScript.clear();
        mWeight = 0;
        mFlags = 0;
        mInventory.mList.clear();
    }
}
