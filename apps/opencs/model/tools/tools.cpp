#include "tools.hpp"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../doc/document.hpp"

#include "birthsigncheck.hpp"
#include "bodypartcheck.hpp"
#include "classcheck.hpp"
#include "enchantmentcheck.hpp"
#include "factioncheck.hpp"
#include "gmstcheck.hpp"
#include "journalcheck.hpp"
#include "magiceffectcheck.hpp"
#include "mandatoryid.hpp"
#include "mergeoperation.hpp"
#include "pathgridcheck.hpp"
#include "racecheck.hpp"
#include "referenceablecheck.hpp"
#include "referencecheck.hpp"
#include "regioncheck.hpp"
#include "reportmodel.hpp"
#include "scriptcheck.hpp"
#include "searchoperation.hpp"
#include "skillcheck.hpp"
#include "soundcheck.hpp"
#include "soundgencheck.hpp"
#include "spellcheck.hpp"
#include "startscriptcheck.hpp"
#include "topicinfocheck.hpp"

#include <apps/opencs/model/doc/operationholder.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/refidcollection.hpp>

namespace CSMDoc
{
    struct Message;
}

CSMDoc::OperationHolder* CSMTools::Tools::get(int type)
{
    switch (type)
    {
        case CSMDoc::State_Verifying:
            return &mVerifier;
        case CSMDoc::State_Searching:
            return &mSearch;
        case CSMDoc::State_Merging:
            return &mMerge;
    }

    return nullptr;
}

const CSMDoc::OperationHolder* CSMTools::Tools::get(int type) const
{
    return const_cast<Tools*>(this)->get(type);
}

CSMDoc::OperationHolder* CSMTools::Tools::getVerifier()
{
    if (!mVerifierOperation)
    {
        mVerifierOperation = new CSMDoc::Operation(CSMDoc::State_Verifying, false);

        connect(&mVerifier, &CSMDoc::OperationHolder::progress, this, &Tools::progress);
        connect(&mVerifier, &CSMDoc::OperationHolder::done, this, &Tools::done);
        connect(&mVerifier, &CSMDoc::OperationHolder::reportMessage, this, &Tools::verifierMessage);

        std::vector<ESM::RefId> mandatoryRefIds;
        {
            auto mandatoryIds = { "Day", "DaysPassed", "GameHour", "Month", "PCRace" };
            for (auto& id : mandatoryIds)
                mandatoryRefIds.push_back(ESM::RefId::stringRefId(id));
        }

        mVerifierOperation->appendStage(new MandatoryIdStage(
            mData.getGlobals(), CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Globals), mandatoryRefIds));

        mVerifierOperation->appendStage(new SkillCheckStage(mData.getSkills()));

        mVerifierOperation->appendStage(new ClassCheckStage(mData.getClasses()));

        mVerifierOperation->appendStage(new FactionCheckStage(mData.getFactions()));

        mVerifierOperation->appendStage(new RaceCheckStage(mData.getRaces()));

        mVerifierOperation->appendStage(
            new SoundCheckStage(mData.getSounds(), mData.getResources(CSMWorld::UniversalId::Type_SoundsRes)));

        mVerifierOperation->appendStage(new RegionCheckStage(mData.getRegions()));

        mVerifierOperation->appendStage(
            new BirthsignCheckStage(mData.getBirthsigns(), mData.getResources(CSMWorld::UniversalId::Type_Textures)));

        mVerifierOperation->appendStage(new SpellCheckStage(mData.getSpells()));

        mVerifierOperation->appendStage(
            new ReferenceableCheckStage(mData.getReferenceables().getDataSet(), mData.getRaces(), mData.getClasses(),
                mData.getFactions(), mData.getScripts(), mData.getResources(CSMWorld::UniversalId::Type_Meshes),
                mData.getResources(CSMWorld::UniversalId::Type_Icons), mData.getBodyParts()));

        mVerifierOperation->appendStage(new ReferenceCheckStage(mData.getReferences(), mData.getReferenceables(),
            mData.getCells(), mData.getFactions(), mData.getBodyParts()));

        mVerifierOperation->appendStage(new ScriptCheckStage(mDocument));

        mVerifierOperation->appendStage(new StartScriptCheckStage(mData.getStartScripts(), mData.getScripts()));

        mVerifierOperation->appendStage(new BodyPartCheckStage(mData.getBodyParts(),
            mData.getResources(CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Meshes)), mData.getRaces()));

        mVerifierOperation->appendStage(new PathgridCheckStage(mData.getPathgrids()));

        mVerifierOperation->appendStage(
            new SoundGenCheckStage(mData.getSoundGens(), mData.getSounds(), mData.getReferenceables()));

        mVerifierOperation->appendStage(new MagicEffectCheckStage(mData.getMagicEffects(), mData.getSounds(),
            mData.getReferenceables(), mData.getResources(CSMWorld::UniversalId::Type_Icons),
            mData.getResources(CSMWorld::UniversalId::Type_Textures)));

        mVerifierOperation->appendStage(new GmstCheckStage(mData.getGmsts()));

        mVerifierOperation->appendStage(new TopicInfoCheckStage(mData.getTopicInfos(), mData.getCells(),
            mData.getClasses(), mData.getFactions(), mData.getGmsts(), mData.getGlobals(), mData.getJournals(),
            mData.getRaces(), mData.getRegions(), mData.getTopics(), mData.getReferenceables().getDataSet(),
            mData.getResources(CSMWorld::UniversalId::Type_SoundsRes)));

        mVerifierOperation->appendStage(new JournalCheckStage(mData.getJournals(), mData.getJournalInfos()));

        mVerifierOperation->appendStage(new EnchantmentCheckStage(mData.getEnchantments()));

        mVerifier.setOperation(mVerifierOperation);
    }

    return &mVerifier;
}

