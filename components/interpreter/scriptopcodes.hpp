#ifndef INTERPRETER_SCRIPTOPCODES_H_INCLUDED
#define INTERPRETER_SCRIPTOPCODES_H_INCLUDED

#include "opcodes.hpp"
#include "runtime.hpp"
#include "context.hpp"

namespace Interpreter
{
    class OpScriptRunning : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                std::string name = runtime.getStringLiteral (runtime[0].mInteger);
                runtime[0].mInteger = runtime.getContext().isScriptRunning (name);
            }
    };

    class OpStartScript : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                std::string name = runtime.getStringLiteral (runtime[0].mInteger);
                runtime.pop();
                runtime.getContext().startScript (name, runtime.getContext().getTargetId());
            }
    };

    class OpStartScriptExplicit : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                std::string targetId = runtime.getStringLiteral (runtime[0].mInteger);
                runtime.pop();

                std::string name = runtime.getStringLiteral (runtime[0].mInteger);
                runtime.pop();

                runtime.getContext().startScript (name, targetId);
            }
    };

    class OpStopScript : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                std::string name = runtime.getStringLiteral (runtime[0].mInteger);
                runtime.pop();
                runtime.getContext().stopScript (name);
            }
    };
}

#endif

