
#include "operation.hpp"

#include <string>
#include <vector>

#include <QTimer>

#include "state.hpp"
#include "stage.hpp"

void CSMDoc::Operation::prepareStages()
{
    mCurrentStage = mStages.begin();
    mCurrentStep = 0;
    mCurrentStepTotal = 0;
    mTotalSteps = 0;

    for (std::vector<std::pair<Stage *, int> >::iterator iter (mStages.begin()); iter!=mStages.end(); ++iter)
    {
        iter->second = iter->first->setup();
        mTotalSteps += iter->second;
    }
}

CSMDoc::Operation::Operation (int type) : mType (type) {}

CSMDoc::Operation::~Operation()
{
    for (std::vector<std::pair<Stage *, int> >::iterator iter (mStages.begin()); iter!=mStages.end(); ++iter)
        delete iter->first;
}

void CSMDoc::Operation::run()
{
    prepareStages();

    QTimer timer;

    timer.connect (&timer, SIGNAL (timeout()), this, SLOT (verify()));

    timer.start (0);

    exec();
}

void CSMDoc::Operation::appendStage (Stage *stage)
{
    mStages.push_back (std::make_pair (stage, 0));
}

void CSMDoc::Operation::abort()
{
    exit();
}

void CSMDoc::Operation::verify()
{
    std::vector<std::string> messages;

    while (mCurrentStage!=mStages.end())
    {
        if (mCurrentStep>=mCurrentStage->second)
        {
            mCurrentStep = 0;
            ++mCurrentStage;
        }
        else
        {
            mCurrentStage->first->perform (mCurrentStep++, messages);
            ++mCurrentStepTotal;
            break;
        }
    }

    emit progress (mCurrentStepTotal, mTotalSteps ? mTotalSteps : 1, mType);

    for (std::vector<std::string>::const_iterator iter (messages.begin()); iter!=messages.end(); ++iter)
        emit reportMessage (iter->c_str(), mType);

    if (mCurrentStage==mStages.end())
        exit();
}