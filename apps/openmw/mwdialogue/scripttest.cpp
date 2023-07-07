#include "scripttest.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwscript/compilercontext.hpp"

#include <components/compiler/exception.hpp>
#include <components/compiler/locals.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/scriptparser.hpp>
#include <components/compiler/streamerrorhandler.hpp>
#include <components/debug/debuglog.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadnpc.hpp>

#include "filter.hpp"

#include <memory>
#include <set>
#include <sstream>

namespace
{

    bool test(const MWWorld::Ptr& actor, const ESM::DialInfo& info, int& compiled, int& total,
        const Compiler::Extensions* extensions, const MWScript::CompilerContext& context,
        Compiler::StreamErrorHandler& errorHandler)
    {
        bool success = true;
        ++total;
        try
        {
            std::istringstream input(info.mResultScript + "\n");

            Compiler::Scanner scanner(errorHandler, input, extensions);

            Compiler::Locals locals;

            const ESM::RefId& actorScript = actor.getClass().getScript(actor);
            if (!actorScript.empty())
            {
                // grab local variables from actor's script, if available.
                locals = MWBase::Environment::get().getScriptManager()->getLocals(actorScript);
            }

            Compiler::ScriptParser parser(errorHandler, context, locals, false);

            scanner.scan(parser);

            if (!errorHandler.isGood())
                success = false;

            ++compiled;
        }
        catch (const Compiler::SourceException&)
        {
            // error has already been reported via error handler
            success = false;
        }
        catch (const std::exception& error)
        {
            Log(Debug::Error) << "Dialogue error: An exception has been thrown: " << error.what();
            success = false;
        }

        return success;
    }

    bool superficiallyMatches(const MWWorld::Ptr& ptr, const ESM::DialInfo& info)
    {
        if (ptr.isEmpty())
            return false;
        MWDialogue::Filter filter(ptr, 0, false);
        return filter.couldPotentiallyMatch(info);
    }

}

namespace MWDialogue
{

    namespace ScriptTest
    {

        std::pair<int, int> compileAll(const Compiler::Extensions* extensions, int warningsMode)
        {
            int compiled = 0, total = 0;
            const auto& store = *MWBase::Environment::get().getESMStore();

            MWScript::CompilerContext compilerContext(MWScript::CompilerContext::Type_Dialogue);
            compilerContext.setExtensions(extensions);
            Compiler::StreamErrorHandler errorHandler;
            errorHandler.setWarningsMode(warningsMode);

            std::unique_ptr<MWWorld::ManualRef> ref;
            for (const ESM::Dialogue& topic : store.get<ESM::Dialogue>())
            {
                MWWorld::Ptr ptr;
                for (const ESM::DialInfo& info : topic.mInfo)
                {
                    if (info.mResultScript.empty())
                        continue;
                    if (!info.mActor.empty())
                    {
                        // We know it can only ever be this actor
                        try
                        {
                            ref = std::make_unique<MWWorld::ManualRef>(store, info.mActor);
                            ptr = ref->getPtr();
                        }
                        catch (const std::logic_error&)
                        {
                            // Too bad it doesn't exist
                            ptr = {};
                        }
                    }
                    else
                    {
                        // Try to find a matching actor
                        if (!superficiallyMatches(ptr, info))
                        {
                            ptr = {};
                            bool found = false;
                            for (const auto& npc : store.get<ESM::NPC>())
                            {
                                ref = std::make_unique<MWWorld::ManualRef>(store, npc.mId);
                                if (superficiallyMatches(ref->getPtr(), info))
                                {
                                    ptr = ref->getPtr();
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                for (const auto& creature : store.get<ESM::Creature>())
                                {
                                    ref = std::make_unique<MWWorld::ManualRef>(store, creature.mId);
                                    if (superficiallyMatches(ref->getPtr(), info))
                                    {
                                        ptr = ref->getPtr();
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (ptr.isEmpty())
                        Log(Debug::Error) << "Could not find an actor to test " << info.mId << " in " << topic.mId;
                    else
                    {
                        errorHandler.reset();
                        errorHandler.setContext(info.mId.getRefIdString() + " in " + topic.mStringId);
                        if (!test(ptr, info, compiled, total, extensions, compilerContext, errorHandler))
                            Log(Debug::Error) << "Test failed for " << info.mId << " in " << topic.mId << '\n'
                                              << info.mResultScript;
                    }
                }
            }

            return std::make_pair(total, compiled);
        }

    }

}
