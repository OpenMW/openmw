#include "operation.hpp"

#include <string>
#include <vector>

#include <QTimer>

#include "../world/universalid.hpp"
#include "../settings/usersettings.hpp"

#include "state.hpp"
#include "stage.hpp"

void CSMDoc::Operation::prepareStages()
{
    mCurrentStage = mStages.begin();
    mCurrentStep = 0;
    mCurrentStepTotal = 0;
    mTotalSteps = 0;
    mError = false;

    for (std::vector<std::pair<Stage *, int> >::iterator iter (mStages.begin()); iter!=mStages.end(); ++iter)
    {
        iter->second = iter->first->setup();
        mTotalSteps += iter->second;

        for (std::map<QString, QStringList>::const_iterator iter2 (mSettings.begin()); iter2!=mSettings.end(); ++iter2)
            iter->first->updateUserSetting (iter2->first, iter2->second);
    }
}

CSMDoc::Operation::Operation (int type, bool ordered, bool finalAlways)
: mType (type), mStages(std::vector<std::pair<Stage *, int> >()), mCurrentStage(mStages.begin()),
  mCurrentStep(0), mCurrentStepTotal(0), mTotalSteps(0), mOrdered (ordered),
  mFinalAlways (finalAlways), mError(false), mConnected (false), mPrepared (false),
  mDefaultSeverity (Message::Severity_Error)
{
    mTimer = new QTimer (this);
}

CSMDoc::Operation::~Operation()
{
    for (std::vector<std::pair<Stage *, int> >::iterator iter (mStages.begin()); iter!=mStages.end(); ++iter)
        delete iter->first;
}

void CSMDoc::Operation::run()
{
    mTimer->stop();
    
    if (!mConnected)
    {
        connect (mTimer, SIGNAL (timeout()), this, SLOT (executeStage()));
        mConnected = true;
    }

    mPrepared = false;

    mTimer->start (0);
}

void CSMDoc::Operation::appendStage (Stage *stage)
{
    mStages.push_back (std::make_pair (stage, 0));
}

void CSMDoc::Operation::configureSettings (const std::vector<QString>& settings)
{
    for (std::vector<QString>::const_iterator iter (settings.begin()); iter!=settings.end(); ++iter)
    {
        mSettings.insert (std::make_pair (*iter, CSMSettings::UserSettings::instance().definitions (*iter)));
    }
}

void CSMDoc::Operation::setDefaultSeverity (Message::Severity severity)
{
    mDefaultSeverity = severity;
}

bool CSMDoc::Operation::hasError() const
{
    return mError;
}

void CSMDoc::Operation::abort()
{
    if (!mTimer->isActive())
        return;

    mError = true;

    if (mFinalAlways)
    {
        if (mStages.begin()!=mStages.end() && mCurrentStage!=--mStages.end())
        {
            mCurrentStep = 0;
            mCurrentStage = --mStages.end();
        }
    }
    else
        mCurrentStage = mStages.end();
}

void CSMDoc::Operation::updateUserSetting (const QString& name, const QStringList& value)
{
    std::map<QString, QStringList>::iterator iter = mSettings.find (name);

    if (iter!=mSettings.end())
        iter->second = value;
}

void CSMDoc::Operation::executeStage()
{
    if (!mPrepared)
    {
        prepareStages();
        mPrepared = true;
    }
    
    Messages messages (mDefaultSeverity);

    while (mCurrentStage!=mStages.end())
    {
        if (mCurrentStep>=mCurrentStage->second)
        {
            mCurrentStep = 0;
            ++mCurrentStage;
        }
        else
        {
            try
            {
                mCurrentStage->first->perform (mCurrentStep++, messages);
            }
            catch (const std::exception& e)
            {
                emit reportMessage (Message (CSMWorld::UniversalId(), e.what(), "", Message::Severity_SeriousError), mType);
                abort();
            }

            ++mCurrentStepTotal;
            break;
        }
    }

    emit progress (mCurrentStepTotal, mTotalSteps ? mTotalSteps : 1, mType);

    for (Messages::Iterator iter (messages.begin()); iter!=messages.end(); ++iter)
        emit reportMessage (*iter, mType);

    if (mCurrentStage==mStages.end())
        operationDone();
}

void CSMDoc::Operation::operationDone()
{
    mTimer->stop();
    emit done (mType, mError);
}
