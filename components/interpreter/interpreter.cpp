#include "interpreter.hpp"

#include <cassert>
#include <stdexcept>

#include "opcodes.hpp"

namespace Interpreter
{
    void Interpreter::execute (Type_Code code)
    {
        unsigned int segSpec = code>>30;

        switch (segSpec)
        {
            case 0:
            {
                int opcode = code>>24;
                unsigned int arg0 = code & 0xffffff;

                std::map<int, Opcode1 *>::iterator iter = mSegment0.find (opcode);

                if (iter==mSegment0.end())
                    abortUnknownCode (0, opcode);

                iter->second->execute (mRuntime, arg0);

                return;
            }

            case 2:
            {
                int opcode = (code>>20) & 0x3ff;
                unsigned int arg0 = code & 0xfffff;

                std::map<int, Opcode1 *>::iterator iter = mSegment2.find (opcode);

                if (iter==mSegment2.end())
                    abortUnknownCode (2, opcode);

                iter->second->execute (mRuntime, arg0);

                return;
            }
        }

        segSpec = code>>26;

        switch (segSpec)
        {
            case 0x30:
            {
                int opcode = (code>>8) & 0x3ffff;
                unsigned int arg0 = code & 0xff;

                std::map<int, Opcode1 *>::iterator iter = mSegment3.find (opcode);

                if (iter==mSegment3.end())
                    abortUnknownCode (3, opcode);

                iter->second->execute (mRuntime, arg0);

                return;
            }

            case 0x32:
            {
                int opcode = code & 0x3ffffff;

                std::map<int, Opcode0 *>::iterator iter = mSegment5.find (opcode);

                if (iter==mSegment5.end())
                    abortUnknownCode (5, opcode);

                iter->second->execute (mRuntime);

                return;
            }
        }

        abortUnknownSegment (code);
    }

    void Interpreter::abortUnknownCode (int segment, int opcode)
    {
        const std::string error = "unknown opcode " + std::to_string(opcode) + " in segment " + std::to_string(segment);
        throw std::runtime_error (error);
    }

    void Interpreter::abortUnknownSegment (Type_Code code)
    {
        const std::string error = "opcode outside of the allocated segment range: " + std::to_string(code);
        throw std::runtime_error (error);
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

    Interpreter::~Interpreter()
    {
        for (std::map<int, Opcode1 *>::iterator iter (mSegment0.begin());
            iter!=mSegment0.end(); ++iter)
            delete iter->second;

        for (std::map<int, Opcode1 *>::iterator iter (mSegment2.begin());
            iter!=mSegment2.end(); ++iter)
            delete iter->second;

        for (std::map<int, Opcode1 *>::iterator iter (mSegment3.begin());
            iter!=mSegment3.end(); ++iter)
            delete iter->second;

        for (std::map<int, Opcode0 *>::iterator iter (mSegment5.begin());
            iter!=mSegment5.end(); ++iter)
            delete iter->second;
    }

    void Interpreter::installSegment0 (int code, Opcode1 *opcode)
    {
        assert(mSegment0.find(code) == mSegment0.end());
        mSegment0.insert (std::make_pair (code, opcode));
    }

    void Interpreter::installSegment2 (int code, Opcode1 *opcode)
    {
        assert(mSegment2.find(code) == mSegment2.end());
        mSegment2.insert (std::make_pair (code, opcode));
    }

    void Interpreter::installSegment3 (int code, Opcode1 *opcode)
    {
        assert(mSegment3.find(code) == mSegment3.end());
        mSegment3.insert (std::make_pair (code, opcode));
    }

    void Interpreter::installSegment5 (int code, Opcode0 *opcode)
    {
        assert(mSegment5.find(code) == mSegment5.end());
        mSegment5.insert (std::make_pair (code, opcode));
    }

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
