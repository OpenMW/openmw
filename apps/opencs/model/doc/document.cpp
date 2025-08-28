#include "document.hpp"

#include "state.hpp"

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/doc/operationholder.hpp>
#include <apps/opencs/model/doc/runner.hpp>
#include <apps/opencs/model/doc/saving.hpp>
#include <apps/opencs/model/tools/tools.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadglob.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/variant.hpp>
#include <components/files/conversion.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stddef.h>
#include <stdexcept>
#include <utility>
#include <variant>

#include "../world/defaultgmsts.hpp"

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

namespace CSMWorld
{
    class IdCompletionManager;
}

void CSMDoc::Document::addGmsts()
{
    for (size_t i = 0; i < CSMWorld::DefaultGmsts::FloatCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = ESM::RefId::stringRefId(CSMWorld::DefaultGmsts::Floats[i]);
        gmst.mValue.setType(ESM::VT_Float);
        gmst.mRecordFlags = 0;
        gmst.mValue.setFloat(CSMWorld::DefaultGmsts::FloatsDefaultValues[i]);
        getData().getGmsts().add(gmst);
    }

    for (size_t i = 0; i < CSMWorld::DefaultGmsts::IntCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = ESM::RefId::stringRefId(CSMWorld::DefaultGmsts::Ints[i]);
        gmst.mValue.setType(ESM::VT_Int);
        gmst.mRecordFlags = 0;
        gmst.mValue.setInteger(CSMWorld::DefaultGmsts::IntsDefaultValues[i]);
        getData().getGmsts().add(gmst);
    }

    for (size_t i = 0; i < CSMWorld::DefaultGmsts::StringCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = ESM::RefId::stringRefId(CSMWorld::DefaultGmsts::Strings[i]);
        gmst.mValue.setType(ESM::VT_String);
        gmst.mRecordFlags = 0;
        gmst.mValue.setString("");
        getData().getGmsts().add(gmst);
    }
}

void CSMDoc::Document::addOptionalGmsts()
{
    for (size_t i = 0; i < CSMWorld::DefaultGmsts::OptionalFloatCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = ESM::RefId::stringRefId(CSMWorld::DefaultGmsts::OptionalFloats[i]);
        gmst.blank();
        gmst.mValue.setType(ESM::VT_Float);
        addOptionalGmst(gmst);
    }

    for (size_t i = 0; i < CSMWorld::DefaultGmsts::OptionalIntCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = ESM::RefId::stringRefId(CSMWorld::DefaultGmsts::OptionalInts[i]);
        gmst.blank();
        gmst.mValue.setType(ESM::VT_Int);
        addOptionalGmst(gmst);
    }

    for (size_t i = 0; i < CSMWorld::DefaultGmsts::OptionalStringCount; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = ESM::RefId::stringRefId(CSMWorld::DefaultGmsts::OptionalStrings[i]);
        gmst.blank();
        gmst.mValue.setType(ESM::VT_String);
        gmst.mValue.setString("<no text>");
        addOptionalGmst(gmst);
    }
}

void CSMDoc::Document::addOptionalGlobals()
{
    static constexpr std::string_view globals[] = {
        "DaysPassed",
        "PCWerewolf",
        "PCYear",
    };

    for (std::size_t i = 0; i < std::size(globals); ++i)
    {
        ESM::Global global;
        global.mId = ESM::RefId::stringRefId(globals[i]);
        global.blank();
        global.mValue.setType(ESM::VT_Long);

        if (i == 0)
            global.mValue.setInteger(1); // dayspassed starts counting at 1

        addOptionalGlobal(global);
    }
}

void CSMDoc::Document::addOptionalMagicEffects()
{
    for (int i = ESM::MagicEffect::SummonFabricant; i <= ESM::MagicEffect::SummonCreature05; ++i)
    {
        ESM::MagicEffect effect;
        effect.mIndex = i;
        effect.mId = ESM::MagicEffect::indexToRefId(i);
        effect.blank();

        addOptionalMagicEffect(effect);
    }
}

void CSMDoc::Document::addOptionalGmst(const ESM::GameSetting& gmst)
{
    if (getData().getGmsts().searchId(gmst.mId) == -1)
    {
        auto record = std::make_unique<CSMWorld::Record<ESM::GameSetting>>();
        record->mBase = gmst;
        record->mState = CSMWorld::RecordBase::State_BaseOnly;
        getData().getGmsts().appendRecord(std::move(record));
    }
}

void CSMDoc::Document::addOptionalGlobal(const ESM::Global& global)
{
    if (getData().getGlobals().searchId(global.mId) == -1)
    {
        auto record = std::make_unique<CSMWorld::Record<ESM::Global>>();
        record->mBase = global;
        record->mState = CSMWorld::RecordBase::State_BaseOnly;
        getData().getGlobals().appendRecord(std::move(record));
    }
}

