#include "mergestages.hpp"

#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

#include <apps/opencs/model/world/land.hpp>
#include <apps/opencs/model/world/landtexture.hpp>
#include <apps/opencs/model/world/metadata.hpp>
#include <apps/opencs/model/world/ref.hpp>
#include <apps/opencs/model/world/refcollection.hpp>
#include <apps/opencs/model/world/refidcollection.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm3/cellref.hpp>
#include <components/esm3/esmreader.hpp>

#include "mergestate.hpp"

#include "../doc/document.hpp"
#include "../world/commands.hpp"
#include "../world/data.hpp"
#include "../world/idtable.hpp"

namespace CSMDoc
{
    class Messages;
}

CSMTools::StartMergeStage::StartMergeStage(MergeState& state)
    : mState(state)
{
}

int CSMTools::StartMergeStage::setup()
{
    return 1;
}

void CSMTools::StartMergeStage::perform(int stage, CSMDoc::Messages& messages)
{
    mState.mCompleted = false;
    mState.mTextureIndices.clear();
}

CSMTools::FinishMergedDocumentStage::FinishMergedDocumentStage(MergeState& state, ToUTF8::FromType encoding)
    : mState(state)
    , mEncoder(encoding)
{
}

int CSMTools::FinishMergedDocumentStage::setup()
{
    return 1;
}

void CSMTools::FinishMergedDocumentStage::perform(int stage, CSMDoc::Messages& messages)
{
    // We know that the content file list contains at least two entries and that the first one
    // does exist on disc (otherwise it would have been impossible to initiate a merge on that
    // document).
    std::filesystem::path path = mState.mSource.getContentFiles()[0];

    ESM::ESMReader reader;
    reader.setEncoder(&mEncoder);
    reader.open(path);

    CSMWorld::MetaData source;
    source.mId = ESM::RefId::stringRefId("sys::meta");
    source.load(reader);

    CSMWorld::MetaData target = mState.mTarget->getData().getMetaData();

    target.mAuthor = source.mAuthor;
    target.mDescription = source.mDescription;

    mState.mTarget->getData().setMetaData(target);

    mState.mCompleted = true;
}

CSMTools::MergeRefIdsStage::MergeRefIdsStage(MergeState& state)
    : mState(state)
{
}

int CSMTools::MergeRefIdsStage::setup()
{
    return mState.mSource.getData().getReferenceables().getSize();
}

void CSMTools::MergeRefIdsStage::perform(int stage, CSMDoc::Messages& messages)
{
    mState.mSource.getData().getReferenceables().copyTo(stage, mState.mTarget->getData().getReferenceables());
}

CSMTools::MergeReferencesStage::MergeReferencesStage(MergeState& state)
    : mState(state)
{
}

int CSMTools::MergeReferencesStage::setup()
{
    mIndex.clear();
    return mState.mSource.getData().getReferences().getSize();
}

void CSMTools::MergeReferencesStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<CSMWorld::CellRef>& record = mState.mSource.getData().getReferences().getRecord(stage);

    if (!record.isDeleted())
    {
        CSMWorld::CellRef ref = record.get();

        ref.mOriginalCell = ref.mCell;

        ref.mRefNum.mIndex = mIndex[ref.mCell]++;
        ref.mRefNum.mContentFile = -1;
        ref.mNew = false;

        mState.mTarget->getData().getReferences().appendRecord(std::make_unique<CSMWorld::Record<CSMWorld::CellRef>>(
            CSMWorld::Record<CSMWorld::CellRef>(CSMWorld::RecordBase::State_ModifiedOnly, nullptr, &ref)));
    }
}

CSMTools::PopulateLandTexturesMergeStage::PopulateLandTexturesMergeStage(MergeState& state)
    : mState(state)
{
}

int CSMTools::PopulateLandTexturesMergeStage::setup()
{
    return mState.mSource.getData().getLandTextures().getSize();
}

void CSMTools::PopulateLandTexturesMergeStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<CSMWorld::LandTexture>& record = mState.mSource.getData().getLandTextures().getRecord(stage);

    if (!record.isDeleted())
    {
        mState.mTarget->getData().getLandTextures().appendRecord(
            std::make_unique<CSMWorld::Record<CSMWorld::LandTexture>>(CSMWorld::Record<CSMWorld::LandTexture>(
                CSMWorld::RecordBase::State_ModifiedOnly, nullptr, &record.get())));
    }
}

CSMTools::MergeLandStage::MergeLandStage(MergeState& state)
    : mState(state)
{
}

int CSMTools::MergeLandStage::setup()
{
    return mState.mSource.getData().getLand().getSize();
}

void CSMTools::MergeLandStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<CSMWorld::Land>& record = mState.mSource.getData().getLand().getRecord(stage);

    if (!record.isDeleted())
    {
        mState.mTarget->getData().getLand().appendRecord(std::make_unique<CSMWorld::Record<CSMWorld::Land>>(
            CSMWorld::Record<CSMWorld::Land>(CSMWorld::RecordBase::State_ModifiedOnly, nullptr, &record.get())));
    }
}

CSMTools::FixLandsAndLandTexturesMergeStage::FixLandsAndLandTexturesMergeStage(MergeState& state)
    : mState(state)
{
}

int CSMTools::FixLandsAndLandTexturesMergeStage::setup()
{
    // We will have no more than the source
    return mState.mSource.getData().getLand().getSize();
}

void CSMTools::FixLandsAndLandTexturesMergeStage::perform(int stage, CSMDoc::Messages& messages)
{
    if (stage < mState.mTarget->getData().getLand().getSize())
    {
        CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&>(
            *mState.mTarget->getData().getTableModel(CSMWorld::UniversalId::Type_Lands));

        CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&>(
            *mState.mTarget->getData().getTableModel(CSMWorld::UniversalId::Type_LandTextures));

        const auto& id = mState.mTarget->getData().getLand().getId(stage);

        CSMWorld::TouchLandCommand cmd(landTable, ltexTable, id.getRefIdString());
        cmd.redo();

        // Get rid of base data
        const CSMWorld::Record<CSMWorld::Land>& oldRecord = mState.mTarget->getData().getLand().getRecord(stage);

        mState.mTarget->getData().getLand().setRecord(stage,
            std::make_unique<CSMWorld::Record<CSMWorld::Land>>(
                CSMWorld::Record<CSMWorld::Land>(CSMWorld::RecordBase::State_ModifiedOnly, nullptr, &oldRecord.get())));
    }
}

CSMTools::CleanupLandTexturesMergeStage::CleanupLandTexturesMergeStage(MergeState& state)
    : mState(state)
{
}

int CSMTools::CleanupLandTexturesMergeStage::setup()
{
    return 1;
}

void CSMTools::CleanupLandTexturesMergeStage::perform(int stage, CSMDoc::Messages& messages)
{
    auto& landTextures = mState.mTarget->getData().getLandTextures();
    for (int i = 0; i < landTextures.getSize();)
    {
        if (!landTextures.getRecord(i).isModified())
            landTextures.removeRows(i, 1);
        else
            ++i;
    }
}
