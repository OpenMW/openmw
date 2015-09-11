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

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class FinishMergedDocumentStage : public CSMDoc::Stage
    {
            MergeState& mState;
            ToUTF8::Utf8Encoder mEncoder;

        public:

            FinishMergedDocumentStage (MergeState& state, ToUTF8::FromType encoding);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    template<typename RecordType, typename Collection = CSMWorld::IdCollection<RecordType> >
    class MergeIdCollectionStage : public CSMDoc::Stage
    {
            MergeState& mState;
            Collection& (CSMWorld::Data::*mAccessor)();

        public:

            MergeIdCollectionStage (MergeState& state, Collection& (CSMWorld::Data::*accessor)());

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
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
            target.appendRecord (CSMWorld::Record<RecordType> (CSMWorld::RecordBase::State_ModifiedOnly, 0, &record.get()));
    }

    class MergeRefIdsStage : public CSMDoc::Stage
    {
            MergeState& mState;

        public:

            MergeRefIdsStage (MergeState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class MergeReferencesStage : public CSMDoc::Stage
    {
            MergeState& mState;
            std::map<std::string, int> mIndex;

        public:

            MergeReferencesStage (MergeState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class ListLandTexturesMergeStage : public CSMDoc::Stage
    {
            MergeState& mState;

        public:

            ListLandTexturesMergeStage (MergeState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class MergeLandTexturesStage : public CSMDoc::Stage
    {
            MergeState& mState;
            std::map<std::pair<uint16_t, int>, int>::iterator mNext;

        public:

            MergeLandTexturesStage (MergeState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class MergeLandStage : public CSMDoc::Stage
    {
            MergeState& mState;

        public:

            MergeLandStage (MergeState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif
