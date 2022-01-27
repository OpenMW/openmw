#include "interpreter.hpp"

#include <cassert>
#include <stdexcept>

#include "opcodes.hpp"

namespace Interpreter
{
    [[noreturn]] static void abortUnknownCode(int segment, int opcode)
    {
        const std::string error = "unknown opcode " + std::to_string(opcode) + " in segment " + std::to_string(segment);
        throw std::runtime_error(error);
    }

    [[noreturn]] static void abortUnknownSegment(Type_Code code)
    {
        const std::string error = "opcode outside of the allocated segment range: " + std::to_string(code);
        throw std::runtime_error(error);
    }

    template<typename T>
    auto& getDispatcher(const T& segment, unsigned int seg, int opcode)
    {
        auto it = segment.find(opcode);
        if (it == segment.end())
        {
            abortUnknownCode(seg, opcode);
        }
        return it->second;
    }

    void Interpreter::execute (Type_Code code)
    {
        unsigned int segSpec = code >> 30;

        switch (segSpec)
        {
            case 0:
            {
                const int opcode = code >> 24;
                const unsigned int arg0 = code & 0xffffff;

                return getDispatcher(mSegment0, 0, opcode)->execute(mRuntime, arg0);
            }

            case 2:
            {
                const int opcode = (code >> 20) & 0x3ff;
                const unsigned int arg0 = code & 0xfffff;

                return getDispatcher(mSegment2, 2, opcode)->execute(mRuntime, arg0);
            }
        }

        segSpec = code >> 26;

        switch (segSpec)
        {
            case 0x30:
            {
                const int opcode = (code >> 8) & 0x3ffff;
                const unsigned int arg0 = code & 0xff;

                return getDispatcher(mSegment3, 3, opcode)->execute(mRuntime, arg0);
            }

            case 0x32:
            {
                const int opcode = code & 0x3ffffff;

                return getDispatcher(mSegment5, 5, opcode)->execute(mRuntime);
            }
        }

        abortUnknownSegment (code);
    }

    void Interpreter::begin()
    {
        if (mRunning)
        {
            mCallstack.push (mRuntime);
            mRuntime.clear();
        }
        else
        {
            mRunning = true;
        }
    }

    void Interpreter::end()
    {
        if (mCallstack.empty())
        {
            mRuntime.clear();
            mRunning = false;
        }
        else
        {
            mRuntime = mCallstack.top();
            mCallstack.pop();
        }
    }

    Interpreter::Interpreter() : mRunning (false)
    {}

    void Interpreter::run (const Type_Code *code, int codeSize, Context& context)
    {
        assert (codeSize>=4);

        begin();

        try
        {
            mRuntime.configure (code, codeSize, context);

            int opcodes = static_cast<int> (code[0]);

            const Type_Code *codeBlock = code + 4;

            while (mRuntime.getPC()>=0 && mRuntime.getPC()<opcodes)
            {
                Type_Code runCode = codeBlock[mRuntime.getPC()];
                mRuntime.setPC (mRuntime.getPC()+1);
                execute (runCode);
            }
        }
        catch (...)
        {
            end();
            throw;
        }

        end();
    }
}
