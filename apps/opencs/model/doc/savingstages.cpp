
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
: mDocument (document), mState (state),
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


CSMDoc::WriteFilterStage::WriteFilterStage (Document& document, SavingState& state,
    CSMFilter::Filter::Scope scope)
: WriteCollectionStage<CSMWorld::IdCollection<CSMFilter::Filter> > (document.getData().getFilters(),
  state),
  mDocument (document), mScope (scope)
{}

void CSMDoc::WriteFilterStage::perform (int stage, Messages& messages)
{
    const CSMWorld::Record<CSMFilter::Filter>& record =
        mDocument.getData().getFilters().getRecord (stage);

    if (record.get().mScope==mScope)
        WriteCollectionStage<CSMWorld::IdCollection<CSMFilter::Filter> >::perform (stage, messages);
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
            mState.getSubRecords()[Misc::StringUtils::lowerCase (record.get().mCell)]
                .push_back (i);
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

    std::map<std::string, std::vector<int> >::const_iterator references =
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
            // first pass: find highest RefNum
            int lastRefNum = -1;

            for (std::vector<int>::const_iterator iter (references->second.begin());
                iter!=references->second.end(); ++iter)
            {
                const CSMWorld::Record<CSMWorld::CellRef>& ref =
                    mDocument.getData().getReferences().getRecord (*iter);

                if (ref.get().mRefNum.mContentFile==0 && ref.get().mRefNum.mIndex>lastRefNum)
                    lastRefNum = ref.get().mRefNum.mIndex;
            }

            // second pass: write
            for (std::vector<int>::const_iterator iter (references->second.begin());
                iter!=references->second.end(); ++iter)
            {
                const CSMWorld::Record<CSMWorld::CellRef>& ref =
                    mDocument.getData().getReferences().getRecord (*iter);

                if (ref.mState==CSMWorld::RecordBase::State_Modified ||
                    ref.mState==CSMWorld::RecordBase::State_ModifiedOnly)
                {
                    if (ref.get().mRefNum.mContentFile==-2)
                    {
                        if (lastRefNum>=0xffffff)
                            throw std::runtime_error (
                                "RefNums exhausted in cell: " + cell.get().mId);

                        ESM::CellRef ref2 = ref.get();
                        ref2.mRefNum.mContentFile = 0;
                        ref2.mRefNum.mIndex = ++lastRefNum;

                        ref2.save (mState.getWriter());
                    }
                    else
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
