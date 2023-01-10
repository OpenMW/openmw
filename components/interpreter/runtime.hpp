#ifndef INTERPRETER_RUNTIME_H_INCLUDED
#define INTERPRETER_RUNTIME_H_INCLUDED

#include <string_view>
#include <vector>

#include "types.hpp"

namespace Interpreter
{
    class Context;
    struct Program;

    /// Runtime data and engine interface

    class Runtime
    {
        Context* mContext = nullptr;
        const Program* mProgram = nullptr;
        int mPC = 0;
        std::vector<Data> mStack;

    public:
        int getPC() const { return mPC; }
        ///< return program counter.

        int getIntegerLiteral(int index) const;

        float getFloatLiteral(int index) const;

        std::string_view getStringLiteral(int index) const;

        void configure(const Program& program, Context& context);
        ///< \a context and \a code must exist as least until either configure, clear or
        /// the destructor is called.

        void clear();

        void setPC(int value) { mPC = value; }
        ///< set program counter.

        void push(const Data& data);
        ///< push data on stack

        void push(Type_Integer value);
        ///< push integer data on stack.

        void push(Type_Float value);
        ///< push float data on stack.

        void pop();
        ///< pop stack

        Data& operator[](int index);
        ///< Access stack member, counted from the top.

        Context& getContext();
    };
}

#endif
