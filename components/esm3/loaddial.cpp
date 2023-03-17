#include "loaddial.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void Dialogue::load(ESMReader& esm, bool& isDeleted)
    {
        loadId(esm);
        loadData(esm, isDeleted);
    }

    void Dialogue::loadId(ESMReader& esm)
    {
        mId = esm.getHNRefId("NAME");
    }

    void Dialogue::loadData(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;

        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("DATA"):
                {
                    esm.getSubHeader();
                    int size = esm.getSubSize();
                    if (size == 1)
                    {
                        esm.getT(mType);
                    }
                    else
                    {
                        esm.skip(size);
                    }
                    break;
                }
                case SREC_DELE:
                    esm.skipHSub();
                    mType = Unknown;
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }
    }

    void Dialogue::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);
        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
        }
        else
        {
            esm.writeHNT("DATA", mType);
        }
    }

    void Dialogue::blank()
    {
        mInfo.clear();
    }

    void Dialogue::readInfo(ESMReader& esm)
    {
        DialInfo info;
        bool isDeleted = false;
        info.load(esm, isDeleted);
        mInfoOrder.insertInfo(std::move(info), isDeleted);
    }

    void Dialogue::setUp()
    {
        mInfoOrder.removeDeleted();
        mInfoOrder.extractOrderedInfo(mInfo);
    }
}
