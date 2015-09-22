#ifndef CSV_WORLD_SCRIPTERRORTABLE_H
#define CSV_WORLD_SCRIPTERRORTABLE_H

#include <QTableWidget>

#include <components/compiler/errorhandler.hpp>
#include <components/compiler/extensions.hpp>

#include "../../model/world/scriptcontext.hpp"
#include "../../model/doc/messages.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class ScriptErrorTable : public QTableWidget, private Compiler::ErrorHandler
    {
            Q_OBJECT

            Compiler::Extensions mExtensions;
            CSMWorld::ScriptContext mContext;

            virtual void report (const std::string& message, const Compiler::TokenLoc& loc, Type type);
            ///< Report error to the user.

            virtual void report (const std::string& message, Type type);
            ///< Report a file related error

            void addMessage (const std::string& message, CSMDoc::Message::Severity severity,
                int line = -1, int column = -1);

            void setWarningsMode (const QString& value);

        public:

            ScriptErrorTable (const CSMDoc::Document& document, QWidget *parent = 0);

            void updateUserSetting (const QString& name, const QStringList& value);

            void update (const std::string& source);

            void clear();

            /// Clear local variable cache for \a script.
            ///
            /// \return Were there any locals that needed clearing?
            bool clearLocals (const std::string& script);

        private slots:

            void cellClicked (int row, int column);

        signals:

            void highlightError (int line, int column);
    };
}

#endif
