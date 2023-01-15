#ifndef INTERPRETER_INTERPRETER_H_INCLUDED
#define INTERPRETER_INTERPRETER_H_INCLUDED

#include <cassert>
#include <map>
#include <memory>
#include <stack>
#include <utility>

#include "components/interpreter/program.hpp"
#include "opcodes.hpp"
#include "runtime.hpp"
#include "types.hpp"

namespace Interpreter
{
    struct Program;

    class Interpreter
    {
        std::stack<Runtime> mCallstack;
        bool mRunning = false;
        Runtime mRuntime;
        std::map<int, std::unique_ptr<Opcode1>> mSegment0;
        std::map<int, std::unique_ptr<Opcode1>> mSegment2;
        std::map<int, std::unique_ptr<Opcode1>> mSegment3;
        std::map<int, std::unique_ptr<Opcode0>> mSegment5;

        void execute(Type_Code code);

        void begin();

        void end();

        template <typename TSeg, typename TOp>
        void installSegment(TSeg& seg, int code, TOp&& op)
        {
            assert(seg.find(code) == seg.end());
            seg.emplace(code, std::move(op));
        }

    public:
        Interpreter() = default;

        Interpreter(const Interpreter&) = delete;
        Interpreter& operator=(const Interpreter&) = delete;

        template <typename T, typename... TArgs>
        void installSegment0(int code, TArgs&&... args)
        {
            installSegment(mSegment0, code, std::make_unique<T>(std::forward<TArgs>(args)...));
        }

        template <typename T, typename... TArgs>
        void installSegment2(int code, TArgs&&... args)
        {
            installSegment(mSegment2, code, std::make_unique<T>(std::forward<TArgs>(args)...));
        }

        template <typename T, typename... TArgs>
        void installSegment3(int code, TArgs&&... args)
        {
            installSegment(mSegment3, code, std::make_unique<T>(std::forward<TArgs>(args)...));
        }

        template <typename T, typename... TArgs>
        void installSegment5(int code, TArgs&&... args)
        {
            installSegment(mSegment5, code, std::make_unique<T>(std::forward<TArgs>(args)...));
        }

        void run(const Program& program, Context& context);
    };
}

#endif
