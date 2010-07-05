
#include "extensions.hpp"

#include "../mwsound/extensions.hpp"

#include "cellextensions.hpp"
#include "miscextensions.hpp"

namespace MWScript
{
    void registerExtensions (Compiler::Extensions& extensions)
    {
        Cell::registerExtensions (extensions);
        Misc::registerExtensions (extensions);
        MWSound::registerExtensions (extensions);
    }
}

