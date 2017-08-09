#ifndef CSM_DOC_STATE_H
#define CSM_DOC_STATE_H

namespace CSMDoc
{
    enum State
    {
        State_Modified = 1,
        State_Locked = 2,
        State_Operation = 4,
        State_Running = 8,

        State_Saving = 16,
        State_Verifying = 32,
        State_Merging = 64,
        State_Searching = 128,
        State_Loading = 256   // pseudo-state; can not be encountered in a loaded document
    };
}

#endif
