
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
: mType (type), mStages(std::vector<std::pair<Stage *, int> >()), mCurrentStage(mStages.begin()),
  mCurrentStep(0), mCurrentStepTotal(0), mTotalSteps(0), mOrdered (ordered),
  mFinalAlways (finalAlways), mError(false), mConnected (false)
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
    
    prepareStages();

    mTimer->start (0);
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

void CSMDoc::Operation::executeStage()
{
    Messages messages;

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
                emit reportMessage (CSMWorld::UniversalId(), e.what(), "", mType);
                abort();
            }

            ++mCurrentStepTotal;
            break;
        }
    }

    emit progress (mCurrentStepTotal, mTotalSteps ? mTotalSteps : 1, mType);

    for (Messages::Iterator iter (messages.begin()); iter!=messages.end(); ++iter)
        emit reportMessage (iter->mId, iter->mMessage, iter->mHint, mType);

    if (mCurrentStage==mStages.end())
        operationDone();
}

void CSMDoc::Operation::operationDone()
{
    mTimer->stop();
    emit done (mType, mError);
}
