
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


CSMDoc::WriteHeaderStage::WriteHeaderStage (Document& document, SavingState& state)
: mDocument (document), mState (state)
{}

int CSMDoc::WriteHeaderStage::setup()
{
    return 1;
}

void CSMDoc::WriteHeaderStage::perform (int stage, std::vector<std::string>& messages)
{
    mState.getWriter().setVersion();

    mState.getWriter().setFormat (0);

    mState.getWriter().setAuthor (mDocument.getData().getAuthor());
    mState.getWriter().setDescription (mDocument.getData().getDescription());
    mState.getWriter().setRecordCount (
        mDocument.getData().count (CSMWorld::RecordBase::State_Modified) +
        mDocument.getData().count (CSMWorld::RecordBase::State_ModifiedOnly) +
        mDocument.getData().count (CSMWorld::RecordBase::State_Deleted));

    /// \todo refine dependency list (at least remove redundant dependencies)
    std::vector<boost::filesystem::path> dependencies = mDocument.getContentFiles();
    std::vector<boost::filesystem::path>::const_iterator end (--dependencies.end());

    for (std::vector<boost::filesystem::path>::const_iterator iter (dependencies.begin());
        iter!=end; ++iter)
    {
        std::string name = iter->filename().string();
        uint64_t size = boost::filesystem::file_size (*iter);

        mState.getWriter().addMaster (name, size);
    }

    mState.getWriter().save (mState.getStream());
}


CSMDoc::WriteRefIdCollectionStage::WriteRefIdCollectionStage (Document& document, SavingState& state)
: mDocument (document), mState (state)
{}

int CSMDoc::WriteRefIdCollectionStage::setup()
{
    return mDocument.getData().getReferenceables().getSize();
}

void CSMDoc::WriteRefIdCollectionStage::perform (int stage, std::vector<std::string>& messages)
{
    mDocument.getData().getReferenceables().save (stage, mState.getWriter());
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