CSMTools::Tools::Tools(CSMDoc::Document& document, ToUTF8::FromType encoding)
    : mDocument(document)
    , mData(document.getData())
    , mVerifierOperation(nullptr)
    , mSearchOperation(nullptr)
    , mMergeOperation(nullptr)
    , mNextReportNumber(0)
    , mEncoding(encoding)
{
    // index 0: load error log
    mReports.insert(std::make_pair(mNextReportNumber++, new ReportModel));
    mActiveReports.insert(std::make_pair(CSMDoc::State_Loading, 0));

    connect(&mSearch, &CSMDoc::OperationHolder::progress, this, &Tools::progress);
    connect(&mSearch, &CSMDoc::OperationHolder::done, this, &Tools::done);
    connect(&mSearch, &CSMDoc::OperationHolder::reportMessage, this, &Tools::verifierMessage);

    connect(&mMerge, &CSMDoc::OperationHolder::progress, this, &Tools::progress);
    connect(&mMerge, &CSMDoc::OperationHolder::done, this, &Tools::done);
    // don't need to connect report message, since there are no messages for merge
}

CSMTools::Tools::~Tools()
{
    if (mVerifierOperation)
    {
        mVerifier.abortAndWait();
        delete mVerifierOperation;
    }

    if (mSearchOperation)
    {
        mSearch.abortAndWait();
        delete mSearchOperation;
    }

    if (mMergeOperation)
    {
        mMerge.abortAndWait();
        delete mMergeOperation;
    }

    for (std::map<int, ReportModel*>::iterator iter(mReports.begin()); iter != mReports.end(); ++iter)
        delete iter->second;
}

CSMWorld::UniversalId CSMTools::Tools::runVerifier(const CSMWorld::UniversalId& reportId)
{
    int reportNumber = reportId.getType() == CSMWorld::UniversalId::Type_VerificationResults ? reportId.getIndex()
                                                                                             : mNextReportNumber++;

    if (mReports.find(reportNumber) == mReports.end())
        mReports.insert(std::make_pair(reportNumber, new ReportModel));

    mActiveReports[CSMDoc::State_Verifying] = reportNumber;

    getVerifier()->start();

    return CSMWorld::UniversalId(CSMWorld::UniversalId::Type_VerificationResults, reportNumber);
}

CSMWorld::UniversalId CSMTools::Tools::newSearch()
{
    mReports.insert(std::make_pair(mNextReportNumber++, new ReportModel(true, false)));

    return CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Search, mNextReportNumber - 1);
}

void CSMTools::Tools::runSearch(const CSMWorld::UniversalId& searchId, const Search& search)
{
    mActiveReports[CSMDoc::State_Searching] = searchId.getIndex();

    if (!mSearchOperation)
    {
        mSearchOperation = new SearchOperation(mDocument);
        mSearch.setOperation(mSearchOperation);
    }

    mSearchOperation->configure(search);

    mSearch.start();
}

void CSMTools::Tools::runMerge(std::unique_ptr<CSMDoc::Document> target)
{
    // not setting an active report, because merge does not produce messages

    if (!mMergeOperation)
    {
        mMergeOperation = new MergeOperation(mDocument, mEncoding);
        mMerge.setOperation(mMergeOperation);
        connect(mMergeOperation, &MergeOperation::mergeDone, this, &Tools::mergeDone);
    }

    target->flagAsDirty();

    mMergeOperation->setTarget(std::move(target));

    mMerge.start();
}

void CSMTools::Tools::abortOperation(int type)
{
    if (CSMDoc::OperationHolder* operation = get(type))
        operation->abort();
}

int CSMTools::Tools::getRunningOperations() const
{
    static const int sOperations[] = {
        CSMDoc::State_Verifying,
        CSMDoc::State_Searching,
        CSMDoc::State_Merging,
        -1,
    };

    int result = 0;

    for (int i = 0; sOperations[i] != -1; ++i)
        if (const CSMDoc::OperationHolder* operation = get(sOperations[i]))
            if (operation->isRunning())
                result |= sOperations[i];

    return result;
}

CSMTools::ReportModel* CSMTools::Tools::getReport(const CSMWorld::UniversalId& id)
{
    if (id.getType() != CSMWorld::UniversalId::Type_VerificationResults
        && id.getType() != CSMWorld::UniversalId::Type_LoadErrorLog
        && id.getType() != CSMWorld::UniversalId::Type_Search)
        throw std::logic_error("invalid request for report model: " + id.toString());

    return mReports.at(id.getIndex());
}

void CSMTools::Tools::verifierMessage(const CSMDoc::Message& message, int type)
{
    std::map<int, int>::iterator iter = mActiveReports.find(type);

    if (iter != mActiveReports.end())
        mReports[iter->second]->add(message);
}
