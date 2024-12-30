#ifndef OPENMW_ESSIMPORT_CONVERTER_H
#define OPENMW_ESSIMPORT_CONVERTER_H

#include <limits>

#include <osg/Image>
#include <osg/ref_ptr>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/esm3/cellstate.hpp>
#include <components/esm3/custommarkerstate.hpp>
#include <components/esm3/dialoguestate.hpp>
#include <components/esm3/globalscript.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadglob.hpp>
#include <components/esm3/projectilestate.hpp>
#include <components/esm3/queststate.hpp>
#include <components/esm3/stolenitems.hpp>
#include <components/esm3/weatherstate.hpp>

#include <components/misc/strings/algorithm.hpp>

#include "importcntc.hpp"
#include "importcrec.hpp"

#include "importcellref.hpp"
#include "importdial.hpp"
#include "importercontext.hpp"
#include "importgame.hpp"
#include "importinfo.hpp"
#include "importjour.hpp"
#include "importklst.hpp"
#include "importproj.h"
#include "importques.hpp"
#include "importscpt.hpp"
#include "importsplm.h"

#include "convertacdt.hpp"
#include "convertnpcc.hpp"
#include "convertplayer.hpp"
#include "convertscpt.hpp"
#include <components/esm/refid.hpp>

namespace ESSImport
{

    class Converter
    {
    public:
        /// @return the order for writing this converter's records to the output file, in relation to other converters
        virtual int getStage() const { return 1; }

        virtual ~Converter() = default;

        void setContext(Context& context) { mContext = &context; }

        /// @note The load method of ESM records accept the deleted flag as a parameter.
        /// I don't know can the DELE sub-record appear in saved games, so the deleted flag will be ignored.
        virtual void read(ESM::ESMReader& esm) {}

        /// Called after the input file has been read in completely, which may be necessary
        /// if the conversion process relies on information in other records
        virtual void write(ESM::ESMWriter& esm) const {}

    protected:
        Context* mContext;
    };

    /// Default converter: simply reads the record and writes it unmodified to the output
    template <typename T>
    class DefaultConverter : public Converter
    {
    public:
        int getStage() const override { return 0; }

        void read(ESM::ESMReader& esm) override
        {
            T record;
            bool isDeleted = false;

            record.load(esm, isDeleted);
            mRecords[record.mId] = record;
        }

        void write(ESM::ESMWriter& esm) const override
        {
            for (auto it = mRecords.begin(); it != mRecords.end(); ++it)
            {
                esm.startRecord(T::sRecordId);
                it->second.save(esm);
                esm.endRecord(T::sRecordId);
            }
        }

    protected:
        std::map<ESM::RefId, T> mRecords;
    };

