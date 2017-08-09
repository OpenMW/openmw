#ifndef GAME_SCRIPT_LOCALS_H
#define GAME_SCRIPT_LOCALS_H

#include <vector>

#include <components/interpreter/types.hpp>

namespace ESM
{
    class Script;
    struct Locals;
}

namespace MWScript
{
    class Locals
    {
            bool mInitialised;

            void ensure (const std::string& scriptName);

        public:
            std::vector<Interpreter::Type_Short> mShorts;
            std::vector<Interpreter::Type_Integer> mLongs;
            std::vector<Interpreter::Type_Float> mFloats;

            Locals();

            /// Are there any locals?
            ///
            /// \note Will return false, if locals have not been configured yet.
            bool isEmpty() const;

            /// \return Did the state of *this change from uninitialised to initialised?
            bool configure (const ESM::Script& script);

            /// @note var needs to be in lowercase
            ///
            /// \note Locals will be automatically configured first, if necessary
            bool setVarByInt(const std::string& script, const std::string& var, int val);

            /// \note Locals will be automatically configured first, if necessary
            //
            // \note If it can not be determined if the variable exists, the error will be
            // ignored and false will be returned.
            bool hasVar(const std::string& script, const std::string& var);

            /// if var does not exist, returns 0
            /// @note var needs to be in lowercase
            ///
            /// \note Locals will be automatically configured first, if necessary
            int getIntVar (const std::string& script, const std::string& var);

            /// if var does not exist, returns 0
            /// @note var needs to be in lowercase
            ///
            /// \note Locals will be automatically configured first, if necessary
            float getFloatVar (const std::string& script, const std::string& var);

            /// \note If locals have not been configured yet, no data is written.
            ///
            /// \return Locals written?
            bool write (ESM::Locals& locals, const std::string& script) const;

            /// \note Locals will be automatically configured first, if necessary
            void read (const ESM::Locals& locals, const std::string& script);
    };
}

#endif