void CSMDoc::Document::addOptionalMagicEffect(const ESM::MagicEffect& magicEffect)
{
    if (getData().getMagicEffects().searchId(magicEffect.mId) == -1)
    {
        auto record = std::make_unique<CSMWorld::Record<ESM::MagicEffect>>();
        record->mBase = magicEffect;
        record->mState = CSMWorld::RecordBase::State_BaseOnly;
        getData().getMagicEffects().appendRecord(std::move(record));
    }
}

void CSMDoc::Document::createBase()
{
    static constexpr std::string_view globals[] = {
        "Day",
        "DaysPassed",
        "GameHour",
        "Month",
        "PCRace",
        "PCVampire",
        "PCWerewolf",
        "PCYear",
    };

    for (std::size_t i = 0; i < std::size(globals); ++i)
    {
        ESM::Global record;
        record.mId = ESM::RefId::stringRefId(globals[i]);
        record.mRecordFlags = 0;
        record.mValue.setType(i == 2 ? ESM::VT_Float : ESM::VT_Long);

        if (i == 0 || i == 1)
            record.mValue.setInteger(1);

        getData().getGlobals().add(record);
    }

    addGmsts();

    for (int i = 0; i < ESM::Skill::Length; ++i)
    {
        ESM::Skill record;
        record.mId = *ESM::Skill::indexToRefId(i).getIf<ESM::SkillId>();
        record.blank();

        getData().getSkills().add(record);
    }

    static constexpr std::string_view voices[] = {
        "Intruder",
        "Attack",
        "Hello",
        "Thief",
        "Alarm",
        "Idle",
        "Flee",
        "Hit",
    };

    for (const std::string_view voice : voices)
    {
        ESM::Dialogue record;
        record.mId = ESM::RefId::stringRefId(voice);
        record.mStringId = voice;
        record.mType = ESM::Dialogue::Voice;
        record.blank();

        getData().getTopics().add(record);
    }

    static constexpr std::string_view greetings[] = {
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
    };

    for (const std::string_view greeting : greetings)
    {
        ESM::Dialogue record;
        record.mId = ESM::RefId::stringRefId(greeting);
        record.mStringId = greeting;
        record.mType = ESM::Dialogue::Greeting;
        record.blank();

        getData().getTopics().add(record);
    }

    static constexpr std::string_view persuasions[] = {
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
    };

    for (const std::string_view persuasion : persuasions)
    {
        ESM::Dialogue record;
        record.mId = ESM::RefId::stringRefId(persuasion);
        record.mStringId = persuasion;
        record.mType = ESM::Dialogue::Persuasion;
        record.blank();

        getData().getTopics().add(record);
    }

    for (int i = 0; i < ESM::MagicEffect::Length; ++i)
    {
        ESM::MagicEffect record;

        record.mIndex = i;
        record.mId = ESM::MagicEffect::indexToRefId(i);

        record.blank();

        getData().getMagicEffects().add(record);
    }
}

