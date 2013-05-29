#include "recordstatusdelegate.hpp"
#include <QPainter>
#include <QApplication>
#include <QFont>
#include <QUndoStack>

#include <QDebug>

CSVWorld::RecordStatusDelegate::RecordStatusDelegate(QUndoStack &undoStack, QObject *parent)
    : CommandDelegate (undoStack, parent)
{
    mModifiedIcon = new QIcon (":./modified.png");
    mAddedIcon = new QIcon (":./added.png");
    mDeletedIcon = new QIcon (":./removed.png");
    mIconSize = 16;

    //Offset values are most likely device-dependent.
    //Need to replace with device-independent references.
    mTextTopOffset = -1;
    mTextLeftOffset = 3;
    mIconTopOffset = -3;
    mIconLeftOffset = 0;

    mStatusDisplay = 0;  //icons and text by default
}

void CSVWorld::RecordStatusDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
   painter->save();

   QFont font = QApplication::font();
   font.setPointSize(10);

   QFontMetrics fm(font);

   QString text = "";
   QIcon *icon = 0;

   switch (index.data().toInt())
   {
   case 0: // State_BaseOnly
       text = "base";
       break;

   case 1: // State_Modified
       text = "modified";
       icon = mModifiedIcon;
       break;

   case 2: // State_Modified_Only
       text = "added";
       icon = mAddedIcon;
       break;

   case 3: // State_Deleted

   case 4: // State_Erased
       text = "deleted";
       icon = mDeletedIcon;
       break;

   default:
       break;
   }

   QRect textRect = option.rect;
   QRect iconRect = option.rect;

   //for icon-only (1), default option.rect centers icon left-to-right
   //otherwise, size option.rect to fit the icon, forcing left-alignment with text
    iconRect.setTop (iconRect.top() + mIconTopOffset);
    iconRect.setBottom (iconRect.top() + mIconSize);

    if (mStatusDisplay == 0 && (icon) )
    {
        iconRect.setRight (iconRect.left() + mIconSize);
        textRect.setLeft (iconRect.right() + (mIconSize/4) * 3);
        textRect.setRight (textRect.left() + fm.width(text));
    }
    else
        textRect.setLeft (textRect.left() + mTextLeftOffset );

    textRect.setTop (textRect.top() + ((option.rect.height() - fm.height()) / 2) + mTextTopOffset);

   if (mStatusDisplay == 0 || mStatusDisplay == 1)
   {
        if (icon)
           painter->drawPixmap(iconRect.center(),icon->pixmap(mIconSize, mIconSize));
   }

   // icon + text or text only, or force text if no icon exists for status
   if (mStatusDisplay == 0 || mStatusDisplay == 2 || !(icon) )
   {
        painter->setFont(font);
        painter->drawText(textRect,text);
   }

   painter->restore();

}

QSize CSVWorld::RecordStatusDelegate::sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize();
}

CSVWorld::CommandDelegate *CSVWorld::RecordStatusDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{
    return new RecordStatusDelegate (undoStack, parent);
}
