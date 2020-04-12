#include "document.hpp"

#include <cassert>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "../world/defaultgmsts.hpp"

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include <components/debug/debuglog.hpp>

void CSMDoc::Document::addGmsts()
{
    for (size_t i=0; i < CSMWorld::DefaultGmsts::FloatCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = CSMWorld::DefaultGmsts::Floats[i];
        gmst.mValue.setType (ESM::VT_Float);
        gmst.mValue.setFloat (CSMWorld::DefaultGmsts::FloatsDefaultValues[i]);
        getData().getGmsts().add (gmst);
    }

    for (size_t i=0; i < CSMWorld::DefaultGmsts::IntCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = CSMWorld::DefaultGmsts::Ints[i];
        gmst.mValue.setType (ESM::VT_Int);
        gmst.mValue.setInteger (CSMWorld::DefaultGmsts::IntsDefaultValues[i]);
        getData().getGmsts().add (gmst);
    }

    for (size_t i=0; i < CSMWorld::DefaultGmsts::StringCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = CSMWorld::DefaultGmsts::Strings[i];
        gmst.mValue.setType (ESM::VT_String);
        gmst.mValue.setString ("");
        getData().getGmsts().add (gmst);
    }
}

void CSMDoc::Document::addOptionalGmsts()
{
    for (size_t i=0; i < CSMWorld::DefaultGmsts::OptionalFloatCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = CSMWorld::DefaultGmsts::OptionalFloats[i];
        gmst.blank();
        gmst.mValue.setType (ESM::VT_Float);
        addOptionalGmst (gmst);
    }

    for (size_t i=0; i < CSMWorld::DefaultGmsts::OptionalIntCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = CSMWorld::DefaultGmsts::OptionalInts[i];
        gmst.blank();
        gmst.mValue.setType (ESM::VT_Int);
        addOptionalGmst (gmst);
    }

    for (size_t i=0; i < CSMWorld::DefaultGmsts::OptionalStringCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = CSMWorld::DefaultGmsts::OptionalStrings[i];
        gmst.blank();
        gmst.mValue.setType (ESM::VT_String);
        gmst.mValue.setString ("<no text>");
        addOptionalGmst (gmst);
    }
}

void CSMDoc::Document::addOptionalGlobals()
{
    static const char *sGlobals[] =
    {
        "DaysPassed",
        "PCWerewolf",
        "PCYear",
        0
    };

    for (int i=0; sGlobals[i]; ++i)
    {
        ESM::Global global;
        global.mId = sGlobals[i];
        global.blank();
        global.mValue.setType (ESM::VT_Long);

        if (i==0)
            global.mValue.setInteger (1); // dayspassed starts counting at 1

        addOptionalGlobal (global);
    }
}

