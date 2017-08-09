#ifndef TEXT_SLOT_MSG_BOX
#define TEXT_SLOT_MSG_BOX

#include <QMessageBox>

namespace Launcher
{
    class TextSlotMsgBox : public QMessageBox
    {
    Q_OBJECT
        public slots:
            void setTextSlot(const QString& string);
    };
}
#endif
