#ifndef CSV_WORLD_SCRIPTHIGHLIGHTER_H
#define CSV_WORLD_SCRIPTHIGHLIGHTER_H

#include <map>
#include <string>

#include <QSyntaxHighlighter>

#include <components/compiler/nullerrorhandler.hpp>
#include <components/compiler/parser.hpp>
#include <components/compiler/extensions.hpp>

#include "../../model/world/scriptcontext.hpp"

namespace CSMPrefs
{
    class Setting;
}

namespace CSVWorld
{
    class ScriptHighlighter : public QSyntaxHighlighter, private Compiler::Parser
    {
        public:

            enum Type
            {
                Type_Int = 0,
                Type_Float = 1,
                Type_Name = 2,
                Type_Keyword = 3,
                Type_Special = 4,
                Type_Comment = 5,
                Type_Highlight = 6,
                Type_Id = 7
            };

            enum Mode
            {
                Mode_General,
                Mode_Console,
                Mode_Dialogue
            };

        private:

            Compiler::NullErrorHandler mErrorHandler;
            Compiler::Extensions mExtensions;
            CSMWorld::ScriptContext mContext;
            std::map<Type, QTextCharFormat> mScheme;
            Mode mMode;
            bool mMarkOccurrences;
            std::string mMarkedWord;

        private:

            bool parseInt (int value, const Compiler::TokenLoc& loc,
                Compiler::Scanner& scanner) override;
            ///< Handle an int token.
            /// \return fetch another token?

            bool parseFloat (float value, const Compiler::TokenLoc& loc,
                Compiler::Scanner& scanner) override;
            ///< Handle a float token.
            /// \return fetch another token?

            bool parseName (const std::string& name,
                const Compiler::TokenLoc& loc, Compiler::Scanner& scanner) override;
            ///< Handle a name token.
            /// \return fetch another token?

            bool parseKeyword (int keyword, const Compiler::TokenLoc& loc,
                Compiler::Scanner& scanner) override;
            ///< Handle a keyword token.
            /// \return fetch another token?

            bool parseSpecial (int code, const Compiler::TokenLoc& loc,
                Compiler::Scanner& scanner) override;
            ///< Handle a special character token.
            /// \return fetch another token?

            bool parseComment (const std::string& comment, const Compiler::TokenLoc& loc,
                Compiler::Scanner& scanner) override;
            ///< Handle comment token.
            /// \return fetch another token?

            void parseEOF (Compiler::Scanner& scanner) override;
            ///< Handle EOF token.

            void highlight (const Compiler::TokenLoc& loc, Type type);

        public:

            ScriptHighlighter (const CSMWorld::Data& data, Mode mode, QTextDocument *parent);

            void highlightBlock (const QString& text) override;

            void setMarkOccurrences(bool);

            void setMarkedWord(const std::string& name);

            void invalidateIds();

            bool settingChanged (const CSMPrefs::Setting *setting);
    };
}

#endif
