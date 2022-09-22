#include "extensions.hpp"

#include <components/interpreter/installopcodes.hpp>
#include <components/interpreter/interpreter.hpp>

#include "aiextensions.hpp"
#include "animationextensions.hpp"
#include "cellextensions.hpp"
#include "consoleextensions.hpp"
#include "containerextensions.hpp"
#include "controlextensions.hpp"
#include "dialogueextensions.hpp"
#include "guiextensions.hpp"
#include "miscextensions.hpp"
#include "skyextensions.hpp"
#include "soundextensions.hpp"
#include "statsextensions.hpp"
#include "transformationextensions.hpp"
#include "userextensions.hpp"

namespace MWScript
{
    void installOpcodes(Interpreter::Interpreter& interpreter, bool consoleOnly)
    {
        Interpreter::installOpcodes(interpreter);
        Cell::installOpcodes(interpreter);
        Misc::installOpcodes(interpreter);
        Gui::installOpcodes(interpreter);
        Sound::installOpcodes(interpreter);
        Sky::installOpcodes(interpreter);
        Stats::installOpcodes(interpreter);
        Container::installOpcodes(interpreter);
        Ai::installOpcodes(interpreter);
        Control::installOpcodes(interpreter);
        Dialogue::installOpcodes(interpreter);
        Animation::installOpcodes(interpreter);
        Transformation::installOpcodes(interpreter);

        if (consoleOnly)
        {
            Console::installOpcodes(interpreter);
            User::installOpcodes(interpreter);
        }
    }
}
