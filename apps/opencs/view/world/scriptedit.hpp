#ifndef SCRIPTEDIT_H
#define SCRIPTEDIT_H

#include <QTextEdit>
#include <QVector>
#include <QTimer>

#include "../../model/world/universalid.hpp"

class QWidget;
class QRegExp;

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class ScriptHighlighter;

    class ScriptEdit : public QTextEdit
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

            ScriptEdit (const CSMDoc::Document& document, QWidget* parent);

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