#ifndef CSM_WOLRD_SCOPE_H
#define CSM_WOLRD_SCOPE_H

#include <string>

namespace CSMWorld
{
    enum Scope
    {
        // record stored in content file
        Scope_Content = 1,

        // record stored in project file
        Scope_Project = 2,

        // record that exists only for the duration of one editing session
        Scope_Session = 4
    };

    Scope getScopeFromId (const std::string& id);
}

#endif
