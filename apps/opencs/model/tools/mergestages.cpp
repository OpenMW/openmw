
#include "mergestages.hpp"

#include <components/misc/stringops.hpp>

#include "mergestate.hpp"

#include "../doc/document.hpp"
#include "../world/data.hpp"


CSMTools::StartMergeStage::StartMergeStage (MergeState& state)
: mState (state)
{}

int CSMTools::StartMergeStage::setup()
{
    return 1;
}

void CSMTools::StartMergeStage::perform (int stage, CSMDoc::Messages& messages)
{
    mState.mCompleted = false;
    mState.mTextureIndices.clear();
}


CSMTools::FinishMergedDocumentStage::FinishMergedDocumentStage (MergeState& state, ToUTF8::FromType encoding)
: mState (state), mEncoder (encoding)
{}

int CSMTools::FinishMergedDocumentStage::setup()
{
    return 1;
}

void CSMTools::FinishMergedDocumentStage::perform (int stage, CSMDoc::Messages& messages)
{
    // We know that the content file list contains at least two entries and that the first one
    // does exist on disc (otherwise it would have been impossible to initiate a merge on that
    // document).
    boost::filesystem::path path = mState.mSource.getContentFiles()[0];

    ESM::ESMReader reader;
    reader.setEncoder (&mEncoder);
    reader.open (path.string());

    CSMWorld::MetaData source;
    source.mId = "sys::meta";
    source.load (reader);

    CSMWorld::MetaData target = mState.mTarget->getData().getMetaData();

    target.mAuthor = source.mAuthor;
    target.mDescription = source.mDescription;

    mState.mTarget->getData().setMetaData (target);

    mState.mCompleted = true;
}


CSMTools::MergeRefIdsStage::MergeRefIdsStage (MergeState& state) : mState (state) {}

int CSMTools::MergeRefIdsStage::setup()
{
    return mState.mSource.getData().getReferenceables().getSize();
}

void CSMTools::MergeRefIdsStage::perform (int stage, CSMDoc::Messages& messages)
{
    mState.mSource.getData().getReferenceables().copyTo (
        stage, mState.mTarget->getData().getReferenceables());
}


CSMTools::MergeReferencesStage::MergeReferencesStage (MergeState& state)
: mState (state)
{}

int CSMTools::MergeReferencesStage::setup()
{
    mIndex.clear();
    return mState.mSource.getData().getReferences().getSize();
}

void CSMTools::MergeReferencesStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<CSMWorld::CellRef>& record =
        mState.mSource.getData().getReferences().getRecord (stage);

    if (!record.isDeleted())
    {
        CSMWorld::CellRef ref = record.get();

        ref.mOriginalCell = ref.mCell;

        ref.mRefNum.mIndex = mIndex[Misc::StringUtils::lowerCase (ref.mCell)]++;
        ref.mRefNum.mContentFile = 0;

        CSMWorld::Record<CSMWorld::CellRef> newRecord (
            CSMWorld::RecordBase::State_ModifiedOnly, 0, &ref);

        mState.mTarget->getData().getReferences().appendRecord (newRecord);
    }
}
