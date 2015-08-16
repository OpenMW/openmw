
#include "mergeoperation.hpp"

#include "../doc/state.hpp"
#include "../doc/document.hpp"

#include "mergestages.hpp"

CSMTools::MergeOperation::MergeOperation (CSMDoc::Document& document, ToUTF8::FromType encoding)
: CSMDoc::Operation (CSMDoc::State_Merging, true), mState (document)
{
    appendStage (new FinishMergedDocumentStage (mState, encoding));
}

void CSMTools::MergeOperation::setTarget (std::auto_ptr<CSMDoc::Document> document)
{
    mState.mTarget = document;
}

void CSMTools::MergeOperation::operationDone()
{
    CSMDoc::Operation::operationDone();

    if (mState.mCompleted)
        emit mergeDone (mState.mTarget.release());
}
