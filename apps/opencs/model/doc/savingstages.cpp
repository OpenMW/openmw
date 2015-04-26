
#include "savingstages.hpp"

#include <fstream>

#include <boost/filesystem.hpp>

#include <QUndoStack>

#include <components/esm/loaddial.hpp>

#include <components/misc/stringops.hpp>

#include "../world/infocollection.hpp"

#include "document.hpp"
#include "savingstate.hpp"

CSMDoc::OpenSaveStage::OpenSaveStage (Document& document, SavingState& state, bool projectFile)
: mDocument (document), mState (state), mProjectFile (projectFile)
{}

int CSMDoc::OpenSaveStage::setup()
{
    return 1;
}

void CSMDoc::OpenSaveStage::perform (int stage, Messages& messages)
{
    mState.start (mDocument, mProjectFile);

    mState.getStream().open (
        mProjectFile ? mState.getPath() : mState.getTmpPath(),
        std::ios::binary);

    if (!mState.getStream().is_open())
        throw std::runtime_error ("failed to open stream for saving");
}


CSMDoc::WriteHeaderStage::WriteHeaderStage (Document& document, SavingState& state, bool simple)
: mDocument (document), mState (state), mSimple (simple)
{}

int CSMDoc::WriteHeaderStage::setup()
{
    return 1;
}

void CSMDoc::WriteHeaderStage::perform (int stage, Messages& messages)
{
    mState.getWriter().setVersion();

    mState.getWriter().clearMaster();

    mState.getWriter().setFormat (0);

    if (mSimple)
    {
        mState.getWriter().setAuthor ("");
        mState.getWriter().setDescription ("");
        mState.getWriter().setRecordCount (0);
    }
    else
    {
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
    }

    mState.getWriter().save (mState.getStream());
}


CSMDoc::WriteDialogueCollectionStage::WriteDialogueCollectionStage (Document& document,
    SavingState& state, bool journal)
: mState (state),
  mTopics (journal ? document.getData().getJournals() : document.getData().getTopics()),
  mInfos (journal ? document.getData().getJournalInfos() : document.getData().getTopicInfos())
{}

int CSMDoc::WriteDialogueCollectionStage::setup()
{
    return mTopics.getSize();
}

void CSMDoc::WriteDialogueCollectionStage::perform (int stage, Messages& messages)
{
    const CSMWorld::Record<ESM::Dialogue>& topic = mTopics.getRecord (stage);

    CSMWorld::RecordBase::State state = topic.mState;

    if (state==CSMWorld::RecordBase::State_Deleted)
    {
        // if the topic is deleted, we do not need to bother with INFO records.

        /// \todo wrote record with delete flag

        return;
    }

    // Test, if we need to save anything associated info records.
    bool infoModified = false;

    CSMWorld::InfoCollection::Range range = mInfos.getTopicRange (topic.get().mId);

    for (CSMWorld::InfoCollection::RecordConstIterator iter (range.first); iter!=range.second; ++iter)
    {
        CSMWorld::RecordBase::State state = iter->mState;

        if (state==CSMWorld::RecordBase::State_Modified ||
            state==CSMWorld::RecordBase::State_ModifiedOnly ||
            state==CSMWorld::RecordBase::State_Deleted)
        {
            infoModified = true;
            break;
        }
    }

    if (state==CSMWorld::RecordBase::State_Modified ||
        state==CSMWorld::RecordBase::State_ModifiedOnly ||
        infoModified)
    {
        mState.getWriter().startRecord (topic.mModified.sRecordId);
        mState.getWriter().writeHNCString ("NAME", topic.mModified.mId);
        topic.mModified.save (mState.getWriter());
        mState.getWriter().endRecord (topic.mModified.sRecordId);

        // write modified selected info records
        for (CSMWorld::InfoCollection::RecordConstIterator iter (range.first); iter!=range.second;
             ++iter)
        {
            CSMWorld::RecordBase::State state = iter->mState;

            if (state==CSMWorld::RecordBase::State_Deleted)
            {
                /// \todo wrote record with delete flag
            }
            else if (state==CSMWorld::RecordBase::State_Modified ||
                state==CSMWorld::RecordBase::State_ModifiedOnly)
            {
                ESM::DialInfo info = iter->get();
                info.mId = info.mId.substr (info.mId.find_last_of ('#')+1);

                if (iter!=range.first)
                {
                    CSMWorld::InfoCollection::RecordConstIterator prev = iter;
                    --prev;

                    info.mPrev =
                        prev->mModified.mId.substr (prev->mModified.mId.find_last_of ('#')+1);
                }

                CSMWorld::InfoCollection::RecordConstIterator next = iter;
                ++next;

                if (next!=range.second)
                {
                    info.mNext =
                        next->mModified.mId.substr (next->mModified.mId.find_last_of ('#')+1);
                }

                mState.getWriter().startRecord (info.sRecordId);
                mState.getWriter().writeHNCString ("INAM", info.mId);
                info.save (mState.getWriter());
                mState.getWriter().endRecord (info.sRecordId);
            }
        }
    }
}


