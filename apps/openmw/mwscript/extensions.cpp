
#include "extensions.hpp"

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/installopcodes.hpp>

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
    
    void installOpcodes (Interpreter::Interpreter& interpreter)
    {
        Interpreter::installOpcodes (interpreter);
        Cell::installOpcodes (interpreter);
        Misc::installOpcodes (interpreter);
        Gui::installOpcodes (interpreter);
        Sound::installOpcodes (interpreter);
        Sky::installOpcodes (interpreter);
    }    
}

