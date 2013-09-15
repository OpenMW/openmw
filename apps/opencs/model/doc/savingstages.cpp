
#include "savingstages.hpp"

#include <fstream>

#include <boost/filesystem.hpp>

#include <QUndoStack>

#include "document.hpp"
#include "savingstate.hpp"

CSMDoc::OpenSaveStage::OpenSaveStage (Document& document, SavingState& state)
: mDocument (document), mState (state)
{}

int CSMDoc::OpenSaveStage::setup()
{
    return 1;
}

void CSMDoc::OpenSaveStage::perform (int stage, std::vector<std::string>& messages)
{
    mState.start (mDocument);

    mState.getStream().open (mState.getTmpPath().string().c_str());

    if (!mState.getStream().is_open())
        throw std::runtime_error ("failed to open stream for saving");
}


CSMDoc::CloseSaveStage::CloseSaveStage (SavingState& state)
: mState (state)
{}

int CSMDoc::CloseSaveStage::setup()
{
    return 1;
}

void CSMDoc::CloseSaveStage::perform (int stage, std::vector<std::string>& messages)
{
    mState.getStream().close();

    if (!mState.getStream())
        throw std::runtime_error ("saving failed");
}


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
        mState.getWriter().close();
        mState.getStream().close();

        if (boost::filesystem::exists (mState.getTmpPath()))
            boost::filesystem::remove (mState.getTmpPath());
    }
    else
    {
        if (boost::filesystem::exists (mState.getPath()))
            boost::filesystem::remove (mState.getPath());

        boost::filesystem::rename (mState.getTmpPath(), mState.getPath());

        mDocument.getUndoStack().setClean();
    }
}