
#include "document.hpp"

#include <cassert>

void CSMDoc::Document::load (const std::vector<boost::filesystem::path>::const_iterator& begin,
    const std::vector<boost::filesystem::path>::const_iterator& end, bool lastAsModified)
{
    assert (begin!=end);

    std::vector<boost::filesystem::path>::const_iterator end2 (end);

    if (lastAsModified)
        --end2;

    for (std::vector<boost::filesystem::path>::const_iterator iter (begin); iter!=end2; ++iter)
        getData().loadFile (*iter, true);

    if (lastAsModified)
        getData().loadFile (*end2, false);

    addOptionalGmsts();
}

void CSMDoc::Document::addOptionalGmsts()
{
    static const char *sFloats[] =
    {
        "fCombatDistanceWerewolfMod",
        "fFleeDistance",
        "fWereWolfAcrobatics",
        "fWereWolfAgility",
        "fWereWolfAlchemy",
        "fWereWolfAlteration",
        "fWereWolfArmorer",
        "fWereWolfAthletics",
        "fWereWolfAxe",
        "fWereWolfBlock",
        "fWereWolfBluntWeapon",
        "fWereWolfConjuration",
        "fWereWolfDestruction",
        "fWereWolfEnchant",
        "fWereWolfEndurance",
        "fWereWolfFatigue",
        "fWereWolfHandtoHand",
        "fWereWolfHealth",
        "fWereWolfHeavyArmor",
        "fWereWolfIllusion",
        "fWereWolfIntellegence",
        "fWereWolfLightArmor",
        "fWereWolfLongBlade",
        "fWereWolfLuck",
        "fWereWolfMagicka",
        "fWereWolfMarksman",
        "fWereWolfMediumArmor",
        "fWereWolfMerchantile",
        "fWereWolfMysticism",
        "fWereWolfPersonality",
        "fWereWolfRestoration",
        "fWereWolfRunMult",
        "fWereWolfSecurity",
        "fWereWolfShortBlade",
        "fWereWolfSilverWeaponDamageMult",
        "fWereWolfSneak",
        "fWereWolfSpear",
        "fWereWolfSpeechcraft",
        "fWereWolfSpeed",
        "fWereWolfStrength",
        "fWereWolfUnarmored",
        "fWereWolfWillPower",
        0
    };

    static const char *sIntegers[] =
    {
        "iWereWolfBounty",
        "iWereWolfFightMod",
        "iWereWolfFleeMod",
        "iWereWolfLevelToAttack",
        0
    };

    static const char *sStrings[] =
    {
        "sCompanionShare",
        "sCompanionWarningButtonOne",
        "sCompanionWarningButtonTwo",
        "sCompanionWarningMessage",
        "sDeleteNote",
        "sEditNote",
        "sEffectSummonCreature01",
        "sEffectSummonCreature02",
        "sEffectSummonCreature03",
        "sEffectSummonCreature04",
        "sEffectSummonCreature05",
        "sEffectSummonFabricant",
        "sLevitateDisabled",
        "sMagicCreature01ID",
        "sMagicCreature02ID",
        "sMagicCreature03ID",
        "sMagicCreature04ID",
        "sMagicCreature05ID",
        "sMagicFabricantID",
        "sMaxSale",
        "sProfitValue",
        "sTeleportDisabled",
        "sWerewolfAlarmMessage",
        "sWerewolfPopup",
        "sWerewolfRefusal",
        "sWerewolfRestMessage",
        0
    };

    for (int i=0; sFloats[i]; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = sFloats[i];
        gmst.mF = 0;
        gmst.mType = ESM::VT_Float;
        addOptionalGmst (gmst);
    }

    for (int i=0; sIntegers[i]; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = sIntegers[i];
        gmst.mI = 0;
        gmst.mType = ESM::VT_Long;
        addOptionalGmst (gmst);
    }

    for (int i=0; sStrings[i]; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = sStrings[i];
        gmst.mStr = "<no text>";
        gmst.mType = ESM::VT_String;
        addOptionalGmst (gmst);
    }
}

