#ifndef GAME_SCRIPT_LOCALS_H
#define GAME_SCRIPT_LOCALS_H

#include <string>
#include <string_view>
#include <vector>

#include <components/esm/refid.hpp>
#include <components/interpreter/types.hpp>

namespace ESM
{
    class Script;
    struct Locals;
    class RefId;
}

namespace MWScript
{
    class Locals
    {
        bool mInitialised;
        ESM::RefId mScriptId;

        void ensure(const ESM::RefId& scriptName);

    public:
        std::vector<Interpreter::Type_Short> mShorts;
        std::vector<Interpreter::Type_Integer> mLongs;
        std::vector<Interpreter::Type_Float> mFloats;

        Locals();

        const ESM::RefId& getScriptId() const { return mScriptId; }

        /// Are there any locals?
        ///
        /// \note Will return false, if locals have not been configured yet.
        bool isEmpty() const;

        /// \return Did the state of *this change from uninitialised to initialised?
        bool configure(const ESM::Script& script);

        /// @note var needs to be in lowercase
        ///
        /// \note Locals will be automatically configured first, if necessary
        bool setVarByInt(const ESM::RefId& script, std::string_view var, int val) { return setVar(script, var, val); }
        bool setVar(const ESM::RefId& script, std::string_view var, double val);

        /// \note Locals will be automatically configured first, if necessary
        //
        // \note If it can not be determined if the variable exists, the error will be
        // ignored and false will be returned.
        bool hasVar(const ESM::RefId& script, std::string_view var);

        /// if var does not exist, returns 0
        /// @note var needs to be in lowercase
        ///
        /// \note Locals will be automatically configured first, if necessary
        double getVarAsDouble(const ESM::RefId& script, std::string_view var);
        int getIntVar(const ESM::RefId& script, std::string_view var)
        {
            return static_cast<int>(getVarAsDouble(script, var));
        }
        float getFloatVar(const ESM::RefId& script, std::string_view var)
        {
            return static_cast<float>(getVarAsDouble(script, var));
        }

        std::size_t getSize(const ESM::RefId& script);

        /// \note If locals have not been configured yet, no data is written.
        ///
        /// \return Locals written?
        bool write(ESM::Locals& locals, const ESM::RefId& script) const;

        /// \note Locals will be automatically configured first, if necessary
        void read(const ESM::Locals& locals, const ESM::RefId& script);
    };
}

#endif
