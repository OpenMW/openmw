#ifndef MWSCRIPT_TESTING_UTIL_H
#define MWSCRIPT_TESTING_UTIL_H

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <components/compiler/context.hpp>
#include <components/compiler/errorhandler.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/extensions.hpp>
#include <components/compiler/extensions0.hpp>
#include <components/compiler/fileparser.hpp>
#include <components/compiler/opcodes.hpp>
#include <components/compiler/scanner.hpp>

#include <components/interpreter/context.hpp>
#include <components/interpreter/installopcodes.hpp>
#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>

#include <components/misc/strings/algorithm.hpp>

namespace
{
    class TestCompilerContext : public Compiler::Context
    {
    public:
        bool canDeclareLocals() const override { return true; }
        char getGlobalType(const std::string& name) const override { return ' '; }
        std::pair<char, bool> getMemberType(const std::string& name, const ESM::RefId& id) const override
        {
            return { ' ', false };
        }
        bool isId(const ESM::RefId& name) const override { return name == "player"; }
    };

    class TestErrorHandler : public Compiler::ErrorHandler
    {
        std::vector<std::pair<std::string, Compiler::TokenLoc>> mErrors;

        void report(
            const std::string& message, const Compiler::TokenLoc& loc, Compiler::ErrorHandler::Type type) override
        {
            if (type == Compiler::ErrorHandler::ErrorMessage)
                mErrors.emplace_back(message, loc);
        }

        void report(const std::string& message, Compiler::ErrorHandler::Type type) override
        {
            report(message, {}, type);
        }

    public:
        void reset() override
        {
            Compiler::ErrorHandler::reset();
            mErrors.clear();
        }

        const std::vector<std::pair<std::string, Compiler::TokenLoc>>& getErrors() const { return mErrors; }
    };

    class LocalVariables
    {
        std::vector<int> mShorts;
        std::vector<int> mLongs;
        std::vector<float> mFloats;

        template <class T>
        T getLocal(std::size_t index, const std::vector<T>& vector) const
        {
            if (index < vector.size())
                return vector[index];
            return {};
        }

        template <class T>
        void setLocal(T value, std::size_t index, std::vector<T>& vector)
        {
            if (index >= vector.size())
                vector.resize(index + 1);
            vector[index] = value;
        }

    public:
        void clear()
        {
            mShorts.clear();
            mLongs.clear();
            mFloats.clear();
        }

        int getShort(std::size_t index) const { return getLocal(index, mShorts); }

        int getLong(std::size_t index) const { return getLocal(index, mLongs); }

        float getFloat(std::size_t index) const { return getLocal(index, mFloats); }

        void setShort(std::size_t index, int value) { setLocal(value, index, mShorts); }

        void setLong(std::size_t index, int value) { setLocal(value, index, mLongs); }

        void setFloat(std::size_t index, float value) { setLocal(value, index, mFloats); }
    };

    class GlobalVariables
    {
        std::map<std::string, int, std::less<>> mShorts;
        std::map<std::string, int, std::less<>> mLongs;
        std::map<std::string, float, std::less<>> mFloats;

        template <class T>
        T getGlobal(std::string_view name, const std::map<std::string, T, std::less<>>& map) const
        {
            auto it = map.find(name);
            if (it != map.end())
                return it->second;
            return {};
        }

    public:
        void clear()
        {
            mShorts.clear();
            mLongs.clear();
            mFloats.clear();
        }

        int getShort(std::string_view name) const { return getGlobal(name, mShorts); }

        int getLong(std::string_view name) const { return getGlobal(name, mLongs); }

        float getFloat(std::string_view name) const { return getGlobal(name, mFloats); }

        void setShort(std::string_view name, int value) { mShorts[std::string(name)] = value; }

        void setLong(std::string_view name, int value) { mLongs[std::string(name)] = value; }

        void setFloat(std::string_view name, float value) { mFloats[std::string(name)] = value; }
    };

    class TestInterpreterContext : public Interpreter::Context
    {
        LocalVariables mLocals;
        std::map<ESM::RefId, GlobalVariables> mMembers;
        std::vector<std::string> mMessages;

    public:
        const std::vector<std::string>& getMessages() { return mMessages; }

        ESM::RefId getTarget() const override { return ESM::RefId(); }

        int getLocalShort(int index) const override { return mLocals.getShort(index); }

        int getLocalLong(int index) const override { return mLocals.getLong(index); }

        float getLocalFloat(int index) const override { return mLocals.getFloat(index); }

        void setLocalShort(int index, int value) override { mLocals.setShort(index, value); }

        void setLocalLong(int index, int value) override { mLocals.setLong(index, value); }

        void setLocalFloat(int index, float value) override { mLocals.setFloat(index, value); }

        void messageBox(std::string_view message, const std::vector<std::string>& buttons) override
        {
            mMessages.emplace_back(message);
        }

        void report(const std::string& message) override {}

        int getGlobalShort(std::string_view name) const override { return {}; }

        int getGlobalLong(std::string_view name) const override { return {}; }

        float getGlobalFloat(std::string_view name) const override { return {}; }

        void setGlobalShort(std::string_view name, int value) override {}

        void setGlobalLong(std::string_view name, int value) override {}

        void setGlobalFloat(std::string_view name, float value) override {}

        std::vector<std::string> getGlobals() const override { return {}; }

        char getGlobalType(std::string_view name) const override { return ' '; }

        std::string getActionBinding(std::string_view action) const override { return {}; }

        std::string_view getActorName() const override { return {}; }

        std::string_view getNPCRace() const override { return {}; }

        std::string_view getNPCClass() const override { return {}; }

        std::string_view getNPCFaction() const override { return {}; }

        std::string_view getNPCRank() const override { return {}; }

        std::string_view getPCName() const override { return {}; }

        std::string_view getPCRace() const override { return {}; }

        std::string_view getPCClass() const override { return {}; }

        std::string_view getPCRank() const override { return {}; }

        std::string_view getPCNextRank() const override { return {}; }

        int getPCBounty() const override { return {}; }

        std::string_view getCurrentCellName() const override { return {}; }

        int getMemberShort(ESM::RefId id, std::string_view name, bool global) const override
        {
            auto it = mMembers.find(id);
            if (it != mMembers.end())
                return it->second.getShort(name);
            return {};
        }

        int getMemberLong(ESM::RefId id, std::string_view name, bool global) const override
        {
            auto it = mMembers.find(id);
            if (it != mMembers.end())
                return it->second.getLong(name);
            return {};
        }

        float getMemberFloat(ESM::RefId id, std::string_view name, bool global) const override
        {
            auto it = mMembers.find(id);
            if (it != mMembers.end())
                return it->second.getFloat(name);
            return {};
        }

        void setMemberShort(ESM::RefId id, std::string_view name, int value, bool global) override
        {
            mMembers[id].setShort(name, value);
        }

        void setMemberLong(ESM::RefId id, std::string_view name, int value, bool global) override
        {
            mMembers[id].setLong(name, value);
        }

        void setMemberFloat(ESM::RefId id, std::string_view name, float value, bool global) override
        {
            mMembers[id].setFloat(name, value);
        }
    };

    struct CompiledScript
    {
        Interpreter::Program mProgram;
        Compiler::Locals mLocals;

        CompiledScript(Interpreter::Program&& program, const Compiler::Locals& locals)
            : mProgram(std::move(program))
            , mLocals(locals)
        {
        }
    };
}

#endif
