
#include "tools.hpp"

#include <QThreadPool>

#include "../doc/state.hpp"
#include "../doc/operation.hpp"

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

CSMDoc::Operation *CSMTools::Tools::get (int type)
{
    switch (type)
    {
        case CSMDoc::State_Verifying: return mVerifier;
    }

    return 0;
}

const CSMDoc::Operation *CSMTools::Tools::get (int type) const
{
    return const_cast<Tools *> (this)->get (type);
}

CSMDoc::Operation *CSMTools::Tools::getVerifier()
{
    if (!mVerifier)
    {
        mVerifier = new CSMDoc::Operation (CSMDoc::State_Verifying, false);

        connect (mVerifier, SIGNAL (progress (int, int, int)), this, SIGNAL (progress (int, int, int)));
        connect (mVerifier, SIGNAL (done (int)), this, SIGNAL (done (int)));
        connect (mVerifier, SIGNAL (reportMessage (const QString&, int)),
            this, SLOT (verifierMessage (const QString&, int)));

        std::vector<std::string> mandatoryIds; //  I want C++11, damn it!
        mandatoryIds.push_back ("Day");
        mandatoryIds.push_back ("DaysPassed");
        mandatoryIds.push_back ("GameHour");
        mandatoryIds.push_back ("Month");
        mandatoryIds.push_back ("PCRace");
        mandatoryIds.push_back ("PCVampire");
        mandatoryIds.push_back ("PCWerewolf");
        mandatoryIds.push_back ("PCYear");

        mVerifier->appendStage (new MandatoryIdStage (mData.getGlobals(),
            CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Globals), mandatoryIds));

        mVerifier->appendStage (new SkillCheckStage (mData.getSkills()));

        mVerifier->appendStage (new ClassCheckStage (mData.getClasses()));

        mVerifier->appendStage (new FactionCheckStage (mData.getFactions()));

        mVerifier->appendStage (new RaceCheckStage (mData.getRaces()));

        mVerifier->appendStage (new SoundCheckStage (mData.getSounds()));

        mVerifier->appendStage (new RegionCheckStage (mData.getRegions()));

        mVerifier->appendStage (new BirthsignCheckStage (mData.getBirthsigns()));

        mVerifier->appendStage (new SpellCheckStage (mData.getSpells()));

	mVerifier->appendStage (new ReferenceableCheckStage (mData.getReferenceables().getDataSet(), mData.getRaces(), mData.getClasses(), mData.getFactions()));

        mVerifier->appendStage (new ScriptCheckStage (mData));
    }

    return mVerifier;
}

CSMTools::Tools::Tools (CSMWorld::Data& data) : mData (data), mVerifier (0), mNextReportNumber (0)
{
    for (std::map<int, ReportModel *>::iterator iter (mReports.begin()); iter!=mReports.end(); ++iter)
        delete iter->second;
}

CSMTools::Tools::~Tools()
{
    delete mVerifier;
}

CSMWorld::UniversalId CSMTools::Tools::runVerifier()
{
    mReports.insert (std::make_pair (mNextReportNumber++, new ReportModel));
    mActiveReports[CSMDoc::State_Verifying] = mNextReportNumber-1;

    getVerifier()->start();

    return CSMWorld::UniversalId (CSMWorld::UniversalId::Type_VerificationResults, mNextReportNumber-1);
}

void CSMTools::Tools::abortOperation (int type)
{
    if (CSMDoc::Operation *operation = get (type))
        operation->abort();
}

int CSMTools::Tools::getRunningOperations() const
{
    static const int sOperations[] =
    {
       CSMDoc::State_Verifying,
        -1
    };

    int result = 0;

    for (int i=0; sOperations[i]!=-1; ++i)
        if (const CSMDoc::Operation *operation = get (sOperations[i]))
            if (operation->isRunning())
                result |= sOperations[i];

    return result;
}

CSMTools::ReportModel *CSMTools::Tools::getReport (const CSMWorld::UniversalId& id)
{
    if (id.getType()!=CSMWorld::UniversalId::Type_VerificationResults)
        throw std::logic_error ("invalid request for report model: " + id.toString());

    return mReports.at (id.getIndex());
}

void CSMTools::Tools::verifierMessage (const QString& message, int type)
{
    std::map<int, int>::iterator iter = mActiveReports.find (type);

    if (iter!=mActiveReports.end())
        mReports[iter->second]->add (message.toUtf8().constData());
}