void CSMDoc::Document::addOptionalMagicEffects()
{
    for (int i=ESM::MagicEffect::SummonFabricant; i<=ESM::MagicEffect::SummonCreature05; ++i)
    {
        ESM::MagicEffect effect;
        effect.mIndex = i;
        effect.mId = ESM::MagicEffect::indexToId (i);
        effect.blank();

        addOptionalMagicEffect (effect);
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

void CSMDoc::Document::addOptionalGlobal (const ESM::Global& global)
{
    if (getData().getGlobals().searchId (global.mId)==-1)
    {
        CSMWorld::Record<ESM::Global> record;
        record.mBase = global;
        record.mState = CSMWorld::RecordBase::State_BaseOnly;
        getData().getGlobals().appendRecord (record);
    }
}

void CSMDoc::Document::addOptionalMagicEffect (const ESM::MagicEffect& magicEffect)
{
    if (getData().getMagicEffects().searchId (magicEffect.mId)==-1)
    {
        CSMWorld::Record<ESM::MagicEffect> record;
        record.mBase = magicEffect;
        record.mState = CSMWorld::RecordBase::State_BaseOnly;
        getData().getMagicEffects().appendRecord (record);
    }
}

void CSMDoc::Document::createBase()
{
    static const char *sGlobals[] =
    {
        "Day",
        "DaysPassed",
        "GameHour",
        "Month",
        "PCRace",
        "PCVampire",
        "PCWerewolf",
        "PCYear",
        0
    };

    for (int i=0; sGlobals[i]; ++i)
    {
        ESM::Global record;
        record.mId = sGlobals[i];
        record.mValue.setType (i==2 ? ESM::VT_Float : ESM::VT_Long);

        if (i==0 || i==1)
            record.mValue.setInteger (1);

        getData().getGlobals().add (record);
    }

    addGmsts();

    for (int i=0; i<27; ++i)
    {
        ESM::Skill record;
        record.mIndex = i;
        record.mId = ESM::Skill::indexToId (record.mIndex);
        record.blank();

        getData().getSkills().add (record);
    }

    static const char *sVoice[] =
    {
        "Intruder",
        "Attack",
        "Hello",
        "Thief",
        "Alarm",
        "Idle",
        "Flee",
        "Hit",
        0
    };

    for (int i=0; sVoice[i]; ++i)
    {
        ESM::Dialogue record;
        record.mId = sVoice[i];
        record.mType = ESM::Dialogue::Voice;
        record.blank();

        getData().getTopics().add (record);
    }

    static const char *sGreetings[] =
    {
        "Greeting 0",
        "Greeting 1",
        "Greeting 2",
        "Greeting 3",
        "Greeting 4",
        "Greeting 5",
        "Greeting 6",
        "Greeting 7",
        "Greeting 8",
        "Greeting 9",
        0
    };

    for (int i=0; sGreetings[i]; ++i)
    {
        ESM::Dialogue record;
        record.mId = sGreetings[i];
        record.mType = ESM::Dialogue::Greeting;
        record.blank();

        getData().getTopics().add (record);
    }

    static const char *sPersuasion[] =
    {
        "Intimidate Success",
        "Intimidate Fail",
        "Service Refusal",
        "Admire Success",
        "Taunt Success",
        "Bribe Success",
        "Info Refusal",
        "Admire Fail",
        "Taunt Fail",
        "Bribe Fail",
        0
    };

    for (int i=0; sPersuasion[i]; ++i)
    {
        ESM::Dialogue record;
        record.mId = sPersuasion[i];
        record.mType = ESM::Dialogue::Persuasion;
        record.blank();

        getData().getTopics().add (record);
    }

    for (int i=0; i<ESM::MagicEffect::Length; ++i)
    {
        ESM::MagicEffect record;

        record.mIndex = i;
        record.mId = ESM::MagicEffect::indexToId (i);

        record.blank();

        getData().getMagicEffects().add (record);
    }
}

CSMDoc::Document::Document (const Files::ConfigurationManager& configuration,
    const std::vector< boost::filesystem::path >& files,bool new_,
    const boost::filesystem::path& savePath, const boost::filesystem::path& resDir,
    ToUTF8::FromType encoding, const std::vector<std::string>& blacklistedScripts,
    bool fsStrict, const Files::PathContainer& dataPaths, const std::vector<std::string>& archives)
: mSavePath (savePath), mContentFiles (files), mNew (new_), mData (encoding, fsStrict, dataPaths, archives, resDir),
  mTools (*this, encoding),
  mProjectPath ((configuration.getUserDataPath() / "projects") /
  (savePath.filename().string() + ".project")),
  mSavingOperation (*this, mProjectPath, encoding),
  mSaving (&mSavingOperation),
  mResDir(resDir), mRunner (mProjectPath),
  mDirty (false), mIdCompletionManager(mData)
{
    if (mContentFiles.empty())
        throw std::runtime_error ("Empty content file sequence");

    if (mNew || !boost::filesystem::exists (mProjectPath))
    {
        boost::filesystem::path filtersPath (configuration.getUserDataPath() / "defaultfilters");

        boost::filesystem::ofstream destination(mProjectPath, std::ios::out | std::ios::binary);
        if (!destination.is_open())
            throw std::runtime_error("Can not create project file: " + mProjectPath.string());
        destination.exceptions(std::ios::failbit | std::ios::badbit);

        if (!boost::filesystem::exists (filtersPath))
            filtersPath = mResDir / "defaultfilters";

        boost::filesystem::ifstream source(filtersPath, std::ios::in | std::ios::binary);
        if (!source.is_open())
            throw std::runtime_error("Can not read filters file: " + filtersPath.string());
        source.exceptions(std::ios::failbit | std::ios::badbit);

        destination << source.rdbuf();
    }

    if (mNew)
    {
        if (mContentFiles.size()==1)
            createBase();
    }

    mBlacklist.add (CSMWorld::UniversalId::Type_Script, blacklistedScripts);

    addOptionalGmsts();
    addOptionalGlobals();
    addOptionalMagicEffects();

    connect (&mUndoStack, SIGNAL (cleanChanged (bool)), this, SLOT (modificationStateChanged (bool)));

    connect (&mTools, SIGNAL (progress (int, int, int)), this, SLOT (progress (int, int, int)));
    connect (&mTools, SIGNAL (done (int, bool)), this, SIGNAL (operationDone (int, bool)));
    connect (&mTools, SIGNAL (done (int, bool)), this, SLOT (operationDone2 (int, bool)));
    connect (&mTools, SIGNAL (mergeDone (CSMDoc::Document*)),
            this, SIGNAL (mergeDone (CSMDoc::Document*)));

    connect (&mSaving, SIGNAL (progress (int, int, int)), this, SLOT (progress (int, int, int)));
    connect (&mSaving, SIGNAL (done (int, bool)), this, SLOT (operationDone2 (int, bool)));

    connect (
        &mSaving, SIGNAL (reportMessage (const CSMDoc::Message&, int)),
        this, SLOT (reportMessage (const CSMDoc::Message&, int)));

    connect (&mRunner, SIGNAL (runStateChanged()), this, SLOT (runStateChanged()));
}

CSMDoc::Document::~Document()
{
}

QUndoStack& CSMDoc::Document::getUndoStack()
{
    return mUndoStack;
}

int CSMDoc::Document::getState() const
{
    int state = 0;

    if (!mUndoStack.isClean() || mDirty)
        state |= State_Modified;

    if (mSaving.isRunning())
        state |= State_Locked | State_Saving | State_Operation;

    if (mRunner.isRunning())
        state |= State_Locked | State_Running;

    if (int operations = mTools.getRunningOperations())
        state |= State_Locked | State_Operation | operations;

    return state;
}

const boost::filesystem::path& CSMDoc::Document::getResourceDir() const
{
    return mResDir;
}

const boost::filesystem::path& CSMDoc::Document::getSavePath() const
{
    return mSavePath;
}

const boost::filesystem::path& CSMDoc::Document::getProjectPath() const
{
    return mProjectPath;
}

const std::vector<boost::filesystem::path>& CSMDoc::Document::getContentFiles() const
{
    return mContentFiles;
}

bool CSMDoc::Document::isNew() const
{
    return mNew;
}

void CSMDoc::Document::save()
{
    if (mSaving.isRunning())
        throw std::logic_error (
            "Failed to initiate save, because a save operation is already running.");

    mSaving.start();

    emit stateChanged (getState(), this);
}

CSMWorld::UniversalId CSMDoc::Document::verify (const CSMWorld::UniversalId& reportId)
{
    CSMWorld::UniversalId id = mTools.runVerifier (reportId);
    emit stateChanged (getState(), this);
    return id;
}


CSMWorld::UniversalId CSMDoc::Document::newSearch()
{
    return mTools.newSearch();
}

void CSMDoc::Document::runSearch (const CSMWorld::UniversalId& searchId, const CSMTools::Search& search)
{
    mTools.runSearch (searchId, search);
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::runMerge (std::unique_ptr<CSMDoc::Document> target)
{
    mTools.runMerge (std::move(target));
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::abortOperation (int type)
{
    if (type==State_Saving)
        mSaving.abort();
    else
        mTools.abortOperation (type);
}

void CSMDoc::Document::modificationStateChanged (bool clean)
{
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::reportMessage (const CSMDoc::Message& message, int type)
{
    /// \todo find a better way to get these messages to the user.
    Log(Debug::Info) << message.mMessage;
}

void CSMDoc::Document::operationDone2 (int type, bool failed)
{
    if (type==CSMDoc::State_Saving && !failed)
        mDirty = false;

    emit stateChanged (getState(), this);
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

bool CSMDoc::Document::isBlacklisted (const CSMWorld::UniversalId& id)
    const
{
    return mBlacklist.isBlacklisted (id);
}

void CSMDoc::Document::startRunning (const std::string& profile,
    const std::string& startupInstruction)
{
    std::vector<std::string> contentFiles;

    for (std::vector<boost::filesystem::path>::const_iterator iter (mContentFiles.begin());
        iter!=mContentFiles.end(); ++iter)
        contentFiles.push_back (iter->filename().string());

    mRunner.configure (getData().getDebugProfiles().getRecord (profile).get(), contentFiles,
        startupInstruction);

    int state = getState();

    if (state & State_Modified)
    {
        // need to save first
        mRunner.start (true);

        new SaveWatcher (&mRunner, &mSaving); // no, that is not a memory leak. Qt is weird.

        if (!(state & State_Saving))
            save();
    }
    else
        mRunner.start();
}

void CSMDoc::Document::stopRunning()
{
    mRunner.stop();
}

QTextDocument *CSMDoc::Document::getRunLog()
{
    return mRunner.getLog();
}

void CSMDoc::Document::runStateChanged()
{
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::progress (int current, int max, int type)
{
    emit progress (current, max, type, 1, this);
}

CSMWorld::IdCompletionManager &CSMDoc::Document::getIdCompletionManager()
{
    return mIdCompletionManager;
}

void CSMDoc::Document::flagAsDirty()
{
    mDirty = true;
}
