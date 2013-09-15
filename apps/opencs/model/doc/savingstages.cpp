
#include "savingstages.hpp"

#include <QUndoStack>

#include "document.hpp"
#include "savingstate.hpp"

CSMDoc::FinalSavingStage::FinalSavingStage (Document& document, SavingState& state)
: mDocument (document), mState (state)
{}

int CSMDoc::FinalSavingStage::setup()
{
    return 1;
}

void CSMDoc::FinalSavingStage::perform (int stage, std::vector<std::string>& messages)
{
    if (mState.hasError())
    {
        /// \todo close stream
        /// \todo delete tmp file
    }
    else
    {
        /// \todo delete file, rename tmp file
        mDocument.getUndoStack().setClean();
    }
}