#include "tools.hpp"

#include <QThreadPool>

#include "../doc/state.hpp"
#include "../doc/operation.hpp"
#include "../doc/document.hpp"

#include "../world/data.hpp"
#include "../world/universalid.hpp"

#include "reportmodel.hpp"
#include "mandatoryid.hpp"
#include "skillcheck.hpp"
#include "classcheck.hpp"
#include "factioncheck.hpp"
#include "racecheck.hpp"
#include "soundcheck.hpp"
#include "regioncheck.hpp"
#include "birthsigncheck.hpp"
#include "spellcheck.hpp"
#include "referenceablecheck.hpp"
#include "scriptcheck.hpp"
#include "bodypartcheck.hpp"
#include "referencecheck.hpp"
#include "startscriptcheck.hpp"
#include "searchoperation.hpp"
#include "pathgridcheck.hpp"
#include "soundgencheck.hpp"
#include "magiceffectcheck.hpp"
#include "mergeoperation.hpp"
#include "gmstcheck.hpp"
#include "topicinfocheck.hpp"
#include "journalcheck.hpp"
#include "enchantmentcheck.hpp"

CSMDoc::OperationHolder *CSMTools::Tools::get (int type)
{
    switch (type)
    {
        case CSMDoc::State_Verifying: return &mVerifier;
        case CSMDoc::State_Searching: return &mSearch;
        case CSMDoc::State_Merging: return &mMerge;
    }

    return nullptr;
}

const CSMDoc::OperationHolder *CSMTools::Tools::get (int type) const
{
    return const_cast<Tools *> (this)->get (type);
}

CSMDoc::OperationHolder *CSMTools::Tools::getVerifier()
{
    if (!mVerifierOperation)
    {
        mVerifierOperation = new CSMDoc::Operation (CSMDoc::State_Verifying, false);

        connect (&mVerifier, SIGNAL (progress (int, int, int)), this, SIGNAL (progress (int, int, int)));
        connect (&mVerifier, SIGNAL (done (int, bool)), this, SIGNAL (done (int, bool)));
        connect (&mVerifier, SIGNAL (reportMessage (const CSMDoc::Message&, int)),
            this, SLOT (verifierMessage (const CSMDoc::Message&, int)));

        std::vector<std::string> mandatoryIds {"Day", "DaysPassed", "GameHour", "Month", "PCRace"};

        mVerifierOperation->appendStage (new MandatoryIdStage (mData.getGlobals(),
            CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Globals), mandatoryIds));

        mVerifierOperation->appendStage (new SkillCheckStage (mData.getSkills()));

        mVerifierOperation->appendStage (new ClassCheckStage (mData.getClasses()));

        mVerifierOperation->appendStage (new FactionCheckStage (mData.getFactions()));

        mVerifierOperation->appendStage (new RaceCheckStage (mData.getRaces()));

        mVerifierOperation->appendStage (new SoundCheckStage (mData.getSounds(), mData.getResources (CSMWorld::UniversalId::Type_SoundsRes)));

        mVerifierOperation->appendStage (new RegionCheckStage (mData.getRegions()));

        mVerifierOperation->appendStage (new BirthsignCheckStage (mData.getBirthsigns(), mData.getResources (CSMWorld::UniversalId::Type_Textures)));

        mVerifierOperation->appendStage (new SpellCheckStage (mData.getSpells()));

        mVerifierOperation->appendStage (new ReferenceableCheckStage (mData.getReferenceables().getDataSet(), mData.getRaces(), mData.getClasses(), mData.getFactions(), mData.getScripts(), 
                                                                      mData.getResources (CSMWorld::UniversalId::Type_Meshes), mData.getResources (CSMWorld::UniversalId::Type_Icons),
                                                                      mData.getBodyParts()));

        mVerifierOperation->appendStage (new ReferenceCheckStage(mData.getReferences(), mData.getReferenceables(), mData.getCells(), mData.getFactions()));

        mVerifierOperation->appendStage (new ScriptCheckStage (mDocument));

        mVerifierOperation->appendStage (new StartScriptCheckStage (mData.getStartScripts(), mData.getScripts()));

        mVerifierOperation->appendStage(
            new BodyPartCheckStage(
                mData.getBodyParts(),
                mData.getResources(
                    CSMWorld::UniversalId( CSMWorld::UniversalId::Type_Meshes )),
                mData.getRaces() ));

        mVerifierOperation->appendStage (new PathgridCheckStage (mData.getPathgrids()));

        mVerifierOperation->appendStage (new SoundGenCheckStage (mData.getSoundGens(),
                                                                 mData.getSounds(),
                                                                 mData.getReferenceables()));

        mVerifierOperation->appendStage (new MagicEffectCheckStage (mData.getMagicEffects(),
                                                                    mData.getSounds(),
                                                                    mData.getReferenceables(),
                                                                    mData.getResources (CSMWorld::UniversalId::Type_Icons),
                                                                    mData.getResources (CSMWorld::UniversalId::Type_Textures)));

        mVerifierOperation->appendStage (new GmstCheckStage (mData.getGmsts()));

        mVerifierOperation->appendStage (new TopicInfoCheckStage (mData.getTopicInfos(),
                                                                  mData.getCells(),
                                                                  mData.getClasses(),
                                                                  mData.getFactions(),
                                                                  mData.getGmsts(),
                                                                  mData.getGlobals(),
                                                                  mData.getJournals(),
                                                                  mData.getRaces(),
                                                                  mData.getRegions(),
                                                                  mData.getTopics(),
                                                                  mData.getReferenceables().getDataSet(),
                                                                  mData.getResources (CSMWorld::UniversalId::Type_SoundsRes)));

        mVerifierOperation->appendStage (new JournalCheckStage(mData.getJournals(), mData.getJournalInfos()));

        mVerifierOperation->appendStage (new EnchantmentCheckStage(mData.getEnchantments()));

        mVerifier.setOperation (mVerifierOperation);
    }

    return &mVerifier;
}

