#ifndef OPENMW_ESSIMPORT_CONVERTER_H
#define OPENMW_ESSIMPORT_CONVERTER_H

#include <OgreImage.h>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

#include <components/esm/loadcell.hpp>
#include <components/esm/loadbook.hpp>
#include <components/esm/loadclas.hpp>
#include <components/esm/loadglob.hpp>
#include <components/esm/cellstate.hpp>
#include <components/esm/loadfact.hpp>
#include <components/esm/dialoguestate.hpp>
#include <components/esm/custommarkerstate.hpp>
#include <components/esm/loadcrea.hpp>
#include <components/esm/weatherstate.hpp>
#include <components/esm/globalscript.hpp>
#include <components/esm/queststate.hpp>
#include <components/esm/stolenitems.hpp>

#include "importcrec.hpp"
#include "importcntc.hpp"

#include "importercontext.hpp"
#include "importcellref.hpp"
#include "importklst.hpp"
#include "importgame.hpp"
#include "importinfo.hpp"
#include "importdial.hpp"
#include "importques.hpp"
#include "importjour.hpp"
#include "importscpt.hpp"

#include "convertacdt.hpp"
#include "convertnpcc.hpp"
#include "convertscpt.hpp"
#include "convertplayer.hpp"

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
        ESM::NPC npc;
        std::string id = esm.getHNString("NAME");
        npc.load(esm);
        if (id != "player")
        {
            // Handles changes to the NPC struct, but since there is no index here
            // it will apply to ALL instances of the class. seems to be the reason for the
            // "feature" in MW where changing AI settings of one guard will change it for all guards of that refID.
            mContext->mNpcs[Misc::StringUtils::lowerCase(id)] = npc;
        }
        else
        {
            mContext->mPlayer.mObject.mCreatureStats.mLevel = npc.mNpdt52.mLevel;
            mContext->mPlayerBase = npc;
            std::map<const int, float> empty;
            // FIXME: player start spells and birthsign spells aren't listed here,
            // need to fix openmw to account for this
            for (std::vector<std::string>::const_iterator it = npc.mSpells.mList.begin(); it != npc.mSpells.mList.end(); ++it)
                mContext->mPlayer.mObject.mCreatureStats.mSpells.mSpells[*it] = empty;

            // Clear the list now that we've written it, this prevents issues cropping up with
            // ensureCustomData() in OpenMW tripping over no longer existing spells, where an error would be fatal.
            mContext->mPlayerBase.mSpells.mList.clear();

            // Same with inventory. Actually it's strange this would contain something, since there's already an
            // inventory list in NPCC. There seems to be a fair amount of redundancy in this format.
            mContext->mPlayerBase.mInventory.mList.clear();
        }
    }
};

class ConvertCREA : public Converter
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        // See comment in ConvertNPC
        ESM::Creature creature;
        std::string id = esm.getHNString("NAME");
        creature.load(esm);
        mContext->mCreatures[Misc::StringUtils::lowerCase(id)] = creature;
    }
};

// Do we need ConvertCONT?
// I've seen a CONT record in a certain save file, but the container contents in it
// were identical to a corresponding CNTC record. See previous comment about redundancy...

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
            convertNPCC(npcc, mContext->mPlayer.mObject);
        }
        else
        {
            int index = npcc.mNPDT.mIndex;
            mContext->mNpcChanges.insert(std::make_pair(std::make_pair(index,id), npcc));
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
        convertACDT(refr.mActorData.mACDT, cStats);

        ESM::NpcStats& npcStats = mContext->mPlayer.mObject.mNpcStats;
        convertNpcData(refr.mActorData, npcStats);

        mSelectedSpell = refr.mActorData.mSelectedSpell;
        if (!refr.mActorData.mSelectedEnchantItem.empty())
        {
            ESM::InventoryState& invState = mContext->mPlayer.mObject.mInventory;

            for (unsigned int i=0; i<invState.mItems.size(); ++i)
            {
                // FIXME: in case of conflict (multiple items with this refID) use the already equipped one?
                if (Misc::StringUtils::ciEqual(invState.mItems[i].mRef.mRefID, refr.mActorData.mSelectedEnchantItem))
                    invState.mSelectedEnchantItem = i;
            }
        }
    }
    virtual void write(ESM::ESMWriter& esm)
    {
        esm.startRecord(ESM::REC_ASPL);
        esm.writeHNString("ID__", mSelectedSpell);
        esm.endRecord(ESM::REC_ASPL);
    }
private:
    std::string mSelectedSpell;
};

class ConvertPCDT : public Converter
{
public:
    ConvertPCDT() : mFirstPersonCam(true) {}

