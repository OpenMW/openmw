#include "loadarmo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/concepts.hpp>

namespace ESM
{
    template <Misc::SameAsWithoutCvref<Armor::AODTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mType, v.mWeight, v.mValue, v.mHealth, v.mEnchant, v.mArmor);
    }

    void PartReferenceList::add(ESMReader& esm)
    {
        PartReference pr;
        esm.getHT(pr.mPart); // The INDX byte
        pr.mMale = esm.getHNORefId("BNAM");
        pr.mFemale = esm.getHNORefId("CNAM");
        mParts.push_back(pr);
    }

    void PartReferenceList::load(ESMReader& esm)
    {
        mParts.clear();
        while (esm.isNextSub("INDX"))
        {
            add(esm);
        }
    }

    void PartReferenceList::save(ESMWriter& esm) const
    {
        for (std::vector<PartReference>::const_iterator it = mParts.begin(); it != mParts.end(); ++it)
        {
            esm.writeHNT("INDX", it->mPart);
            esm.writeHNORefId("BNAM", it->mMale);
            esm.writeHNORefId("CNAM", it->mFemale);
        }
    }

    void Armor::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mParts.mParts.clear();

        bool hasName = false;
        bool hasData = false;
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
                case fourCC("AODT"):
                    esm.getSubComposite(mData);
                    hasData = true;
                    break;
                case fourCC("SCRI"):
                    mScript = esm.getRefId();
                    break;
                case fourCC("ITEX"):
                    mIcon = esm.getHString();
                    break;
                case fourCC("ENAM"):
                    mEnchant = esm.getRefId();
                    break;
                case fourCC("INDX"):
                    mParts.add(esm);
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
        if (!hasData && !isDeleted)
            esm.fail("Missing AODT subrecord");
    }

    void Armor::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNOCRefId("SCRI", mScript);
        esm.writeNamedComposite("AODT", mData);
        esm.writeHNOCString("ITEX", mIcon);
        mParts.save(esm);
        esm.writeHNOCRefId("ENAM", mEnchant);
    }

    void Armor::blank()
    {
        mRecordFlags = 0;
        mData.mType = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mHealth = 0;
        mData.mEnchant = 0;
        mData.mArmor = 0;
        mParts.mParts.clear();
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript = ESM::RefId();
        mEnchant = ESM::RefId();
    }
}
