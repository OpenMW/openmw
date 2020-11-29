#ifndef CSM_TOOLS_MERGESTAGES_H
#define CSM_TOOLS_MERGESTAGES_H

#include <algorithm>
#include <map>

#include <components/to_utf8/to_utf8.hpp>

#include "../doc/stage.hpp"

#include "../world/data.hpp"

#include "mergestate.hpp"

namespace CSMTools
{
    class StartMergeStage : public CSMDoc::Stage
    {
            MergeState& mState;

        public:

            StartMergeStage (MergeState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class FinishMergedDocumentStage : public CSMDoc::Stage
    {
            MergeState& mState;
            ToUTF8::Utf8Encoder mEncoder;

        public:

            FinishMergedDocumentStage (MergeState& state, ToUTF8::FromType encoding);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    template<typename RecordType, typename Collection = CSMWorld::IdCollection<RecordType> >
    class MergeIdCollectionStage : public CSMDoc::Stage
    {
            MergeState& mState;
            Collection& (CSMWorld::Data::*mAccessor)();

        public:

            MergeIdCollectionStage (MergeState& state, Collection& (CSMWorld::Data::*accessor)());

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    template<typename RecordType, typename Collection>
    MergeIdCollectionStage<RecordType, Collection>::MergeIdCollectionStage (MergeState& state, Collection& (CSMWorld::Data::*accessor)())
    : mState (state), mAccessor (accessor)
    {}

    template<typename RecordType, typename Collection>
    int MergeIdCollectionStage<RecordType, Collection>::setup()
    {
        return (mState.mSource.getData().*mAccessor)().getSize();
    }

    template<typename RecordType, typename Collection>
    void MergeIdCollectionStage<RecordType, Collection>::perform (int stage, CSMDoc::Messages& messages)
    {
        const Collection& source = (mState.mSource.getData().*mAccessor)();
        Collection& target = (mState.mTarget->getData().*mAccessor)();

        const CSMWorld::Record<RecordType>& record = source.getRecord (stage);

        if (!record.isDeleted())
            target.appendRecord (CSMWorld::Record<RecordType> (CSMWorld::RecordBase::State_ModifiedOnly, nullptr, &record.get()));
    }

    class MergeRefIdsStage : public CSMDoc::Stage
    {
            MergeState& mState;

        public:

            MergeRefIdsStage (MergeState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class MergeReferencesStage : public CSMDoc::Stage
    {
            MergeState& mState;
            std::map<std::string, int> mIndex;

        public:

            MergeReferencesStage (MergeState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    /// Adds all land texture records that could potentially be referenced when merging
    class PopulateLandTexturesMergeStage : public CSMDoc::Stage
    {
            MergeState& mState;

        public:

            PopulateLandTexturesMergeStage (MergeState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class MergeLandStage : public CSMDoc::Stage
    {
            MergeState& mState;

        public:

            MergeLandStage (MergeState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    /// During this stage, the complex process of combining LandTextures from
    /// potentially multiple plugins is undertaken.
    class FixLandsAndLandTexturesMergeStage : public CSMDoc::Stage
    {
            MergeState& mState;

        public:

            FixLandsAndLandTexturesMergeStage (MergeState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    /// Removes base LandTexture records. This gets rid of the base records previously
    /// needed in FixLandsAndLandTexturesMergeStage.
    class CleanupLandTexturesMergeStage : public CSMDoc::Stage
    {
            MergeState& mState;

        public:

            CleanupLandTexturesMergeStage (MergeState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif
