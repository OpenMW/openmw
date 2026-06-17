#ifndef INTERPRETER_INTERPRETER_H_INCLUDED
#define INTERPRETER_INTERPRETER_H_INCLUDED

#include <map>
#include <memory>
#include <stack>
#include <utility>

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

        [[noreturn]] void abortDuplicateInstruction(std::string_view name, int code);

        template <typename T, typename... Args>
        void installSegment(auto& segment, std::string_view name, int code, Args&&... args)
        {
            if (segment.find(code) != segment.end())
                abortDuplicateInstruction(name, code);
            segment.emplace(code, std::make_unique<T>(std::forward<Args>(args)...));
        }

    public:
        Interpreter() = default;

        Interpreter(const Interpreter&) = delete;
        Interpreter& operator=(const Interpreter&) = delete;

        template <typename T, typename... TArgs>
        void installSegment0(int code, TArgs&&... args)
        {
            installSegment<T>(mSegment0, "0", code, std::forward<TArgs>(args)...);
        }

        template <typename T, typename... TArgs>
        void installSegment2(int code, TArgs&&... args)
        {
            installSegment<T>(mSegment2, "2", code, std::forward<TArgs>(args)...);
        }

        template <typename T, typename... TArgs>
        void installSegment3(int code, TArgs&&... args)
        {
            installSegment<T>(mSegment3, "3", code, std::forward<TArgs>(args)...);
        }

        template <typename T, typename... TArgs>
        void installSegment5(int code, TArgs&&... args)
        {
            installSegment<T>(mSegment5, "5", code, std::forward<TArgs>(args)...);
        }

        void run(const Program& program, Context& context);
    };
}

#endif
