#ifndef CSM_DOC_SAVING_H
#define CSM_DOC_SAVING_H

#include "operation.hpp"
#include "savingstate.hpp"

namespace CSMDoc
{
    class Document;

    class Saving : public Operation
    {
            Q_OBJECT

            Document& mDocument;
            SavingState mState;

        public:

            Saving (Document& document);

    };
}

#endif
