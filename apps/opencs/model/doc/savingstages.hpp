#ifndef CSM_DOC_SAVINGSTAGES_H
#define CSM_DOC_SAVINGSTAGES_H

#include "stage.hpp"

namespace CSMDoc
{
    class Document;
    class SavingState;

    class FinalSavingStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            FinalSavingStage (Document& document, SavingState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, std::vector<std::string>& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif
