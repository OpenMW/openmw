#ifndef OPENMW_ESSIMPORT_CONVERTER_H
#define OPENMW_ESSIMPORT_CONVERTER_H

#include <limits>

#include <osg/Image>
#include <osg/ref_ptr>

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
#include <components/esm/projectilestate.hpp>

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
#include "importproj.h"
#include "importsplm.h"

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

    /// @note The load method of ESM records accept the deleted flag as a parameter.
    /// I don't know can the DELE sub-record appear in saved games, so the deleted flag will be ignored.
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
    int getStage() override { return 0; }

    void read(ESM::ESMReader& esm) override
    {
        T record;
        bool isDeleted = false;

        record.load(esm, isDeleted);
        mRecords[record.mId] = record;
    }

    void write(ESM::ESMWriter& esm) override
    {
        for (typename std::map<std::string, T>::const_iterator it = mRecords.begin(); it != mRecords.end(); ++it)
        {
            esm.startRecord(T::sRecordId);
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
    void read(ESM::ESMReader &esm) override
    {
        ESM::NPC npc;
        bool isDeleted = false;

        npc.load(esm, isDeleted);
        if (npc.mId != "player")
        {
            // Handles changes to the NPC struct, but since there is no index here
            // it will apply to ALL instances of the class. seems to be the reason for the
            // "feature" in MW where changing AI settings of one guard will change it for all guards of that refID.
            mContext->mNpcs[Misc::StringUtils::lowerCase(npc.mId)] = npc;
        }
        else
        {
            mContext->mPlayer.mObject.mCreatureStats.mLevel = npc.mNpdt.mLevel;
            mContext->mPlayerBase = npc;
            ESM::SpellState::SpellParams empty;
            // FIXME: player start spells and birthsign spells aren't listed here,
            // need to fix openmw to account for this
            for (const auto & spell : npc.mSpells.mList)
                mContext->mPlayer.mObject.mCreatureStats.mSpells.mSpells[spell] = empty;

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
    void read(ESM::ESMReader &esm) override
    {
        // See comment in ConvertNPC
        ESM::Creature creature;
        bool isDeleted = false;

        creature.load(esm, isDeleted);
        mContext->mCreatures[Misc::StringUtils::lowerCase(creature.mId)] = creature;
    }
};

// Do we need ConvertCONT?
// I've seen a CONT record in a certain save file, but the container contents in it
// were identical to a corresponding CNTC record. See previous comment about redundancy...

class ConvertGlobal : public DefaultConverter<ESM::Global>
{
public:
    void read(ESM::ESMReader &esm) override
    {
        ESM::Global global;
        bool isDeleted = false;

        global.load(esm, isDeleted);
        if (Misc::StringUtils::ciEqual(global.mId, "gamehour"))
            mContext->mHour = global.mValue.getFloat();
        if (Misc::StringUtils::ciEqual(global.mId, "day"))
            mContext->mDay = global.mValue.getInteger();
        if (Misc::StringUtils::ciEqual(global.mId, "month"))
            mContext->mMonth = global.mValue.getInteger();
        if (Misc::StringUtils::ciEqual(global.mId, "year"))
            mContext->mYear = global.mValue.getInteger();
        mRecords[global.mId] = global;
    }
};

class ConvertClass : public DefaultConverter<ESM::Class>
{
public:
    void read(ESM::ESMReader &esm) override
    {
        ESM::Class class_;
        bool isDeleted = false;

        class_.load(esm, isDeleted);
        if (class_.mId == "NEWCLASSID_CHARGEN")
            mContext->mCustomPlayerClassName = class_.mName;

        mRecords[class_.mId] = class_;
    }
};

class ConvertBook : public DefaultConverter<ESM::Book>
{
public:
    void read(ESM::ESMReader &esm) override
    {
        ESM::Book book;
        bool isDeleted = false;

        book.load(esm, isDeleted);
        if (book.mData.mSkillId == -1)
            mContext->mPlayer.mObject.mNpcStats.mUsedIds.push_back(Misc::StringUtils::lowerCase(book.mId));

        mRecords[book.mId] = book;
    }
};

class ConvertNPCC : public Converter
{
public:
    void read(ESM::ESMReader &esm) override
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
    void read(ESM::ESMReader &esm) override
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
    void write(ESM::ESMWriter& esm) override
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
    ConvertPCDT()
        : mFirstPersonCam(true),
          mTeleportingEnabled(true),
          mLevitationEnabled(true)
    {}

    void read(ESM::ESMReader &esm) override
    {
        PCDT pcdt;
        pcdt.load(esm);

        convertPCDT(pcdt, mContext->mPlayer, mContext->mDialogueState.mKnownTopics, mFirstPersonCam, mTeleportingEnabled, mLevitationEnabled, mContext->mControlsState);
    }
    void write(ESM::ESMWriter &esm) override
    {
        esm.startRecord(ESM::REC_ENAB);
        esm.writeHNT("TELE", mTeleportingEnabled);
        esm.writeHNT("LEVT", mLevitationEnabled);
        esm.endRecord(ESM::REC_ENAB);

        esm.startRecord(ESM::REC_CAM_);
        esm.writeHNT("FIRS", mFirstPersonCam);
        esm.endRecord(ESM::REC_CAM_);
    }
private:
    bool mFirstPersonCam;
    bool mTeleportingEnabled;
    bool mLevitationEnabled;
};

class ConvertCNTC : public Converter
{
    void read(ESM::ESMReader &esm) override
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
    void read(ESM::ESMReader &esm) override
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
    void read(ESM::ESMReader &esm) override;
    void write(ESM::ESMWriter &esm) override;

private:
    osg::ref_ptr<osg::Image> mGlobalMapImage;
};

class ConvertCell : public Converter
{
public:
    void read(ESM::ESMReader& esm) override;
    void write(ESM::ESMWriter& esm) override;

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
    void read(ESM::ESMReader& esm) override
    {
        KLST klst;
        klst.load(esm);
        mKillCounter = klst.mKillCounter;

        mContext->mPlayer.mObject.mNpcStats.mWerewolfKills = klst.mWerewolfKills;
    }

    void write(ESM::ESMWriter &esm) override
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
    void read(ESM::ESMReader& esm) override
    {
        ESM::Faction faction;
        bool isDeleted = false;

        faction.load(esm, isDeleted);
        std::string id = Misc::StringUtils::lowerCase(faction.mId);

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
    void read(ESM::ESMReader &esm) override
    {
        std::string itemid = esm.getHNString("NAME");
        Misc::StringUtils::lowerCaseInPlace(itemid);

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
    void write(ESM::ESMWriter &esm) override
    {
        ESM::StolenItems items;
        for (std::map<std::string, std::set<Owner> >::const_iterator it = mStolenItems.begin(); it != mStolenItems.end(); ++it)
        {
            std::map<std::pair<std::string, bool>, int> owners;
            for (const auto & ownerIt : it->second)
            {
                owners.insert(std::make_pair(std::make_pair(ownerIt.first, ownerIt.second)
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
    void read(ESM::ESMReader& esm) override
    {
        INFO info;
        info.load(esm);
    }
};

class ConvertDIAL : public Converter
{
public:
    void read(ESM::ESMReader& esm) override
    {
        std::string id = esm.getHNString("NAME");
        DIAL dial;
        dial.load(esm);
        if (dial.mIndex > 0)
            mDials[id] = dial;
    }
    void write(ESM::ESMWriter &esm) override
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
    void read(ESM::ESMReader& esm) override
    {
        std::string id = esm.getHNString("NAME");
        QUES quest;
        quest.load(esm);
    }
};

class ConvertJOUR : public Converter
{
public:
    void read(ESM::ESMReader& esm) override
    {
        JOUR journal;
        journal.load(esm);
    }
};

class ConvertGAME : public Converter
{
public:
    ConvertGAME()
        : mHasGame(false)
    {
    }

    void read(ESM::ESMReader &esm) override
    {
        mGame.load(esm);
        mHasGame = true;
    }

    int validateWeatherID(int weatherID)
    {
        if(weatherID >= -1 && weatherID < 10)
        {
            return weatherID;
        }
        else
        {
            std::stringstream error;
            error << "Invalid weather ID:" << weatherID << std::endl;
            throw std::runtime_error(error.str());
        }
    }

    void write(ESM::ESMWriter &esm) override
    {
        if (!mHasGame)
            return;
        esm.startRecord(ESM::REC_WTHR);
        ESM::WeatherState weather;
        weather.mTimePassed = 0.0f;
        weather.mFastForward = false;
        weather.mWeatherUpdateTime = mGame.mGMDT.mTimeOfNextTransition - mContext->mHour;
        weather.mTransitionFactor = 1 - (mGame.mGMDT.mWeatherTransition / 100.0f);
        weather.mCurrentWeather = validateWeatherID(mGame.mGMDT.mCurrentWeather);
        weather.mNextWeather = validateWeatherID(mGame.mGMDT.mNextWeather);
        weather.mQueuedWeather = -1;
        // TODO: Determine how ModRegion modifiers are saved in Morrowind.
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
    void read(ESM::ESMReader &esm) override
    {
        SCPT script;
        script.load(esm);
        ESM::GlobalScript out;
        convertSCPT(script, out);
        mScripts.push_back(out);
    }
    void write(ESM::ESMWriter &esm) override
    {
        for (const auto & script : mScripts)
        {
            esm.startRecord(ESM::REC_GSCR);
            script.save(esm);
            esm.endRecord(ESM::REC_GSCR);
        }
    }
private:
    std::vector<ESM::GlobalScript> mScripts;
};

/// Projectile converter
class ConvertPROJ : public Converter
{
public:
    int getStage() override { return 2; }
    void read(ESM::ESMReader& esm) override;
    void write(ESM::ESMWriter& esm) override;
private:
    void convertBaseState(ESM::BaseProjectileState& base, const PROJ::PNAM& pnam);
    PROJ mProj;
};

class ConvertSPLM : public Converter
{
public:
    void read(ESM::ESMReader& esm) override;
    void write(ESM::ESMWriter& esm) override;
private:
    SPLM mSPLM;
};

}

#endif
