#ifndef CSM_DOC_STATE_H
#define CSM_DOC_STATE_H

namespace CSMDoc
{
        enum State
        {
                State_Modified = 1,
                State_Locked = 2,
                State_Operation = 4,

                State_Saving = 8,
                State_Verifying = 16,
                State_Compiling = 32, // not implemented yet
                State_Searching = 64 // not implemented yet
        };
}

#endif