    class ConvertNPC : public Converter
    {
    public:
        void read(ESM::ESMReader& esm) override
        {
            ESM::NPC npc;
            bool isDeleted = false;

            npc.load(esm, isDeleted);
            if (npc.mId != "player")
            {
                // Handles changes to the NPC struct, but since there is no index here
                // it will apply to ALL instances of the class. seems to be the reason for the
                // "feature" in MW where changing AI settings of one guard will change it for all guards of that refID.
                mContext->mNpcs[npc.mId] = npc;
            }
            else
            {
                mContext->mPlayer.mObject.mCreatureStats.mLevel = npc.mNpdt.mLevel;
                mContext->mPlayerBase = npc;
                // FIXME: player start spells and birthsign spells aren't listed here,
                // need to fix openmw to account for this
                mContext->mPlayer.mObject.mCreatureStats.mSpells.mSpells = npc.mSpells.mList;

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
        void read(ESM::ESMReader& esm) override
        {
            // See comment in ConvertNPC
            ESM::Creature creature;
            bool isDeleted = false;

            creature.load(esm, isDeleted);
            mContext->mCreatures[creature.mId] = creature;
        }
    };

    // Do we need ConvertCONT?
    // I've seen a CONT record in a certain save file, but the container contents in it
    // were identical to a corresponding CNTC record. See previous comment about redundancy...

    class ConvertGlobal : public DefaultConverter<ESM::Global>
    {
    public:
        void read(ESM::ESMReader& esm) override
        {
            ESM::Global global;
            bool isDeleted = false;

            global.load(esm, isDeleted);
            if (global.mId == "gamehour")
                mContext->mHour = global.mValue.getFloat();
            if (global.mId == "day")
                mContext->mDay = global.mValue.getInteger();
            if (global.mId == "month")
                mContext->mMonth = global.mValue.getInteger();
            if (global.mId == "year")
                mContext->mYear = global.mValue.getInteger();
            mRecords[global.mId] = global;
        }
    };

    class ConvertClass : public DefaultConverter<ESM::Class>
    {
    public:
        void read(ESM::ESMReader& esm) override
        {
            ESM::Class classRecord;
            bool isDeleted = false;

            classRecord.load(esm, isDeleted);
            if (classRecord.mId == "NEWCLASSID_CHARGEN")
                mContext->mCustomPlayerClassName = classRecord.mName;

            mRecords[classRecord.mId] = classRecord;
        }
    };

    class ConvertBook : public DefaultConverter<ESM::Book>
    {
    public:
        void read(ESM::ESMReader& esm) override
        {
            ESM::Book book;
            bool isDeleted = false;

            book.load(esm, isDeleted);
            if (book.mData.mSkillId == -1)
                mContext->mPlayer.mObject.mNpcStats.mUsedIds.push_back(book.mId);

            mRecords[book.mId] = book;
        }
    };

    class ConvertNPCC : public Converter
    {
    public:
        void read(ESM::ESMReader& esm) override
        {
            auto id = esm.getHNRefId("NAME");
            NPCC npcc;
            npcc.load(esm);
            if (id == "PlayerSaveGame")
            {
                convertNPCC(npcc, mContext->mPlayer.mObject);
            }
            else
            {
                int index = npcc.mNPDT.mIndex;
                mContext->mNpcChanges.insert(std::make_pair(std::make_pair(index, id), npcc));
            }
        }
    };

    class ConvertREFR : public Converter
    {
    public:
        void read(ESM::ESMReader& esm) override
        {
            CellRef refr;
            refr.load(esm);
            assert(refr.mIndexedRefId == "PlayerSaveGame");
            mContext->mPlayer.mObject.mPosition = refr.mPos;

            ESM::CreatureStats& cStats = mContext->mPlayer.mObject.mCreatureStats;
            convertACDT(refr.mActorData.mACDT, cStats);

            ESM::NpcStats& npcStats = mContext->mPlayer.mObject.mNpcStats;
            convertNpcData(refr.mActorData, npcStats);

            mSelectedSpell = refr.mActorData.mSelectedSpell;
            if (!refr.mActorData.mSelectedEnchantItem.empty())
            {
                ESM::InventoryState& invState = mContext->mPlayer.mObject.mInventory;

                for (uint32_t i = 0; i < static_cast<uint32_t>(invState.mItems.size()); ++i)
                {
                    // FIXME: in case of conflict (multiple items with this refID) use the already equipped one?
                    if (invState.mItems[i].mRef.mRefID == refr.mActorData.mSelectedEnchantItem)
                        invState.mSelectedEnchantItem = i;
                }
            }
        }
        void write(ESM::ESMWriter& esm) const override
        {
            esm.startRecord(ESM::REC_ASPL);
            esm.writeHNRefId("ID__", mSelectedSpell);
            esm.endRecord(ESM::REC_ASPL);
        }

    private:
        ESM::RefId mSelectedSpell;
    };

    class ConvertPCDT : public Converter
    {
    public:
        ConvertPCDT()
            : mFirstPersonCam(true)
            , mTeleportingEnabled(true)
            , mLevitationEnabled(true)
        {
        }

        void read(ESM::ESMReader& esm) override
        {
            PCDT pcdt;
            pcdt.load(esm);

            convertPCDT(pcdt, mContext->mPlayer, mContext->mDialogueState.mKnownTopics, mFirstPersonCam,
                mTeleportingEnabled, mLevitationEnabled, mContext->mControlsState);
        }
        void write(ESM::ESMWriter& esm) const override
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
        void read(ESM::ESMReader& esm) override
        {
            auto id = esm.getHNRefId("NAME");
            CNTC cntc;
            cntc.load(esm);
            mContext->mContainerChanges.insert(std::make_pair(std::make_pair(cntc.mIndex, id), cntc));
        }
    };

    class ConvertCREC : public Converter
    {
    public:
        void read(ESM::ESMReader& esm) override
        {
            auto id = esm.getHNRefId("NAME");
            CREC crec;
            crec.load(esm);
            mContext->mCreatureChanges.insert(std::make_pair(std::make_pair(crec.mIndex, id), crec));
        }
    };

    class ConvertFMAP : public Converter
    {
    public:
        void read(ESM::ESMReader& esm) override;
        void write(ESM::ESMWriter& esm) const override;

    private:
        osg::ref_ptr<osg::Image> mGlobalMapImage;
    };

    class ConvertCell : public Converter
    {
    public:
        void read(ESM::ESMReader& esm) override;
        void write(ESM::ESMWriter& esm) const override;

    private:
        struct Cell
        {
            ESM::Cell mCell;
            std::vector<CellRef> mRefs;
            std::vector<unsigned int> mFogOfWar;
        };

        std::map<std::string, Cell, Misc::StringUtils::CiComp> mIntCells;
        std::map<std::pair<int, int>, Cell> mExtCells;

        std::vector<ESM::CustomMarker> mMarkers;

        void writeCell(const Cell& cell, ESM::ESMWriter& esm) const;
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

        void write(ESM::ESMWriter& esm) const override
        {
            esm.startRecord(ESM::REC_DCOU);
            for (const auto& [id, count] : mKillCounter)
            {
                esm.writeHNRefId("ID__", id);
                esm.writeHNT("COUN", count);
            }
            esm.endRecord(ESM::REC_DCOU);
        }

    private:
        std::map<ESM::RefId, int> mKillCounter;
    };

    class ConvertFACT : public Converter
    {
    public:
        void read(ESM::ESMReader& esm) override
        {
            ESM::Faction faction;
            bool isDeleted = false;

            faction.load(esm, isDeleted);
            const auto& id = faction.mId;

            for (auto it = faction.mReactions.begin(); it != faction.mReactions.end(); ++it)
            {
                const auto& faction2 = it->first;
                mContext->mDialogueState.mChangedFactionReaction[id].insert(std::make_pair(faction2, it->second));
            }
        }
    };

    /// Stolen items
    class ConvertSTLN : public Converter
    {
    public:
        void read(ESM::ESMReader& esm) override
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
        void write(ESM::ESMWriter& esm) const override
        {
            ESM::StolenItems items;
            for (auto it = mStolenItems.begin(); it != mStolenItems.end(); ++it)
            {
                std::map<std::pair<ESM::RefId, bool>, int> owners;
                for (const auto& ownerIt : it->second)
                {
                    owners.insert(std::make_pair(std::make_pair(ESM::RefId::stringRefId(ownerIt.first), ownerIt.second)
                        // Since OpenMW doesn't suffer from the owner contamination bug,
                        // it needs a count argument. But for legacy savegames, we don't know
                        // this count, so must assume all items of that ID are stolen,
                        // like vanilla MW did.
                        ,
                        std::numeric_limits<int>::max()));
                }

                items.mStolenItems.insert(std::make_pair(ESM::RefId::stringRefId(it->first), owners));
            }

            esm.startRecord(ESM::REC_STLN);
            items.write(esm);
            esm.endRecord(ESM::REC_STLN);
        }

    private:
        typedef std::pair<std::string, bool> Owner; // <owner id, bool isFaction>

        std::map<std::string, std::set<Owner>> mStolenItems;
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
        void write(ESM::ESMWriter& esm) const override
        {
            for (auto it = mDials.begin(); it != mDials.end(); ++it)
            {
                esm.startRecord(ESM::REC_QUES);
                ESM::QuestState state;
                state.mFinished = 0;
                state.mState = it->second.mIndex;
                state.mTopic = ESM::RefId::stringRefId(it->first);
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

        void read(ESM::ESMReader& esm) override
        {
            mGame.load(esm);
            mHasGame = true;
        }

        int validateWeatherID(int weatherID) const
        {
            if (weatherID >= -1 && weatherID < 10)
            {
                return weatherID;
            }
            else
            {
                throw std::runtime_error("Invalid weather ID: " + std::to_string(weatherID));
            }
        }

        void write(ESM::ESMWriter& esm) const override
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
        void read(ESM::ESMReader& esm) override
        {
            SCPT script;
            script.load(esm);
            ESM::GlobalScript out;
            convertSCPT(script, out);
            mScripts.push_back(std::move(out));
        }
        void write(ESM::ESMWriter& esm) const override
        {
            for (const auto& script : mScripts)
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
        int getStage() const override { return 2; }
        void read(ESM::ESMReader& esm) override;
        void write(ESM::ESMWriter& esm) const override;

    private:
        void convertBaseState(ESM::BaseProjectileState& base, const PROJ::PNAM& pnam) const;
        PROJ mProj;
    };

    class ConvertSPLM : public Converter
    {
    public:
        void read(ESM::ESMReader& esm) override;
        void write(ESM::ESMWriter& esm) const override;

    private:
        SPLM mSPLM;
    };

}

#endif
