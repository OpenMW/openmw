#include "loadnpc.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/concepts.hpp>

namespace ESM
{
    namespace
    {
        struct NPDTstruct12
        {
            NPC::NPDTstruct52& mStruct;
        };
    }

    template <Misc::SameAsWithoutCvref<NPC::NPDTstruct52> T>
    void decompose(T&& v, const auto& f)
    {
        char padding1 = 0;
        char padding2 = 0;
        f(v.mLevel, v.mAttributes, v.mSkills, padding1, v.mHealth, v.mMana, v.mFatigue, v.mDisposition, v.mReputation,
            v.mRank, padding2, v.mGold);
    }

    template <Misc::SameAsWithoutCvref<NPDTstruct12> T>
    void decompose(T&& v, const auto& f)
    {
        char padding[] = { 0, 0, 0 };
        f(v.mStruct.mLevel, v.mStruct.mDisposition, v.mStruct.mReputation, v.mStruct.mRank, padding, v.mStruct.mGold);
    }

    void NPC::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mSpells.mList.clear();
        mInventory.mList.clear();
        mTransport.mList.clear();
        mAiPackage.mList.clear();
        mAiData.blank();
        mAiData.mHello = mAiData.mFight = mAiData.mFlee = 30;

        bool hasName = false;
        bool hasNpdt = false;
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
                case fourCC("RNAM"):
                    mRace = esm.getRefId();
                    break;
                case fourCC("CNAM"):
                    mClass = esm.getRefId();
                    break;
                case fourCC("ANAM"):
                    mFaction = esm.getRefId();
                    break;
                case fourCC("BNAM"):
                    mHead = esm.getRefId();
                    break;
                case fourCC("KNAM"):
                    mHair = esm.getRefId();
                    break;
                case fourCC("SCRI"):
                    mScript = esm.getRefId();
                    break;
                case fourCC("NPDT"):
                    hasNpdt = true;
                    esm.getSubHeader();
                    if (esm.getSubSize() == getCompositeSize(mNpdt))
                    {
                        mNpdtType = NPC_DEFAULT;
                        esm.getComposite(mNpdt);
                    }
                    else
                    {
                        NPDTstruct12 data{ mNpdt };
                        if (esm.getSubSize() == getCompositeSize(data))
                        {
                            mNpdtType = NPC_WITH_AUTOCALCULATED_STATS;

                            // Clearing the mNdpt struct to initialize all values
                            blankNpdt();
                            esm.getComposite(data);
                        }
                        else
                            esm.fail("NPC_NPDT must be 12 or 52 bytes long");
                    }
                    break;
                case fourCC("FLAG"):
                    hasFlags = true;
                    int32_t flags;
                    esm.getHT(flags);
                    mFlags = flags & 0xFF;
                    mBloodType = ((flags >> 8) & 0xFF) >> 2;
                    break;
                case fourCC("NPCS"):
                    mSpells.add(esm);
                    break;
                case fourCC("NPCO"):
                    mInventory.add(esm);
                    break;
                case fourCC("AIDT"):
                    esm.getSubComposite(mAiData);
                    break;
                case fourCC("DODT"):
                case fourCC("DNAM"):
                    mTransport.add(esm);
                    break;
                case AI_Wander:
                case AI_Activate:
                case AI_Escort:
                case AI_Follow:
                case AI_Travel:
                case AI_CNDT:
                    mAiPackage.add(esm);
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
        if (!hasNpdt && !isDeleted)
            esm.fail("Missing NPDT subrecord");
        if (!hasFlags && !isDeleted)
            esm.fail("Missing FLAG subrecord");
    }
    void NPC::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNOCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNCRefId("RNAM", mRace);
        esm.writeHNCRefId("CNAM", mClass);
        esm.writeHNCRefId("ANAM", mFaction);
        esm.writeHNCRefId("BNAM", mHead);
        esm.writeHNCRefId("KNAM", mHair);
        esm.writeHNOCRefId("SCRI", mScript);

        if (mNpdtType == NPC_DEFAULT)
        {
            esm.writeNamedComposite("NPDT", mNpdt);
        }
        else if (mNpdtType == NPC_WITH_AUTOCALCULATED_STATS)
        {
            esm.writeNamedComposite("NPDT", NPDTstruct12{ const_cast<NPDTstruct52&>(mNpdt) });
        }

        esm.writeHNT("FLAG", ((mBloodType << 10) + mFlags));

        mInventory.save(esm);
        mSpells.save(esm);
        esm.writeNamedComposite("AIDT", mAiData);

        mTransport.save(esm);

        mAiPackage.save(esm);
    }

    bool NPC::isMale() const
    {
        return (mFlags & Female) == 0;
    }

    void NPC::setIsMale(bool value)
    {
        mFlags |= Female;
        if (value)
        {
            mFlags ^= Female;
        }
    }

    void NPC::blank()
    {
        mRecordFlags = 0;
        mNpdtType = NPC_DEFAULT;
        blankNpdt();
        mBloodType = 0;
        mFlags = 0;
        mInventory.mList.clear();
        mSpells.mList.clear();
        mAiData.blank();
        mAiData.mHello = mAiData.mFight = mAiData.mFlee = 30;
        mTransport.mList.clear();
        mAiPackage.mList.clear();
        mName.clear();
        mModel.clear();
        mRace = ESM::RefId();
        mClass = ESM::RefId();
        mFaction = ESM::RefId();
        mScript = ESM::RefId();
        mHair = ESM::RefId();
        mHead = ESM::RefId();
    }

    void NPC::blankNpdt()
    {
        mNpdt.mLevel = 0;
        mNpdt.mAttributes.fill(0);
        mNpdt.mSkills.fill(0);
        mNpdt.mReputation = 0;
        mNpdt.mHealth = mNpdt.mMana = mNpdt.mFatigue = 0;
        mNpdt.mDisposition = 0;
        mNpdt.mRank = 0;
        mNpdt.mGold = 0;
    }

    int NPC::getFactionRank() const
    {
        if (mFaction.empty())
            return -1;
        else
            return mNpdt.mRank;
    }

    const std::vector<Transport::Dest>& NPC::getTransport() const
    {
        return mTransport.mList;
    }
}
