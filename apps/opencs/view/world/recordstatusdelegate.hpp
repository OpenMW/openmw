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

        int mStatusDisplay;

        int mIconSize;
        int mIconTopOffset;
        int mTextLeftOffset;

    public:
        explicit RecordStatusDelegate(QUndoStack& undoStack, QObject *parent = 0);

        void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

        QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const;

    };

    class RecordStatusDelegateFactory : public CommandDelegateFactory
    {
            //std::vector<std::pair<int, QString> > mValues;

        public:

            virtual CommandDelegate *makeDelegate (QUndoStack& undoStack, QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.

    };
}
#endif // RECORDSTATUSDELEGATE_H

