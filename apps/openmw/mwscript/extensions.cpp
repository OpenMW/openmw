
#include "extensions.hpp"

#include "soundextensions.hpp"
#include "cellextensions.hpp"
#include "miscextensions.hpp"
#include "guiextensions.hpp"
#include "skyextensions.hpp"

namespace MWScript
{
    void registerExtensions (Compiler::Extensions& extensions)
    {
        Cell::registerExtensions (extensions);
        Misc::registerExtensions (extensions);
        Gui::registerExtensions (extensions);
        Sound::registerExtensions (extensions);
        Sky::registerExtensions (extensions);
    }
}