    virtual void read(ESM::ESMReader &esm)
    {
        PCDT pcdt;
        pcdt.load(esm);

        convertPCDT(pcdt, mContext->mPlayer, mContext->mDialogueState.mKnownTopics, mFirstPersonCam);
    }
    virtual void write(ESM::ESMWriter &esm)
    {
        esm.startRecord(ESM::REC_CAM_);
        esm.writeHNT("FIRS", mFirstPersonCam);
        esm.endRecord(ESM::REC_CAM_);
    }
private:
    bool mFirstPersonCam;
};

class ConvertCNTC : public Converter
{
    virtual void read(ESM::ESMReader &esm)
    {
        std::string id = esm.getHNString("NAME");
        CNTC cntc;
        cntc.load(esm);
        mContext->mContainerChanges.insert(std::make_pair(std::make_pair(cntc.mIndex,id), cntc));
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
        mContext->mCreatureChanges.insert(std::make_pair(std::make_pair(crec.mIndex,id), crec));
    }
};

class ConvertFMAP : public Converter
{
public:
    virtual void read(ESM::ESMReader &esm);
    virtual void write(ESM::ESMWriter &esm);

private:
    Ogre::Image mGlobalMapImage;
};

class ConvertCell : public Converter
{
public:
    virtual void read(ESM::ESMReader& esm);
    virtual void write(ESM::ESMWriter& esm);

private:
    struct Cell
    {
        ESM::Cell mCell;
        std::vector<CellRef> mRefs;
        std::vector<unsigned int> mFogOfWar;
    };

    std::map<std::string, Cell> mIntCells;
    std::map<std::pair<int, int>, Cell> mExtCells;

    std::vector<ESM::CustomMarker> mMarkers;

    void writeCell(const Cell& cell, ESM::ESMWriter &esm);
};

class ConvertKLST : public Converter
{
public:
    virtual void read(ESM::ESMReader& esm)
    {
        KLST klst;
        klst.load(esm);
        mKillCounter = klst.mKillCounter;

        mContext->mPlayer.mObject.mNpcStats.mWerewolfKills = klst.mWerewolfKills;
    }

    virtual void write(ESM::ESMWriter &esm)
    {
        esm.startRecord(ESM::REC_DCOU);
        for (std::map<std::string, int>::const_iterator it = mKillCounter.begin(); it != mKillCounter.end(); ++it)
        {
            esm.writeHNString("ID__", it->first);
            esm.writeHNT ("COUN", it->second);
        }
        esm.endRecord(ESM::REC_DCOU);
    }

private:
    std::map<std::string, int> mKillCounter;
};

class ConvertFACT : public Converter
{
public:
    virtual void read(ESM::ESMReader& esm)
    {
        std::string id = esm.getHNString("NAME");
        ESM::Faction faction;
        faction.load(esm);

        Misc::StringUtils::toLower(id);
        for (std::map<std::string, int>::const_iterator it = faction.mReactions.begin(); it != faction.mReactions.end(); ++it)
        {
            std::string faction2 = Misc::StringUtils::lowerCase(it->first);
            mContext->mDialogueState.mChangedFactionReaction[id].insert(std::make_pair(faction2, it->second));
        }
    }
};

/// Stolen items
class ConvertSTLN : public Converter
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        std::string itemid = esm.getHNString("NAME");
        Misc::StringUtils::toLower(itemid);

        while (esm.isNextSub("FNAM") || esm.isNextSub("ONAM"))
        {
            if (esm.retSubName().toString() == "FNAM")
            {
                std::string factionid = esm.getHString();
                mStolenItems[itemid].insert(std::make_pair(Misc::StringUtils::lowerCase(factionid), true));
            }
            else
            {
                std::string ownerid = esm.getHString();
                mStolenItems[itemid].insert(std::make_pair(Misc::StringUtils::lowerCase(ownerid), false));
            }
        }
    }
    virtual void write(ESM::ESMWriter &esm)
    {
        ESM::StolenItems items;
        for (std::map<std::string, std::set<Owner> >::const_iterator it = mStolenItems.begin(); it != mStolenItems.end(); ++it)
        {
            std::map<std::pair<std::string, bool>, int> owners;
            for (std::set<Owner>::const_iterator ownerIt = it->second.begin(); ownerIt != it->second.end(); ++ownerIt)
            {
                owners.insert(std::make_pair(std::make_pair(ownerIt->first, ownerIt->second)
                                             // Since OpenMW doesn't suffer from the owner contamination bug,
                                             // it needs a count argument. But for legacy savegames, we don't know
                                             // this count, so must assume all items of that ID are stolen,
                                             // like vanilla MW did.
                                             ,std::numeric_limits<int>::max()));
            }

            items.mStolenItems.insert(std::make_pair(it->first, owners));
        }

        esm.startRecord(ESM::REC_STLN);
        items.write(esm);
        esm.endRecord(ESM::REC_STLN);
    }

