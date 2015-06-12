#ifndef CSV_WORLD_SCRIPTHIGHLIGHTER_H
#define CSV_WORLD_SCRIPTHIGHLIGHTER_H

#include <map>

#include <QSyntaxHighlighter>

#include <components/compiler/nullerrorhandler.hpp>
#include <components/compiler/parser.hpp>
#include <components/compiler/extensions.hpp>

#include "../../model/world/scriptcontext.hpp"

namespace CSVWorld
{
    class ScriptHighlighter : public QSyntaxHighlighter, private Compiler::Parser
    {
        public:

            enum Type
            {
                Type_Int,
                Type_Float,
                Type_Name,
                Type_Keyword,
                Type_Special,
                Type_Comment,
                Type_Id
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

        private:

            virtual bool parseInt (int value, const Compiler::TokenLoc& loc,
                Compiler::Scanner& scanner);
            ///< Handle an int token.
            /// \return fetch another token?

            virtual bool parseFloat (float value, const Compiler::TokenLoc& loc,
                Compiler::Scanner& scanner);
            ///< Handle a float token.
            /// \return fetch another token?

            virtual bool parseName (const std::string& name,
                const Compiler::TokenLoc& loc, Compiler::Scanner& scanner);
            ///< Handle a name token.
            /// \return fetch another token?

            virtual bool parseKeyword (int keyword, const Compiler::TokenLoc& loc,
                Compiler::Scanner& scanner);
            ///< Handle a keyword token.
            /// \return fetch another token?

            virtual bool parseSpecial (int code, const Compiler::TokenLoc& loc,
                Compiler::Scanner& scanner);
            ///< Handle a special character token.
            /// \return fetch another token?

            virtual bool parseComment (const std::string& comment, const Compiler::TokenLoc& loc,
                Compiler::Scanner& scanner);
            ///< Handle comment token.
            /// \return fetch another token?

            virtual void parseEOF (Compiler::Scanner& scanner);
            ///< Handle EOF token.

            void highlight (const Compiler::TokenLoc& loc, Type type);

        public:

            ScriptHighlighter (const CSMWorld::Data& data, Mode mode, QTextDocument *parent);

            virtual void highlightBlock (const QString& text);

            void invalidateIds();

            bool updateUserSetting (const QString &name, const QStringList &list);
    };
}

#endif