CSMDoc::Document::Document(const Files::ConfigurationManager& configuration, std::vector<std::filesystem::path> files,
    bool isNew, const std::filesystem::path& savePath, const std::filesystem::path& resDir, ToUTF8::FromType encoding,
    const Files::PathContainer& dataPaths, const std::vector<std::string>& archives)
    : mSavePath(savePath)
    , mContentFiles(std::move(files))
    , mNew(isNew)
    , mData(encoding, dataPaths, archives, resDir)
    , mTools(*this, encoding)
    , mProjectPath((configuration.getUserDataPath() / "projects") / (savePath.filename().u8string() + u8".project"))
    , mSavingOperation(*this, mProjectPath, encoding)
    , mSaving(&mSavingOperation)
    , mResDir(resDir)
    , mRunner(mProjectPath)
    , mDirty(false)
    , mIdCompletionManager(mData)
{
    if (mContentFiles.empty())
        throw std::runtime_error("Empty content file sequence");

    if (mNew || !std::filesystem::exists(mProjectPath))
    {
        auto filtersPath = configuration.getUserDataPath() / "defaultfilters";

        std::ofstream destination(mProjectPath, std::ios::out | std::ios::binary);
        if (!destination.is_open())
            throw std::runtime_error("Can not create project file: " + Files::pathToUnicodeString(mProjectPath));
        destination.exceptions(std::ios::failbit | std::ios::badbit);

        if (!std::filesystem::exists(filtersPath))
            filtersPath = mResDir / "defaultfilters";

        std::ifstream source(filtersPath, std::ios::in | std::ios::binary);
        if (!source.is_open())
            throw std::runtime_error("Can not read filters file: " + Files::pathToUnicodeString(filtersPath));
        source.exceptions(std::ios::failbit | std::ios::badbit);

        destination << source.rdbuf();
    }

    if (mNew)
    {
        if (mContentFiles.size() == 1)
            createBase();
    }

    addOptionalGmsts();
    addOptionalGlobals();
    addOptionalMagicEffects();

    connect(&mUndoStack, &QUndoStack::cleanChanged, this, &Document::modificationStateChanged);

    connect(&mTools, &CSMTools::Tools::progress, this, qOverload<int, int, int>(&Document::progress));
    connect(&mTools, &CSMTools::Tools::done, this, &Document::operationDone);
    connect(&mTools, &CSMTools::Tools::done, this, &Document::operationDone2);
    connect(&mTools, &CSMTools::Tools::mergeDone, this, &Document::mergeDone);

    connect(&mSaving, &OperationHolder::progress, this, qOverload<int, int, int>(&Document::progress));
    connect(&mSaving, &OperationHolder::done, this, &Document::operationDone2);

    connect(&mSaving, &OperationHolder::reportMessage, this, &Document::reportMessage);

    connect(&mRunner, &Runner::runStateChanged, this, &Document::runStateChanged);
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

const std::filesystem::path& CSMDoc::Document::getResourceDir() const
{
    return mResDir;
}

const std::filesystem::path& CSMDoc::Document::getSavePath() const
{
    return mSavePath;
}

const std::filesystem::path& CSMDoc::Document::getProjectPath() const
{
    return mProjectPath;
}

const std::vector<std::filesystem::path>& CSMDoc::Document::getContentFiles() const
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
        throw std::logic_error("Failed to initiate save, because a save operation is already running.");

    mSaving.start();

    emit stateChanged(getState(), this);
}

CSMWorld::UniversalId CSMDoc::Document::verify(const CSMWorld::UniversalId& reportId)
{
    CSMWorld::UniversalId id = mTools.runVerifier(reportId);
    emit stateChanged(getState(), this);
    return id;
}

CSMWorld::UniversalId CSMDoc::Document::newSearch()
{
    return mTools.newSearch();
}

void CSMDoc::Document::runSearch(const CSMWorld::UniversalId& searchId, const CSMTools::Search& search)
{
    mTools.runSearch(searchId, search);
    emit stateChanged(getState(), this);
}

void CSMDoc::Document::runMerge(std::unique_ptr<CSMDoc::Document> target)
{
    mTools.runMerge(std::move(target));
    emit stateChanged(getState(), this);
}

void CSMDoc::Document::abortOperation(int type)
{
    if (type == State_Saving)
        mSaving.abort();
    else
        mTools.abortOperation(type);
}

void CSMDoc::Document::modificationStateChanged(bool clean)
{
    emit stateChanged(getState(), this);
}

void CSMDoc::Document::reportMessage(const CSMDoc::Message& message, int type)
{
    /// \todo find a better way to get these messages to the user.
    Log(Debug::Info) << message.mMessage;
}

void CSMDoc::Document::operationDone2(int type, bool failed)
{
    if (type == CSMDoc::State_Saving && !failed)
        mDirty = false;

    emit stateChanged(getState(), this);
}

const CSMWorld::Data& CSMDoc::Document::getData() const
{
    return mData;
}

CSMWorld::Data& CSMDoc::Document::getData()
{
    return mData;
}

CSMTools::ReportModel* CSMDoc::Document::getReport(const CSMWorld::UniversalId& id)
{
    return mTools.getReport(id);
}

void CSMDoc::Document::startRunning(const std::string& profile, const std::string& startupInstruction)
{
    std::vector<std::filesystem::path> contentFiles;

    for (const auto& mContentFile : mContentFiles)
    {
        contentFiles.emplace_back(mContentFile.filename());
    }

    mRunner.configure(getData().getDebugProfiles().getRecord(ESM::RefId::stringRefId(profile)).get(), contentFiles,
        startupInstruction);

    int state = getState();

    if (state & State_Modified)
    {
        // need to save first
        mRunner.start(true);

        new SaveWatcher(&mRunner, &mSaving); // no, that is not a memory leak. Qt is weird.

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

QTextDocument* CSMDoc::Document::getRunLog()
{
    return mRunner.getLog();
}

void CSMDoc::Document::runStateChanged()
{
    emit stateChanged(getState(), this);
}

void CSMDoc::Document::progress(int current, int max, int type)
{
    emit progress(current, max, type, 1, this);
}

CSMWorld::IdCompletionManager& CSMDoc::Document::getIdCompletionManager()
{
    return mIdCompletionManager;
}

void CSMDoc::Document::flagAsDirty()
{
    mDirty = true;
}
