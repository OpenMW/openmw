#ifndef SCRIPTEDIT_H
#define SCRIPTEDIT_H

#include <qtextedit.h>
#include <QVector>

#include "../../model/world/universalid.hpp"

class QWidget;

namespace CSVWorld
{
    class ScriptEdit : public QTextEdit
    {
            Q_OBJECT
        public:
            ScriptEdit (QWidget* parent);

        private:
            QVector<CSMWorld::UniversalId::Type> mAllowedTypes;

            void dragEnterEvent (QDragEnterEvent* event);

            void dropEvent (QDropEvent* event);

            void dragMoveEvent (QDragMoveEvent* event);
    };
}
#endif // SCRIPTEDIT_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
