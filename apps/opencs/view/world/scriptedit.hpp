#ifndef SCRIPTEDIT_H
#define SCRIPTEDIT_H

#include <qtextedit.h>
#include <QVector>

#include "../../model/world/universalid.hpp"

class QWidget;
class QRegExp;

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class ScriptEdit : public QTextEdit
    {
            Q_OBJECT
        public:
            ScriptEdit (QWidget* parent, const CSMDoc::Document& document);

        private:
            QVector<CSMWorld::UniversalId::Type> mAllowedTypes;
            const CSMDoc::Document& mDocument;
            const QRegExp mWhiteListQoutes;

            void dragEnterEvent (QDragEnterEvent* event);

            void dropEvent (QDropEvent* event);

            void dragMoveEvent (QDragMoveEvent* event);

            bool stringNeedsQuote(const std::string& id) const;
    };
}
#endif // SCRIPTEDIT_H