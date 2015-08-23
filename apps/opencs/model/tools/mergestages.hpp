#ifndef CSM_TOOLS_MERGESTAGES_H
#define CSM_TOOLS_MERGESTAGES_H

#include <algorithm>

#include <components/to_utf8/to_utf8.hpp>

#include "../doc/stage.hpp"

#include "../world/data.hpp"

#include "mergestate.hpp"

namespace CSMTools
{
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

            static const int stepSize = 1000;

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
        const Collection& source = (mState.mSource.getData().*mAccessor)();

        int size = source.getSize();

        int steps = size / stepSize;

        if (size % stepSize)
            ++steps;

        return steps;
    }

    template<typename RecordType, typename Collection>
    void MergeIdCollectionStage<RecordType, Collection>::perform (int stage, CSMDoc::Messages& messages)
    {
        const Collection& source = (mState.mSource.getData().*mAccessor)();
        Collection& target = (mState.mTarget->getData().*mAccessor)();

        int begin = stage * stepSize;
        int end = std::min ((stage+1) * stepSize, source.getSize());

        for (int i=begin; i<end; ++i)
        {
            const CSMWorld::Record<RecordType>& record = source.getRecord (i);

            if (!record.isDeleted())
                target.appendRecord (CSMWorld::Record<RecordType> (CSMWorld::RecordBase::State_BaseOnly, &record.get()));
        }
    }
}

#endif
