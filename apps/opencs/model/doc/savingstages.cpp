#include "savingstages.hpp"

#include <boost/filesystem.hpp>

#include <QUndoStack>

#include <components/esm/loaddial.hpp>

#include "../world/infocollection.hpp"
#include "../world/cellcoordinates.hpp"

#include "document.hpp"

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

    if (mSimple)
    {
        mState.getWriter().setAuthor ("");
        mState.getWriter().setDescription ("");
        mState.getWriter().setRecordCount (0);
        mState.getWriter().setFormat (ESM::Header::CurrentFormat);
    }
    else
    {
        mDocument.getData().getMetaData().save (mState.getWriter());
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
    ESM::ESMWriter& writer = mState.getWriter();
    const CSMWorld::Record<ESM::Dialogue>& topic = mTopics.getRecord (stage);

    if (topic.mState == CSMWorld::RecordBase::State_Deleted)
    {
        // if the topic is deleted, we do not need to bother with INFO records.
        ESM::Dialogue dialogue = topic.get();
        writer.startRecord(dialogue.sRecordId);
        dialogue.save(writer, true);
        writer.endRecord(dialogue.sRecordId);
        return;
    }

    // Test, if we need to save anything associated info records.
    bool infoModified = false;
    CSMWorld::InfoCollection::Range range = mInfos.getTopicRange (topic.get().mId);

    for (CSMWorld::InfoCollection::RecordConstIterator iter (range.first); iter!=range.second; ++iter)
    {
        if (iter->isModified() || iter->mState == CSMWorld::RecordBase::State_Deleted)
        {
            infoModified = true;
            break;
        }
    }

    if (topic.isModified() || infoModified)
    {
        if (infoModified && topic.mState != CSMWorld::RecordBase::State_Modified
                         && topic.mState != CSMWorld::RecordBase::State_ModifiedOnly)
        {
            mState.getWriter().startRecord (topic.mBase.sRecordId);
            topic.mBase.save (mState.getWriter(), topic.mState == CSMWorld::RecordBase::State_Deleted);
            mState.getWriter().endRecord (topic.mBase.sRecordId);
        }
        else
        {
            mState.getWriter().startRecord (topic.mModified.sRecordId);
            topic.mModified.save (mState.getWriter(), topic.mState == CSMWorld::RecordBase::State_Deleted);
            mState.getWriter().endRecord (topic.mModified.sRecordId);
        }

        // write modified selected info records
        for (CSMWorld::InfoCollection::RecordConstIterator iter (range.first); iter!=range.second; ++iter)
        {
            if (iter->isModified() || iter->mState == CSMWorld::RecordBase::State_Deleted)
            {
                ESM::DialInfo info = iter->get();
                info.mId = info.mId.substr (info.mId.find_last_of ('#')+1);

                info.mPrev = "";
                if (iter!=range.first)
                {
                    CSMWorld::InfoCollection::RecordConstIterator prev = iter;
                    --prev;

                    info.mPrev = prev->get().mId.substr (prev->get().mId.find_last_of ('#')+1);
                }

                CSMWorld::InfoCollection::RecordConstIterator next = iter;
                ++next;

                info.mNext = "";
                if (next!=range.second)
                {
                    info.mNext = next->get().mId.substr (next->get().mId.find_last_of ('#')+1);
                }

                writer.startRecord (info.sRecordId);
                info.save (writer, iter->mState == CSMWorld::RecordBase::State_Deleted);
                writer.endRecord (info.sRecordId);
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

        if (record.isModified() || record.mState == CSMWorld::RecordBase::State_Deleted)
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
                    record.get().mCell : record.get().mOriginalCell) != stream.str() && !interior && record.mState!=CSMWorld::RecordBase::State_ModifiedOnly && !record.get().mNew)
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
    ESM::ESMWriter& writer = mState.getWriter();
    const CSMWorld::Record<CSMWorld::Cell>& cell = mDocument.getData().getCells().getRecord (stage);

    std::map<std::string, std::deque<int> >::const_iterator references =
        mState.getSubRecords().find (Misc::StringUtils::lowerCase (cell.get().mId));

    if (cell.isModified() ||
        cell.mState == CSMWorld::RecordBase::State_Deleted ||
        references!=mState.getSubRecords().end())
    {
        CSMWorld::Cell cellRecord = cell.get();
        bool interior = cellRecord.mId.substr (0, 1)!="#";

        // count new references and adjust RefNumCount accordingsly
        unsigned int newRefNum = cellRecord.mRefNumCounter;

        if (references!=mState.getSubRecords().end())
        {
            for (std::deque<int>::const_iterator iter (references->second.begin());
                iter!=references->second.end(); ++iter)
            {
                const CSMWorld::Record<CSMWorld::CellRef>& ref =
                    mDocument.getData().getReferences().getRecord (*iter);

                CSMWorld::CellRef refRecord = ref.get();

                if (refRecord.mNew ||
                    (!interior && ref.mState==CSMWorld::RecordBase::State_ModifiedOnly &&
                    /// \todo consider worldspace
                    CSMWorld::CellCoordinates (refRecord.getCellIndex()).getId("") != refRecord.mCell))
                    ++cellRecord.mRefNumCounter;

                if (refRecord.mRefNum.mIndex >= newRefNum)
                    newRefNum = refRecord.mRefNum.mIndex + 1;

            }
        }

        // write cell data
        writer.startRecord (cellRecord.sRecordId);

        if (interior)
            cellRecord.mData.mFlags |= ESM::Cell::Interior;
        else
        {
            cellRecord.mData.mFlags &= ~ESM::Cell::Interior;

            std::istringstream stream (cellRecord.mId.c_str());
            char ignore;
            stream >> ignore >> cellRecord.mData.mX >> cellRecord.mData.mY;
        }

        cellRecord.save (writer, cell.mState == CSMWorld::RecordBase::State_Deleted);

        // write references
        if (references!=mState.getSubRecords().end())
        {
            for (std::deque<int>::const_iterator iter (references->second.begin());
                iter!=references->second.end(); ++iter)
            {
                const CSMWorld::Record<CSMWorld::CellRef>& ref =
                    mDocument.getData().getReferences().getRecord (*iter);

                if (ref.isModified() || ref.mState == CSMWorld::RecordBase::State_Deleted)
                {
                    CSMWorld::CellRef refRecord = ref.get();

                    // Check for uninitialized content file
                    if (!refRecord.mRefNum.hasContentFile())
                        refRecord.mRefNum.mContentFile = 0;

                    // recalculate the ref's cell location
                    std::ostringstream stream;
                    if (!interior)
                    {
                        std::pair<int, int> index = refRecord.getCellIndex();
                        stream << "#" << index.first << " " << index.second;
                    }

                    if (refRecord.mNew || refRecord.mRefNum.mIndex == 0 ||
                        (!interior && ref.mState==CSMWorld::RecordBase::State_ModifiedOnly &&
                        refRecord.mCell!=stream.str()))
                    {
                        refRecord.mRefNum.mIndex = newRefNum++;
                    }
                    else if ((refRecord.mOriginalCell.empty() ? refRecord.mCell : refRecord.mOriginalCell)
                            != stream.str() && !interior)
                    {
                        // An empty mOriginalCell is meant to indicate that it is the same as
                        // the current cell.  It is possible that a moved ref is moved again.

                        ESM::MovedCellRef moved;
                        moved.mRefNum = refRecord.mRefNum;

                        // Need to fill mTarget with the ref's new position.
                        std::istringstream istream (stream.str().c_str());

                        char ignore;
                        istream >> ignore >> moved.mTarget[0] >> moved.mTarget[1];

                        refRecord.mRefNum.save (writer, false, "MVRF");
                        writer.writeHNT ("CNDT", moved.mTarget);
                    }

                    refRecord.save (writer, false, false, ref.mState == CSMWorld::RecordBase::State_Deleted);
                }
            }
        }

        writer.endRecord (cellRecord.sRecordId);
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
    ESM::ESMWriter& writer = mState.getWriter();
    const CSMWorld::Record<CSMWorld::Pathgrid>& pathgrid =
        mDocument.getData().getPathgrids().getRecord (stage);

    if (pathgrid.isModified() || pathgrid.mState == CSMWorld::RecordBase::State_Deleted)
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

        writer.startRecord (record.sRecordId);
        record.save (writer, pathgrid.mState == CSMWorld::RecordBase::State_Deleted);
        writer.endRecord (record.sRecordId);
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
    ESM::ESMWriter& writer = mState.getWriter();
    const CSMWorld::Record<CSMWorld::Land>& land =
        mDocument.getData().getLand().getRecord (stage);

    if (land.isModified() || land.mState == CSMWorld::RecordBase::State_Deleted)
    {
        CSMWorld::Land record = land.get();
        writer.startRecord (record.sRecordId);
        record.save (writer, land.mState == CSMWorld::RecordBase::State_Deleted);
        writer.endRecord (record.sRecordId);
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
    ESM::ESMWriter& writer = mState.getWriter();
    const CSMWorld::Record<CSMWorld::LandTexture>& landTexture =
        mDocument.getData().getLandTextures().getRecord (stage);

    if (landTexture.isModified() || landTexture.mState == CSMWorld::RecordBase::State_Deleted)
    {
        CSMWorld::LandTexture record = landTexture.get();
        writer.startRecord (record.sRecordId);
        record.save (writer, landTexture.mState == CSMWorld::RecordBase::State_Deleted);
        writer.endRecord (record.sRecordId);
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
