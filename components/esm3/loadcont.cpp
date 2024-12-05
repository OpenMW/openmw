#include "loadcont.hpp"

#include "components/esm/defs.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void InventoryList::add(ESMReader& esm)
    {
        esm.getSubHeader();
        ContItem ci;
        esm.getT(ci.mCount);
        ci.mItem = esm.getMaybeFixedRefIdSize(32);
        mList.push_back(ci);
    }

    void InventoryList::save(ESMWriter& esm) const
    {
        for (std::vector<ContItem>::const_iterator it = mList.begin(); it != mList.end(); ++it)
        {
            esm.startSubRecord("NPCO");
            esm.writeT(it->mCount);
            esm.writeMaybeFixedSizeRefId(it->mItem, 32);
            esm.endRecord("NPCO");
        }
    }

    void Container::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mInventory.mList.clear();

        bool hasName = false;
        bool hasWeight = false;
        bool hasFlags = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getRefId();
                    hasName = true;
                    break;
                case fourCC("MODL"):
                    mModel = esm.getHString();
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("CNDT"):
                    esm.getHT(mWeight);
                    hasWeight = true;
                    break;
                case fourCC("FLAG"):
                    esm.getHT(mFlags);
                    if (mFlags & 0xf4)
                        esm.fail("Unknown flags");
                    if (!(mFlags & 0x8))
                        esm.fail("Flag 8 not set");
                    hasFlags = true;
                    break;
                case fourCC("SCRI"):
                    mScript = esm.getRefId();
                    break;
                case fourCC("NPCO"):
                    mInventory.add(esm);
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasWeight && !isDeleted)
            esm.fail("Missing CNDT subrecord");
        if (!hasFlags && !isDeleted)
            esm.fail("Missing FLAG subrecord");
    }

    void Container::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("CNDT", mWeight);
        esm.writeHNT("FLAG", mFlags);

        esm.writeHNOCRefId("SCRI", mScript);

        mInventory.save(esm);
    }

    void Container::blank()
    {
        mRecordFlags = 0;
        mName.clear();
        mModel.clear();
        mScript = ESM::RefId();
        mWeight = 0;
        mFlags = 0x8; // set default flag value
        mInventory.mList.clear();
    }
}
