#ifndef SCRIPTEDIT_H
#define SCRIPTEDIT_H

#include <QPlainTextEdit>
#include <QVector>
#include <QTimer>

#include "../../model/world/universalid.hpp"

#include "scripthighlighter.hpp"

class QWidget;
class QRegExp;

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class ScriptEdit : public QPlainTextEdit
    {
            Q_OBJECT

        public:

            class ChangeLock
            {
                    ScriptEdit& mEdit;

                    ChangeLock (const ChangeLock&);
                    ChangeLock& operator= (const ChangeLock&);

                public:

                    ChangeLock (ScriptEdit& edit);
                    ~ChangeLock();
            };

            friend class ChangeLock;

        private:

            int mChangeLocked;
            ScriptHighlighter *mHighlighter;
            QTimer mUpdateTimer;

        public:

            ScriptEdit (const CSMDoc::Document& document, ScriptHighlighter::Mode mode,
                QWidget* parent);

            /// Should changes to the data be ignored (i.e. not cause updated)?
            ///
            /// \note This mechanism is used to avoid infinite update recursions
            bool isChangeLocked() const;

        private:
            QVector<CSMWorld::UniversalId::Type> mAllowedTypes;
            const CSMDoc::Document& mDocument;
            const QRegExp mWhiteListQoutes;

            void dragEnterEvent (QDragEnterEvent* event);

            void dropEvent (QDropEvent* event);

            void dragMoveEvent (QDragMoveEvent* event);

            bool stringNeedsQuote(const std::string& id) const;

        private slots:

            void idListChanged();

            void updateHighlighting();
    };
}
#endif // SCRIPTEDIT_H
