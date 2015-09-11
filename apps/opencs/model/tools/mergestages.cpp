
#include "mergestages.hpp"

#include <sstream>

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


CSMTools::ListLandTexturesMergeStage::ListLandTexturesMergeStage (MergeState& state)
: mState (state)
{}

int CSMTools::ListLandTexturesMergeStage::setup()
{
    return mState.mSource.getData().getLand().getSize();
}

void CSMTools::ListLandTexturesMergeStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<CSMWorld::Land>& record =
        mState.mSource.getData().getLand().getRecord (stage);

    if (!record.isDeleted())
    {
        const CSMWorld::Land& land = record.get();

        // make sure record is loaded
        land.loadData (ESM::Land::DATA_VHGT | ESM::Land::DATA_VNML |
            ESM::Land::DATA_VCLR | ESM::Land::DATA_VTEX | ESM::Land::DATA_WNAM);

        if (const ESM::Land::LandData *data = land.getLandData (ESM::Land::DATA_VTEX))
        {
            // list texture indices
            std::pair<uint16_t, int> key;
            key.second = land.mPlugin;

            for (int i=0; i<ESM::Land::LAND_NUM_TEXTURES; ++i)
            {
                key.first = data->mTextures[i];

                mState.mTextureIndices[key] = -1;
            }
        }
    }
}


CSMTools::MergeLandTexturesStage::MergeLandTexturesStage (MergeState& state)
: mState (state), mNext (mState.mTextureIndices.end())
{}

int CSMTools::MergeLandTexturesStage::setup()
{
    // Should use the size of mState.mTextureIndices instead, but that is not available at this
    // point. Unless there are any errors in the land and land texture records this will not
    // make a difference.
    return mState.mSource.getData().getLandTextures().getSize();
}

void CSMTools::MergeLandTexturesStage::perform (int stage, CSMDoc::Messages& messages)
{
    if (stage==0)
        mNext = mState.mTextureIndices.begin();

    bool found = false;

    do
    {
        if (mNext==mState.mTextureIndices.end())
            return;

        mNext->second = stage+1;

        std::ostringstream stream;
        stream << mNext->first.first-1 << "_" << mNext->first.second;

        int index = mState.mSource.getData().getLandTextures().searchId (stream.str());

        if (index!=-1)
        {
            CSMWorld::LandTexture texture =
                mState.mSource.getData().getLandTextures().getRecord (index).get();

            std::ostringstream stream;
            stream << mNext->second-1 << "_0";

            texture.mIndex = mNext->second-1;
            texture.mId = stream.str();

            CSMWorld::Record<CSMWorld::LandTexture> newRecord (
                CSMWorld::RecordBase::State_ModifiedOnly, 0, &texture);

            mState.mTarget->getData().getLandTextures().appendRecord (newRecord);

            found = true;
        }

        ++mNext;
    }
    while (!found);
}


CSMTools::MergeLandStage::MergeLandStage (MergeState& state) : mState (state) {}

int CSMTools::MergeLandStage::setup()
{
    return mState.mSource.getData().getLand().getSize();
}

void CSMTools::MergeLandStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<CSMWorld::Land>& record =
        mState.mSource.getData().getLand().getRecord (stage);

    if (!record.isDeleted())
    {
        const CSMWorld::Land& land = record.get();

        land.loadData (ESM::Land::DATA_VCLR | ESM::Land::DATA_VHGT | ESM::Land::DATA_VNML |
            ESM::Land::DATA_VTEX | ESM::Land::DATA_WNAM);

        CSMWorld::Land newLand (land);

        newLand.mEsm = 0; // avoid potential dangling pointer (ESMReader isn't needed anyway,
                          // because record is already fully loaded)
        newLand.mPlugin = 0;

        if (land.mDataTypes & ESM::Land::DATA_VTEX)
        {
            // adjust land texture references
            if (ESM::Land::LandData *data = newLand.getLandData())
            {
                std::pair<uint16_t, int> key;
                key.second = land.mPlugin;

                for (int i=0; i<ESM::Land::LAND_NUM_TEXTURES; ++i)
                {
                    key.first = data->mTextures[i];
                    std::map<std::pair<uint16_t, int>, int>::const_iterator iter =
                        mState.mTextureIndices.find (key);

                    if (iter!=mState.mTextureIndices.end())
                        data->mTextures[i] = iter->second;
                    else
                        data->mTextures[i] = 0;
                }
            }
        }

        CSMWorld::Record<CSMWorld::Land> newRecord (
            CSMWorld::RecordBase::State_ModifiedOnly, 0, &newLand);

        mState.mTarget->getData().getLand().appendRecord (newRecord);
    }
}