CSMDoc::WriteRefIdCollectionStage::WriteRefIdCollectionStage (Document& document, SavingState& state)
: mDocument (document), mState (state)
{}

int CSMDoc::WriteRefIdCollectionStage::setup()
{
    return mDocument.getData().getReferenceables().getSize();
}

void CSMDoc::WriteRefIdCollectionStage::perform (int stage, Messages& messages)
{
    mDocument.getData().getReferenceables().save (stage, mState.getWriter());
}


CSMDoc::CollectionReferencesStage::CollectionReferencesStage (Document& document,
    SavingState& state)
: mDocument (document), mState (state)
{}

int CSMDoc::CollectionReferencesStage::setup()
{
    mState.getSubRecords().clear();

    int size = mDocument.getData().getReferences().getSize();

    int steps = size/100;
    if (size%100) ++steps;

    return steps;
}

void CSMDoc::CollectionReferencesStage::perform (int stage, Messages& messages)
{
    int size = mDocument.getData().getReferences().getSize();

    for (int i=stage*100; i<stage*100+100 && i<size; ++i)
    {
        const CSMWorld::Record<CSMWorld::CellRef>& record =
            mDocument.getData().getReferences().getRecord (i);

        if (record.mState==CSMWorld::RecordBase::State_Deleted ||
            record.mState==CSMWorld::RecordBase::State_Modified ||
            record.mState==CSMWorld::RecordBase::State_ModifiedOnly)
        {
            std::string cellId = record.get().mOriginalCell.empty() ?
                record.get().mCell : record.get().mOriginalCell;

            std::deque<int>& indices =
                mState.getSubRecords()[Misc::StringUtils::lowerCase (cellId)];

            // collect moved references at the end of the container
            bool interior = cellId.substr (0, 1)!="#";
            std::ostringstream stream;
            if (!interior)
            {
                // recalculate the ref's cell location
                std::pair<int, int> index = record.get().getCellIndex();
                stream << "#" << index.first << " " << index.second;
            }

            // An empty mOriginalCell is meant to indicate that it is the same as
            // the current cell.  It is possible that a moved ref is moved again.
            if ((record.get().mOriginalCell.empty() ?
                    record.get().mCell : record.get().mOriginalCell) != stream.str() && !interior)
                indices.push_back (i);
            else
                indices.push_front (i);
        }
    }
}


CSMDoc::WriteCellCollectionStage::WriteCellCollectionStage (Document& document,
    SavingState& state)
: mDocument (document), mState (state)
{}

int CSMDoc::WriteCellCollectionStage::setup()
{
    return mDocument.getData().getCells().getSize();
}

void CSMDoc::WriteCellCollectionStage::perform (int stage, Messages& messages)
{
    const CSMWorld::Record<CSMWorld::Cell>& cell =
        mDocument.getData().getCells().getRecord (stage);

    std::map<std::string, std::deque<int> >::const_iterator references =
        mState.getSubRecords().find (Misc::StringUtils::lowerCase (cell.get().mId));

    if (cell.mState==CSMWorld::RecordBase::State_Modified ||
        cell.mState==CSMWorld::RecordBase::State_ModifiedOnly ||
        references!=mState.getSubRecords().end())
    {
        bool interior = cell.get().mId.substr (0, 1)!="#";

        // write cell data
        mState.getWriter().startRecord (cell.mModified.sRecordId);

        mState.getWriter().writeHNOCString ("NAME", cell.get().mName);

        ESM::Cell cell2 = cell.get();

        if (interior)
            cell2.mData.mFlags |= ESM::Cell::Interior;
        else
        {
            cell2.mData.mFlags &= ~ESM::Cell::Interior;

            std::istringstream stream (cell.get().mId.c_str());
            char ignore;
            stream >> ignore >> cell2.mData.mX >> cell2.mData.mY;
        }
        cell2.save (mState.getWriter());

        // write references
        if (references!=mState.getSubRecords().end())
        {
            for (std::deque<int>::const_iterator iter (references->second.begin());
                iter!=references->second.end(); ++iter)
            {
                const CSMWorld::Record<CSMWorld::CellRef>& ref =
                    mDocument.getData().getReferences().getRecord (*iter);

                if (ref.mState==CSMWorld::RecordBase::State_Modified ||
                    ref.mState==CSMWorld::RecordBase::State_ModifiedOnly)
                {
                    // recalculate the ref's cell location
                    std::ostringstream stream;
                    if (!interior)
                    {
                        std::pair<int, int> index = ref.get().getCellIndex();
                        stream << "#" << index.first << " " << index.second;
                    }

                    // An empty mOriginalCell is meant to indicate that it is the same as
                    // the current cell.  It is possible that a moved ref is moved again.
                    if ((ref.get().mOriginalCell.empty() ? ref.get().mCell : ref.get().mOriginalCell)
                            != stream.str() && !interior)
                    {
                        ESM::MovedCellRef moved;
                        moved.mRefNum = ref.get().mRefNum;

                        // Need to fill mTarget with the ref's new position.
                        std::istringstream istream (stream.str().c_str());

                        char ignore;
                        istream >> ignore >> moved.mTarget[0] >> moved.mTarget[1];

                        ref.get().mRefNum.save (mState.getWriter(), false, "MVRF");
                        mState.getWriter().writeHNT ("CNDT", moved.mTarget, 8);
                    }

                    ref.get().save (mState.getWriter());
                }
                else if (ref.mState==CSMWorld::RecordBase::State_Deleted)
                {
                    /// \todo write record with delete flag
                }
            }
        }

        mState.getWriter().endRecord (cell.mModified.sRecordId);
    }
    else if (cell.mState==CSMWorld::RecordBase::State_Deleted)
    {
        /// \todo write record with delete flag
    }
}


