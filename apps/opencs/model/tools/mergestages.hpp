#ifndef CSM_TOOLS_MERGESTAGES_H
#define CSM_TOOLS_MERGESTAGES_H

#include "../doc/stage.hpp"

namespace CSMTools
{
    struct MergeState;

    class FinishMergedDocumentStage : public CSMDoc::Stage
    {
            MergeState& mState;

        public:

            FinishMergedDocumentStage (MergeState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif
