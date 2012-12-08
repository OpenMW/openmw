
#include "operation.hpp"

#include <QTimer>

#include "../doc/state.hpp"

#include "stage.hpp"

void CSMTools::Operation::prepareStages()
{
    mCurrentStage = mStages.begin();
    mCurrentStep = 0;
    mCurrentStepTotal = 0;
    mTotalSteps = 0;

    for (std::vector<std::pair<Stage *, int> >::iterator iter (mStages.begin()); iter!=mStages.end(); ++iter)
    {
        iter->second = mTotalSteps;
        mTotalSteps += iter->first->setup();
    }
}

CSMTools::Operation::Operation (int type) : mType (type) {}

CSMTools::Operation::~Operation()
{
    for (std::vector<std::pair<Stage *, int> >::iterator iter (mStages.begin()); iter!=mStages.end(); ++iter)
        delete iter->first;
}

void CSMTools::Operation::run()
{
    prepareStages();

    QTimer timer;

    timer.connect (&timer, SIGNAL (timeout()), this, SLOT (verify()));

    timer.start (0);

    exec();
}

void CSMTools::Operation::appendStage (Stage *stage)
{
    mStages.push_back (std::make_pair (stage, 0));
}

void CSMTools::Operation::abort()
{
    exit();
}

void CSMTools::Operation::verify()
{
    while (mCurrentStage!=mStages.end())
    {
        if (mCurrentStep>=mCurrentStage->second)
        {
            mCurrentStep = 0;
            ++mCurrentStage;
        }
        else
        {
            mCurrentStage->first->perform (mCurrentStep++);
            ++mCurrentStepTotal;
            break;
        }
    }

    emit progress (mCurrentStepTotal, mTotalSteps ? mTotalSteps : 1, mType);

    if (mCurrentStage==mStages.end())
        exit();
}