#include "loaddial.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <stdexcept>

namespace ESM
{

    void Dialogue::load(ESMReader& esm, bool& isDeleted)
    {
        loadId(esm);
        loadData(esm, isDeleted);
    }

    void Dialogue::loadId(ESMReader& esm)
    {
        if (esm.getFormatVersion() <= MaxStringRefIdFormatVersion)
        {
            mStringId = esm.getHNString("NAME");
            mId = ESM::RefId::stringRefId(mStringId);
            return;
        }

        if (esm.getFormatVersion() <= MaxNameIsRefIdOnlyFormatVersion)
        {
            mId = esm.getHNRefId("NAME");
            return;
        }

        mId = esm.getHNRefId("ID__");
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
                        mType = Unknown;
                    }
                    break;
                }
                case SREC_DELE:
                    esm.skipHSub();
                    mType = Unknown;
                    isDeleted = true;
                    break;
                case SREC_NAME:
                    mStringId = esm.getHString();
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!isDeleted && MaxStringRefIdFormatVersion < esm.getFormatVersion()
            && esm.getFormatVersion() <= MaxNameIsRefIdOnlyFormatVersion)
            mStringId = mId.toString();
    }

    void Dialogue::save(ESMWriter& esm, bool isDeleted) const
    {
        if (esm.getFormatVersion() <= MaxStringRefIdFormatVersion)
        {
            if (mId != mStringId)
                throw std::runtime_error("Trying to save Dialogue record with name \"" + mStringId
                    + "\" not maching id " + mId.toDebugString());
            esm.writeHNCString("NAME", mStringId);
        }
        else if (esm.getFormatVersion() <= MaxNameIsRefIdOnlyFormatVersion)
            esm.writeHNRefId("NAME", mId);
        else
            esm.writeHNRefId("ID__", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
        }
        else
        {
            if (esm.getFormatVersion() > MaxNameIsRefIdOnlyFormatVersion)
                esm.writeHNString("NAME", mStringId);
            esm.writeHNT("DATA", mType);
        }
    }

    void Dialogue::blank()
    {
        mType = Unknown;
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
