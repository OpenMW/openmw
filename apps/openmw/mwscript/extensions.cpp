
#include "extensions.hpp"

#include "cellextensions.hpp"

namespace MWScript
{
    void registerExtensions (Compiler::Extensions& extensions)
    {
        Cell::registerExtensions (extensions);
    }
}

