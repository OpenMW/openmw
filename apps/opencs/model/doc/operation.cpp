
#include "operation.hpp"

#include <string>
#include <vector>

#include <QTimer>

#include "../world/universalid.hpp"

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
    }
}

CSMDoc::Operation::Operation (int type, bool ordered, bool finalAlways)
: mType (type), mOrdered (ordered), mFinalAlways (finalAlways)
{
    connect (this, SIGNAL (finished()), this, SLOT (operationDone()));
}

CSMDoc::Operation::~Operation()
{
    for (std::vector<std::pair<Stage *, int> >::iterator iter (mStages.begin()); iter!=mStages.end(); ++iter)
        delete iter->first;
}

void CSMDoc::Operation::run()
{
    prepareStages();

    QTimer timer;

    timer.connect (&timer, SIGNAL (timeout()), this, SLOT (executeStage()));

    timer.start (0);

    exec();
}

void CSMDoc::Operation::appendStage (Stage *stage)
{
    mStages.push_back (std::make_pair (stage, 0));
}

bool CSMDoc::Operation::hasError() const
{
    return mError;
}

void CSMDoc::Operation::abort()
{
    if (!isRunning())
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

void CSMDoc::Operation::executeStage()
{
    Stage::Messages messages;

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
                emit reportMessage (CSMWorld::UniversalId(), e.what(), mType);
                abort();
            }

            ++mCurrentStepTotal;
            break;
        }
    }

    emit progress (mCurrentStepTotal, mTotalSteps ? mTotalSteps : 1, mType);

    for (Stage::Messages::const_iterator iter (messages.begin()); iter!=messages.end(); ++iter)
        emit reportMessage (iter->first, iter->second, mType);

    if (mCurrentStage==mStages.end())
        exit();
}

void CSMDoc::Operation::operationDone()
{
    emit done (mType);
}