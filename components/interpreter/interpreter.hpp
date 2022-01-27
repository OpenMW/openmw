#ifndef INTERPRETER_INTERPRETER_H_INCLUDED
#define INTERPRETER_INTERPRETER_H_INCLUDED

#include <map>
#include <stack>
#include <memory>
#include <cassert>
#include <utility>

#include "runtime.hpp"
#include "types.hpp"
#include "opcodes.hpp"

namespace Interpreter
{
    class Interpreter
    {
            std::stack<Runtime> mCallstack;
            bool mRunning;
            Runtime mRuntime;
            std::map<int, std::unique_ptr<Opcode1>> mSegment0;
            std::map<int, std::unique_ptr<Opcode1>> mSegment2;
            std::map<int, std::unique_ptr<Opcode1>> mSegment3;
            std::map<int, std::unique_ptr<Opcode0>> mSegment5;

            // not implemented
            Interpreter (const Interpreter&);
            Interpreter& operator= (const Interpreter&);

            void execute (Type_Code code);

            void begin();

            void end();

            template<typename TSeg, typename TOp>
            void installSegment(TSeg& seg, int code, TOp&& op)
            {
                assert(seg.find(code) == seg.end());
                seg.emplace(code, std::move(op));
            }

        public:

            Interpreter();

            template<typename T, typename ...TArgs>
            void installSegment0(int code, TArgs&& ...args)
            {
                installSegment(mSegment0, code, std::make_unique<T>(std::forward<TArgs>(args)...));
            }

            template<typename T, typename ...TArgs>
            void installSegment2(int code, TArgs&& ...args)
            {
                installSegment(mSegment2, code, std::make_unique<T>(std::forward<TArgs>(args)...));
            }

            template<typename T, typename ...TArgs>
            void installSegment3(int code, TArgs&& ...args)
            {
                installSegment(mSegment3, code, std::make_unique<T>(std::forward<TArgs>(args)...));
            }

            template<typename T, typename ...TArgs>
            void installSegment5(int code, TArgs&& ...args)
            {
                installSegment(mSegment5, code, std::make_unique<T>(std::forward<TArgs>(args)...));
            }

            void run (const Type_Code *code, int codeSize, Context& context);
    };
}

#endif
