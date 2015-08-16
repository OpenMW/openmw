
#include "mergestages.hpp"

#include "mergestate.hpp"

CSMTools::FinishMergedDocumentStage::FinishMergedDocumentStage (MergeState& state)
: mState (state)
{}

int CSMTools::FinishMergedDocumentStage::setup()
{
    return 1;
}

void CSMTools::FinishMergedDocumentStage::perform (int stage, CSMDoc::Messages& messages)
{
    mState.mCompleted = true;
}
