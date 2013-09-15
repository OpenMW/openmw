#ifndef CSM_DOC_SAVINGSTATE_H
#define CSM_DOC_SAVINGSTATE_H

namespace CSMDoc
{
    class Operation;

    class SavingState
    {
            Operation& mOperation;

        public:

            SavingState (Operation& operation);

            bool hasError() const;
    };


}

#endif