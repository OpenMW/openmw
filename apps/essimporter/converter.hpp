#ifndef OPENMW_ESSIMPORT_CONVERTER_H
#define OPENMW_ESSIMPORT_CONVERTER_H

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

#include <components/esm/loadcell.hpp>
#include <components/esm/loadbook.hpp>
#include <components/esm/loadclas.hpp>
#include <components/esm/loadglob.hpp>
#include "importcrec.hpp"

#include "importercontext.hpp"
#include "importcellref.hpp"

namespace ESSImport
{

class Converter
{
public:
    /// @return the order for writing this converter's records to the output file, in relation to other converters
    virtual int getStage() { return 1; }

    virtual ~Converter() {}

    void setContext(Context& context) { mContext = &context; }

    virtual void read(ESM::ESMReader& esm)
    {
    }

    /// Called after the input file has been read in completely, which may be necessary
    /// if the conversion process relies on information in other records
    virtual void write(ESM::ESMWriter& esm)
    {

    }

protected:
    Context* mContext;
};

/// Default converter: simply reads the record and writes it unmodified to the output
template <typename T>
class DefaultConverter : public Converter
{
public:
    virtual int getStage() { return 0; }

    virtual void read(ESM::ESMReader& esm)
    {
        std::string id = esm.getHNString("NAME");
        T record;
        record.load(esm);
        mRecords[id] = record;
    }

    virtual void write(ESM::ESMWriter& esm)
    {
        for (typename std::map<std::string, T>::const_iterator it = mRecords.begin(); it != mRecords.end(); ++it)
        {
            esm.startRecord(T::sRecordId);
            esm.writeHNString("NAME", it->first);
            it->second.save(esm);
            esm.endRecord(T::sRecordId);
        }
    }

protected:
    std::map<std::string, T> mRecords;
};

class ConvertNPC : public Converter
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        // this is always the player
        ESM::NPC npc;
        std::string id = esm.getHNString("NAME");
        assert (id == "player");
        npc.load(esm);
        mContext->mPlayer.mObject.mCreatureStats.mLevel = npc.mNpdt52.mLevel;
        mContext->mPlayerBase = npc;
        std::map<const int, float> empty;
        for (std::vector<std::string>::const_iterator it = npc.mSpells.mList.begin(); it != npc.mSpells.mList.end(); ++it)
            mContext->mPlayer.mObject.mCreatureStats.mSpells.mSpells[*it] = empty;
    }
};

class ConvertGlobal : public DefaultConverter<ESM::Global>
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        std::string id = esm.getHNString("NAME");
        ESM::Global global;
        global.load(esm);
        if (Misc::StringUtils::ciEqual(id, "gamehour"))
            mContext->mHour = global.mValue.getFloat();
        if (Misc::StringUtils::ciEqual(id, "day"))
            mContext->mDay = global.mValue.getInteger();
        if (Misc::StringUtils::ciEqual(id, "month"))
            mContext->mMonth = global.mValue.getInteger();
        if (Misc::StringUtils::ciEqual(id, "year"))
            mContext->mYear = global.mValue.getInteger();
        mRecords[id] = global;
    }
};

class ConvertClass : public DefaultConverter<ESM::Class>
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        std::string id = esm.getHNString("NAME");
        ESM::Class class_;
        class_.load(esm);

        if (id == "NEWCLASSID_CHARGEN")
            mContext->mCustomPlayerClassName = class_.mName;

        mRecords[id] = class_;
    }
};

class ConvertBook : public DefaultConverter<ESM::Book>
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        std::string id = esm.getHNString("NAME");
        ESM::Book book;
        book.load(esm);
        if (book.mData.mSkillID == -1)
            mContext->mPlayer.mObject.mNpcStats.mUsedIds.push_back(Misc::StringUtils::lowerCase(id));

        mRecords[id] = book;
    }
};

class ConvertNPCC : public Converter
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        std::string id = esm.getHNString("NAME");
        NPCC npcc;
        npcc.load(esm);
        if (id == "PlayerSaveGame")
        {
            mContext->mPlayer.mObject.mNpcStats.mReputation = npcc.mNPDT.mReputation;
        }
    }
};

class ConvertREFR : public Converter
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        REFR refr;
        refr.load(esm);
        assert(refr.mRefID == "PlayerSaveGame");
        mContext->mPlayer.mObject.mPosition = refr.mPos;

        ESM::CreatureStats& cStats = mContext->mPlayer.mObject.mCreatureStats;
        for (int i=0; i<3; ++i)
        {
            int writeIndex = translateDynamicIndex(i);
            cStats.mDynamic[writeIndex].mBase = refr.mACDT.mDynamic[i][1];
            cStats.mDynamic[writeIndex].mMod = refr.mACDT.mDynamic[i][1];
            cStats.mDynamic[writeIndex].mCurrent = refr.mACDT.mDynamic[i][0];
        }
        for (int i=0; i<8; ++i)
        {
            cStats.mAttributes[i].mBase = refr.mACDT.mAttributes[i][1];
            cStats.mAttributes[i].mMod = refr.mACDT.mAttributes[i][0];
            cStats.mAttributes[i].mCurrent = refr.mACDT.mAttributes[i][0];
        }
        ESM::NpcStats& npcStats = mContext->mPlayer.mObject.mNpcStats;
        for (int i=0; i<ESM::Skill::Length; ++i)
        {
            npcStats.mSkills[i].mRegular.mMod = refr.mSkills[i][1];
            npcStats.mSkills[i].mRegular.mCurrent = refr.mSkills[i][1];
            npcStats.mSkills[i].mRegular.mBase = refr.mSkills[i][0];
        }
    }

    // OpenMW uses Health,Magicka,Fatigue, MW uses Health,Fatigue,Magicka
    int translateDynamicIndex(int mwIndex)
    {
        if (mwIndex == 1)
            return 2;
        else if (mwIndex == 2)
            return 1;
        return mwIndex;
    }
};

class ConvertPCDT : public Converter
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        PCDT pcdt;
        pcdt.load(esm);

        mContext->mPlayer.mBirthsign = pcdt.mBirthsign;
        mContext->mPlayer.mObject.mNpcStats.mBounty = pcdt.mBounty;
        for (std::vector<PCDT::FNAM>::const_iterator it = pcdt.mFactions.begin(); it != pcdt.mFactions.end(); ++it)
        {
            ESM::NpcStats::Faction faction;
            faction.mExpelled = it->mFlags & 0x2;
            faction.mRank = it->mRank;
            faction.mReputation = it->mReputation;
            mContext->mPlayer.mObject.mNpcStats.mFactions[it->mFactionName.toString()] = faction;
        }

    }
};

class ConvertCREC : public Converter
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        std::string id = esm.getHNString("NAME");
        CREC crec;
        crec.load(esm);
    }
};

class ConvertFMAP : public Converter
{
public:
    virtual void read(ESM::ESMReader &esm);
};

class ConvertCell : public Converter
{
public:
    virtual void read(ESM::ESMReader& esm)
    {
        ESM::Cell cell;
        std::string id = esm.getHNString("NAME");
        cell.load(esm, false);
        CellRef ref;
        while (esm.hasMoreSubs())
        {
            ref.load (esm);
            if (esm.isNextSub("DELE"))
                std::cout << "deleted ref " << ref.mIndexedRefId << std::endl;
        }
    }

};

}

#endif
