#include "scripttest.hpp"

#include <iostream>

#include "../mwworld/manualref.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwscript/compilercontext.hpp"

#include <components/compiler/exception.hpp>
#include <components/compiler/streamerrorhandler.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/locals.hpp>
#include <components/compiler/output.hpp>
#include <components/compiler/scriptparser.hpp>

#include "filter.hpp"

namespace
{

void test(const MWWorld::Ptr& actor, int &compiled, int &total, const Compiler::Extensions* extensions, int warningsMode)
{
    MWDialogue::Filter filter(actor, 0, false);

    MWScript::CompilerContext compilerContext(MWScript::CompilerContext::Type_Dialogue);
    compilerContext.setExtensions(extensions);
    std::ostream errorStream(std::cout.rdbuf());
    Compiler::StreamErrorHandler errorHandler(errorStream);
    errorHandler.setWarningsMode (warningsMode);

    const MWWorld::Store<ESM::Dialogue>& dialogues = MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();
    for (MWWorld::Store<ESM::Dialogue>::iterator it = dialogues.begin(); it != dialogues.end(); ++it)
    {
        std::vector<const ESM::DialInfo*> infos = filter.listAll(*it);

        for (std::vector<const ESM::DialInfo*>::iterator it = infos.begin(); it != infos.end(); ++it)
        {
            const ESM::DialInfo* info = *it;
            if (!info->mResultScript.empty())
            {
                bool success = true;
                ++total;
                try
                {
                    errorHandler.reset();

                    std::istringstream input (info->mResultScript + "\n");

                    Compiler::Scanner scanner (errorHandler, input, extensions);

                    Compiler::Locals locals;

                    std::string actorScript = actor.getClass().getScript(actor);

                    if (!actorScript.empty())
                    {
                        // grab local variables from actor's script, if available.
                        locals = MWBase::Environment::get().getScriptManager()->getLocals (actorScript);
                    }

                    Compiler::ScriptParser parser(errorHandler, compilerContext, locals, false);

                    scanner.scan (parser);

                    if (!errorHandler.isGood())
                        success = false;

                    ++compiled;
                }
                catch (const Compiler::SourceException& /* error */)
                {
                    // error has already been reported via error handler
                    success = false;
                }
                catch (const std::exception& error)
                {
                    std::cerr << std::string ("Dialogue error: An exception has been thrown: ") + error.what() << std::endl;
                    success = false;
                }

                if (!success)
                {
                    std::cerr
                        << "compiling failed (dialogue script)" << std::endl
                        << info->mResultScript
                        << std::endl << std::endl;
                }
            }
        }
    }
}

}

namespace MWDialogue
{

namespace ScriptTest
{

    std::pair<int, int> compileAll(const Compiler::Extensions *extensions, int warningsMode)
    {
        int compiled = 0, total = 0;
        const MWWorld::Store<ESM::NPC>& npcs = MWBase::Environment::get().getWorld()->getStore().get<ESM::NPC>();
        for (MWWorld::Store<ESM::NPC>::iterator it = npcs.begin(); it != npcs.end(); ++it)
        {
            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), it->mId);
            test(ref.getPtr(), compiled, total, extensions, warningsMode);
        }

        const MWWorld::Store<ESM::Creature>& creatures = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>();
        for (MWWorld::Store<ESM::Creature>::iterator it = creatures.begin(); it != creatures.end(); ++it)
        {
            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), it->mId);
            test(ref.getPtr(), compiled, total, extensions, warningsMode);
        }
        return std::make_pair(total, compiled);
    }

}

}
