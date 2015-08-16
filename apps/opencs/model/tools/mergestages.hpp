#ifndef CSM_TOOLS_MERGESTAGES_H
#define CSM_TOOLS_MERGESTAGES_H

#include <components/to_utf8/to_utf8.hpp>

#include "../doc/stage.hpp"

namespace CSMTools
{
    struct MergeState;

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
}

#endif
