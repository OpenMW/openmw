#ifndef CSM_TOOLS_SCRIPTCHECK_H
#define CSM_TOOLS_SCRIPTCHECK_H

#include <components/compiler/errorhandler.hpp>
#include <components/compiler/extensions.hpp>

#include "../doc/stage.hpp"

#include "../world/scriptcontext.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSMTools
{
    /// \brief VerifyStage: make sure that scripts compile
    class ScriptCheckStage : public CSMDoc::Stage, private Compiler::ErrorHandler
    {
            enum WarningMode
            {
                Mode_Ignore,
                Mode_Normal,
                Mode_Strict
            };

            const CSMDoc::Document& mDocument;
            Compiler::Extensions mExtensions;
            CSMWorld::ScriptContext mContext;
            std::string mId;
            std::string mFile;
            CSMDoc::Messages *mMessages;
            WarningMode mWarningMode;
            bool mIgnoreBaseRecords;

            CSMDoc::Message::Severity getSeverity (Type type);

            void report (const std::string& message, const Compiler::TokenLoc& loc, Type type) override;
            ///< Report error to the user.

            void report (const std::string& message, Type type) override;
            ///< Report a file related error

        public:

            ScriptCheckStage (const CSMDoc::Document& document);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