CSMDoc::WritePathgridCollectionStage::WritePathgridCollectionStage (Document& document,
    SavingState& state)
: mDocument (document), mState (state)
{}

int CSMDoc::WritePathgridCollectionStage::setup()
{
    return mDocument.getData().getPathgrids().getSize();
}

void CSMDoc::WritePathgridCollectionStage::perform (int stage, Messages& messages)
{
    const CSMWorld::Record<CSMWorld::Pathgrid>& pathgrid =
        mDocument.getData().getPathgrids().getRecord (stage);

    if (pathgrid.mState==CSMWorld::RecordBase::State_Modified ||
        pathgrid.mState==CSMWorld::RecordBase::State_ModifiedOnly)
    {
        CSMWorld::Pathgrid record = pathgrid.get();

        if (record.mId.substr (0, 1)=="#")
        {
            std::istringstream stream (record.mId.c_str());
            char ignore;
            stream >> ignore >> record.mData.mX >> record.mData.mY;
        }
        else
            record.mCell = record.mId;

        mState.getWriter().startRecord (record.sRecordId);

        record.save (mState.getWriter());

        mState.getWriter().endRecord (record.sRecordId);
    }
    else if (pathgrid.mState==CSMWorld::RecordBase::State_Deleted)
    {
        /// \todo write record with delete flag
    }
}


CSMDoc::WriteLandCollectionStage::WriteLandCollectionStage (Document& document,
    SavingState& state)
: mDocument (document), mState (state)
{}

int CSMDoc::WriteLandCollectionStage::setup()
{
    return mDocument.getData().getLand().getSize();
}

void CSMDoc::WriteLandCollectionStage::perform (int stage, Messages& messages)
{
    const CSMWorld::Record<CSMWorld::Land>& land =
        mDocument.getData().getLand().getRecord (stage);

    if (land.mState==CSMWorld::RecordBase::State_Modified ||
        land.mState==CSMWorld::RecordBase::State_ModifiedOnly)
    {
        CSMWorld::Land record = land.get();

        mState.getWriter().startRecord (record.mLand->sRecordId);

        record.mLand->save (mState.getWriter());
        if(record.mLand->mLandData)
            record.mLand->mLandData->save (mState.getWriter());

        mState.getWriter().endRecord (record.mLand->sRecordId);
    }
    else if (land.mState==CSMWorld::RecordBase::State_Deleted)
    {
        /// \todo write record with delete flag
    }
}


CSMDoc::WriteLandTextureCollectionStage::WriteLandTextureCollectionStage (Document& document,
    SavingState& state)
: mDocument (document), mState (state)
{}

int CSMDoc::WriteLandTextureCollectionStage::setup()
{
    return mDocument.getData().getLandTextures().getSize();
}

void CSMDoc::WriteLandTextureCollectionStage::perform (int stage, Messages& messages)
{
    const CSMWorld::Record<CSMWorld::LandTexture>& landTexture =
        mDocument.getData().getLandTextures().getRecord (stage);

    if (landTexture.mState==CSMWorld::RecordBase::State_Modified ||
        landTexture.mState==CSMWorld::RecordBase::State_ModifiedOnly)
    {
        CSMWorld::LandTexture record = landTexture.get();

        mState.getWriter().startRecord (record.sRecordId);

        record.save (mState.getWriter());

        mState.getWriter().endRecord (record.sRecordId);
    }
    else if (landTexture.mState==CSMWorld::RecordBase::State_Deleted)
    {
        /// \todo write record with delete flag
    }
}


CSMDoc::CloseSaveStage::CloseSaveStage (SavingState& state)
: mState (state)
{}

int CSMDoc::CloseSaveStage::setup()
{
    return 1;
}

void CSMDoc::CloseSaveStage::perform (int stage, Messages& messages)
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

void CSMDoc::FinalSavingStage::perform (int stage, Messages& messages)
{
    if (mState.hasError())
    {
        mState.getWriter().close();
        mState.getStream().close();

        if (boost::filesystem::exists (mState.getTmpPath()))
            boost::filesystem::remove (mState.getTmpPath());
    }
    else if (!mState.isProjectFile())
    {
        if (boost::filesystem::exists (mState.getPath()))
            boost::filesystem::remove (mState.getPath());

        boost::filesystem::rename (mState.getTmpPath(), mState.getPath());

        mDocument.getUndoStack().setClean();
    }
}
