
#include "tools.hpp"

#include <QThreadPool>

#include "verifier.hpp"

#include "../doc/state.hpp"

#include "../world/data.hpp"
#include "../world/universalid.hpp"

#include "reportmodel.hpp"
#include "mandatoryid.hpp"
#include "skillcheck.hpp"

CSMTools::Operation *CSMTools::Tools::get (int type)
{
    switch (type)
    {
        case CSMDoc::State_Verifying: return mVerifier;
    }

    return 0;
}

const CSMTools::Operation *CSMTools::Tools::get (int type) const
{
    return const_cast<Tools *> (this)->get (type);
}

CSMTools::Verifier *CSMTools::Tools::getVerifier()
{
    if (!mVerifier)
    {
        mVerifier = new Verifier;

        connect (mVerifier, SIGNAL (progress (int, int, int)), this, SIGNAL (progress (int, int, int)));
        connect (mVerifier, SIGNAL (finished()), this, SLOT (verifierDone()));
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
    if (Operation *operation = get (type))
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
        if (const Operation *operation = get (sOperations[i]))
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

void CSMTools::Tools::verifierDone()
{
    emit done (CSMDoc::State_Verifying);
}

void CSMTools::Tools::verifierMessage (const QString& message, int type)
{
    std::map<int, int>::iterator iter = mActiveReports.find (type);

    if (iter!=mActiveReports.end())
        mReports[iter->second]->add (message.toStdString());
}