CSMTools::Tools::Tools (CSMDoc::Document& document, ToUTF8::FromType encoding)
: mDocument (document), mData (document.getData()), mVerifierOperation (nullptr),
  mSearchOperation (nullptr), mMergeOperation (nullptr), mNextReportNumber (0), mEncoding (encoding)
{
    // index 0: load error log
    mReports.insert (std::make_pair (mNextReportNumber++, new ReportModel));
    mActiveReports.insert (std::make_pair (CSMDoc::State_Loading, 0));

    connect (&mSearch, SIGNAL (progress (int, int, int)), this, SIGNAL (progress (int, int, int)));
    connect (&mSearch, SIGNAL (done (int, bool)), this, SIGNAL (done (int, bool)));
    connect (&mSearch, SIGNAL (reportMessage (const CSMDoc::Message&, int)),
        this, SLOT (verifierMessage (const CSMDoc::Message&, int)));

    connect (&mMerge, SIGNAL (progress (int, int, int)), this, SIGNAL (progress (int, int, int)));
    connect (&mMerge, SIGNAL (done (int, bool)), this, SIGNAL (done (int, bool)));
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

    for (std::map<int, ReportModel *>::iterator iter (mReports.begin()); iter!=mReports.end(); ++iter)
        delete iter->second;
}

CSMWorld::UniversalId CSMTools::Tools::runVerifier (const CSMWorld::UniversalId& reportId)
{
    int reportNumber = reportId.getType()==CSMWorld::UniversalId::Type_VerificationResults ?
        reportId.getIndex() : mNextReportNumber++;

    if (mReports.find (reportNumber)==mReports.end())
        mReports.insert (std::make_pair (reportNumber, new ReportModel));

    mActiveReports[CSMDoc::State_Verifying] = reportNumber;

    getVerifier()->start();

    return CSMWorld::UniversalId (CSMWorld::UniversalId::Type_VerificationResults, reportNumber);
}

CSMWorld::UniversalId CSMTools::Tools::newSearch()
{
    mReports.insert (std::make_pair (mNextReportNumber++, new ReportModel (true, false)));

    return CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Search, mNextReportNumber-1);
}

void CSMTools::Tools::runSearch (const CSMWorld::UniversalId& searchId, const Search& search)
{
    mActiveReports[CSMDoc::State_Searching] = searchId.getIndex();

    if (!mSearchOperation)
    {
        mSearchOperation = new SearchOperation (mDocument);
        mSearch.setOperation (mSearchOperation);
    }

    mSearchOperation->configure (search);

    mSearch.start();
}

void CSMTools::Tools::runMerge (std::unique_ptr<CSMDoc::Document> target)
{
    // not setting an active report, because merge does not produce messages

    if (!mMergeOperation)
    {
        mMergeOperation = new MergeOperation (mDocument, mEncoding);
        mMerge.setOperation (mMergeOperation);
        connect (mMergeOperation, SIGNAL (mergeDone (CSMDoc::Document*)),
            this, SIGNAL (mergeDone (CSMDoc::Document*)));
    }

    target->flagAsDirty();

    mMergeOperation->setTarget (std::move(target));

    mMerge.start();
}

void CSMTools::Tools::abortOperation (int type)
{
    if (CSMDoc::OperationHolder *operation = get (type))
        operation->abort();
}

int CSMTools::Tools::getRunningOperations() const
{
    static const int sOperations[] =
    {
       CSMDoc::State_Verifying,
       CSMDoc::State_Searching,
       CSMDoc::State_Merging,
        -1
    };

    int result = 0;

    for (int i=0; sOperations[i]!=-1; ++i)
        if (const CSMDoc::OperationHolder *operation = get (sOperations[i]))
            if (operation->isRunning())
                result |= sOperations[i];

    return result;
}

CSMTools::ReportModel *CSMTools::Tools::getReport (const CSMWorld::UniversalId& id)
{
    if (id.getType()!=CSMWorld::UniversalId::Type_VerificationResults &&
        id.getType()!=CSMWorld::UniversalId::Type_LoadErrorLog &&
        id.getType()!=CSMWorld::UniversalId::Type_Search)
        throw std::logic_error ("invalid request for report model: " + id.toString());

    return mReports.at (id.getIndex());
}

void CSMTools::Tools::verifierMessage (const CSMDoc::Message& message, int type)
{
    std::map<int, int>::iterator iter = mActiveReports.find (type);

    if (iter!=mActiveReports.end())
        mReports[iter->second]->add (message);
}