void CSMDoc::Document::addOptionalGmst (const ESM::GameSetting& gmst)
{
    if (getData().getGmsts().searchId (gmst.mId)==-1)
    {
        CSMWorld::Record<ESM::GameSetting> record;
        record.mBase = gmst;
        record.mState = CSMWorld::RecordBase::State_BaseOnly;
        getData().getGmsts().appendRecord (record);
    }
}

void CSMDoc::Document::createBase()
{
    static const char *sGlobals[] =
    {
        "Day", "DaysPassed", "GameHour", "Month", "PCRace", "PCVampire", "PCWerewolf", "PCYear", 0
    };

    for (int i=0; sGlobals[i]; ++i)
    {
        ESM::Global record;
        record.mId = sGlobals[i];
        record.mValue = i==0 ? 1 : 0;
        record.mType = ESM::VT_Float;
        getData().getGlobals().add (record);
    }
}

CSMDoc::Document::Document (const std::vector<boost::filesystem::path>& files, bool new_)
: mTools (mData)
{
    if (files.empty())
        throw std::runtime_error ("Empty content file sequence");

    /// \todo adjust last file name:
    /// \li make sure it is located in the data-local directory (adjust path if necessary)
    /// \li make sure the extension matches the new scheme (change it if necesarry)

    mName = files.back().filename().string();

    if (files.size()>1 || !new_)
    {
        std::vector<boost::filesystem::path>::const_iterator end = files.end();

        if (new_)
            --end;

        load (files.begin(), end, !new_);
    }

    if (new_ && files.size()==1)
        createBase();

    connect (&mUndoStack, SIGNAL (cleanChanged (bool)), this, SLOT (modificationStateChanged (bool)));

    connect (&mTools, SIGNAL (progress (int, int, int)), this, SLOT (progress (int, int, int)));
    connect (&mTools, SIGNAL (done (int)), this, SLOT (operationDone (int)));

     // dummy implementation -> remove when proper save is implemented.
    mSaveCount = 0;
    connect (&mSaveTimer, SIGNAL(timeout()), this, SLOT (saving()));
}

QUndoStack& CSMDoc::Document::getUndoStack()
{
    return mUndoStack;
}

int CSMDoc::Document::getState() const
{
    int state = 0;

    if (!mUndoStack.isClean())
        state |= State_Modified;

    if (mSaveCount)
        state |= State_Locked | State_Saving | State_Operation;

    if (int operations = mTools.getRunningOperations())
        state |= State_Locked | State_Operation | operations;

    return state;
}

const std::string& CSMDoc::Document::getName() const
{
    return mName;
}

void CSMDoc::Document::save()
{
    mSaveCount = 1;
    mSaveTimer.start (500);
    emit stateChanged (getState(), this);
    emit progress (1, 16, State_Saving, 1, this);
}

CSMWorld::UniversalId CSMDoc::Document::verify()
{
    CSMWorld::UniversalId id = mTools.runVerifier();
    emit stateChanged (getState(), this);
    return id;
}

void CSMDoc::Document::abortOperation (int type)
{
    mTools.abortOperation (type);

    if (type==State_Saving)
    {
        mSaveTimer.stop();
        emit stateChanged (getState(), this);
    }
}

void CSMDoc::Document::modificationStateChanged (bool clean)
{
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::operationDone (int type)
{
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::saving()
{
    ++mSaveCount;

    emit progress (mSaveCount, 16, State_Saving, 1, this);

    if (mSaveCount>15)
    {
        mSaveCount = 0;
        mSaveTimer.stop();
        mUndoStack.setClean();
        emit stateChanged (getState(), this);
    }
}

const CSMWorld::Data& CSMDoc::Document::getData() const
{
    return mData;
}

CSMWorld::Data& CSMDoc::Document::getData()
{
    return mData;
}

CSMTools::ReportModel *CSMDoc::Document::getReport (const CSMWorld::UniversalId& id)
{
    return mTools.getReport (id);
}

void CSMDoc::Document::progress (int current, int max, int type)
{
    emit progress (current, max, type, 1, this);
}