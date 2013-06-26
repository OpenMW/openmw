#ifndef RECORDSTATUSDELEGATE_H
#define RECORDSTATUSDELEGATE_H

#include "util.hpp"
#include <QTextOption>
#include <QFont>

class QIcon;
class QFont;
class QFontMetrics;

namespace CSVWorld
{
    class RecordStatusDelegate : public CommandDelegate
    {
        QFont mFont;
        QFontMetrics *mFontMetrics;

        QTextOption mTextAlignment;

        QIcon *mModifiedIcon;
        QIcon *mAddedIcon;
        QIcon *mDeletedIcon;
        QIcon *mBaseIcon;

        int mStatusDisplay;

        int mIconSize;
        int mIconTopOffset;
        int mTextLeftOffset;

    public:
        explicit RecordStatusDelegate(QUndoStack& undoStack, QObject *parent = 0);

        void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

        QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const;

        void updateEditorSetting (const QString &settingName, const QString &settingValue);

    };

    class RecordStatusDelegateFactory : public CommandDelegateFactory
    {
        public:

            virtual CommandDelegate *makeDelegate (QUndoStack& undoStack, QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.

    };
}
#endif // RECORDSTATUSDELEGATE_H

