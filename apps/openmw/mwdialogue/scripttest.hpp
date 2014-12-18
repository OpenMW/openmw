#ifndef OPENMW_MWDIALOGUE_SCRIPTTEST_H
#define OPENMW_MWDIALOGUE_SCRIPTTEST_H

#include <components/compiler/extensions.hpp>

namespace MWDialogue
{

namespace ScriptTest
{

/// Attempt to compile all dialogue scripts, use for verification purposes
/// @return A pair containing <total number of scripts, number of successfully compiled scripts>
std::pair<int, int> compileAll(const Compiler::Extensions* extensions, int warningsMode);

}

}

#endif