private:
    typedef std::pair<std::string, bool> Owner; // <owner id, bool isFaction>

    std::map<std::string, std::set<Owner> > mStolenItems;
};

/// Seen responses for a dialogue topic?
/// Each DIAL record is followed by a number of INFO records, I believe, just like in ESMs
/// Dialogue conversion problems:
/// - Journal is stored in one continuous HTML markup rather than each entry separately with associated info ID.
/// - Seen dialogue responses only store the INFO id, rather than the fulltext.
/// - Quest stages only store the INFO id, rather than the journal entry fulltext.
class ConvertINFO : public Converter
{
public:
    virtual void read(ESM::ESMReader& esm)
    {
        INFO info;
        info.load(esm);
    }
};

class ConvertDIAL : public Converter
{
public:
    virtual void read(ESM::ESMReader& esm)
    {
        std::string id = esm.getHNString("NAME");
        DIAL dial;
        dial.load(esm);
        if (dial.mIndex > 0)
            mDials[id] = dial;
    }
    virtual void write(ESM::ESMWriter &esm)
    {
        for (std::map<std::string, DIAL>::const_iterator it = mDials.begin(); it != mDials.end(); ++it)
        {
            esm.startRecord(ESM::REC_QUES);
            ESM::QuestState state;
            state.mFinished = 0;
            state.mState = it->second.mIndex;
            state.mTopic = Misc::StringUtils::lowerCase(it->first);
            state.save(esm);
            esm.endRecord(ESM::REC_QUES);
        }
    }
private:
    std::map<std::string, DIAL> mDials;
};

class ConvertQUES : public Converter
{
public:
    virtual void read(ESM::ESMReader& esm)
    {
        std::string id = esm.getHNString("NAME");
        QUES quest;
        quest.load(esm);
    }
};

class ConvertJOUR : public Converter
{
public:
    virtual void read(ESM::ESMReader& esm)
    {
        JOUR journal;
        journal.load(esm);
    }
};

class ConvertGAME : public Converter
{
public:
    ConvertGAME() : mHasGame(false) {}

    std::string toString(int weatherId)
    {
        switch (weatherId)
        {
        case 0:
            return "clear";
        case 1:
            return "cloudy";
        case 2:
            return "foggy";
        case 3:
            return "overcast";
        case 4:
            return "rain";
        case 5:
            return "thunderstorm";
        case 6:
            return "ashstorm";
        case 7:
            return "blight";
        case 8:
            return "snow";
        case 9:
            return "blizzard";
        case -1:
            return "";
        default:
            {
                std::stringstream error;
                error << "unknown weather id: " << weatherId;
                throw std::runtime_error(error.str());
            }
        }
    }

    virtual void read(ESM::ESMReader &esm)
    {
        mGame.load(esm);
        mHasGame = true;
    }

    virtual void write(ESM::ESMWriter &esm)
    {
        if (!mHasGame)
            return;
        esm.startRecord(ESM::REC_WTHR);
        ESM::WeatherState weather;
        weather.mCurrentWeather = toString(mGame.mGMDT.mCurrentWeather);
        weather.mNextWeather = toString(mGame.mGMDT.mNextWeather);
        weather.mRemainingTransitionTime = mGame.mGMDT.mWeatherTransition/100.f*(0.015f*24*3600);
        weather.mHour = mContext->mHour;
        weather.mWindSpeed = 0.f;
        weather.mTimePassed = 0.0;
        weather.mFirstUpdate = false;
        weather.save(esm);
        esm.endRecord(ESM::REC_WTHR);
    }

private:
    bool mHasGame;
    GAME mGame;
};

/// Running global script
class ConvertSCPT : public Converter
{
public:
    virtual void read(ESM::ESMReader &esm)
    {
        SCPT script;
        script.load(esm);
        ESM::GlobalScript out;
        convertSCPT(script, out);
        mScripts.push_back(out);
    }
    virtual void write(ESM::ESMWriter &esm)
    {
        for (std::vector<ESM::GlobalScript>::const_iterator it = mScripts.begin(); it != mScripts.end(); ++it)
        {
            esm.startRecord(ESM::REC_GSCR);
            it->save(esm);
            esm.endRecord(ESM::REC_GSCR);
        }
    }
private:
    std::vector<ESM::GlobalScript> mScripts;
};

}

#endif
