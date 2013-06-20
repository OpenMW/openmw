#include "recordstatusdelegate.hpp"
#include <QPainter>
#include <QApplication>
#include <QUndoStack>
#include "../../model/settings/usersettings.hpp"

CSVWorld::RecordStatusDelegate::RecordStatusDelegate(QUndoStack &undoStack, QObject *parent)
    : CommandDelegate (undoStack, parent)
{
    mModifiedIcon = new QIcon (":./modified.png");
    mAddedIcon = new QIcon (":./added.png");
    mDeletedIcon = new QIcon (":./removed.png");
    mBaseIcon = new QIcon (":./base.png");
    mIconSize = 16;

    //Offset values are most likely device-dependent.
    //Need to replace with device-independent references.
    mTextLeftOffset = 3;
    mIconTopOffset = -3;

    mStatusDisplay = 0;  //icons and text by default.  Remove when implemented as a user preference

    mFont = QApplication::font();
    mFont.setPointSize(10);

    mFontMetrics = new QFontMetrics(mFont);

    mTextAlignment.setAlignment (Qt::AlignLeft | Qt::AlignVCenter );
}

void CSVWorld::RecordStatusDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
   painter->save();

   QString text = "";
   QIcon *icon = 0;

   switch (index.data().toInt())
   {
   case 0: // State_BaseOnly
       text = "Base";
       icon = mBaseIcon;
       break;

   case 1: // State_Modified
       text = "Modified";
       icon = mModifiedIcon;
       break;

   case 2: // State_Modified_Only
       text = "Added";
       icon = mAddedIcon;
       break;

   case 3: // State_Deleted

   case 4: // State_Erased
       text = "Deleted";
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
        iconRect.setRight (iconRect.left()+ mIconSize*2);
        textRect.setLeft (iconRect.right() + mTextLeftOffset *1.25);
    }
    else
        textRect.setLeft (textRect.left() + mTextLeftOffset );

    if ( (mStatusDisplay == 0 || mStatusDisplay == 1) && (icon) )
        painter->drawPixmap(iconRect.center().x()-10,iconRect.center().y()+2, icon->pixmap(mIconSize, mIconSize));

   // icon + text or text only, or force text if no icon exists for status
   if (mStatusDisplay == 0 || mStatusDisplay == 2 || !(icon) )
   {
        painter->setFont(mFont);
        painter->drawText(textRect, text, mTextAlignment);
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

void CSVWorld::RecordStatusDelegate::updateEditorSetting (const QString &settingName, const QString &settingValue)
{
    if (settingName == "Record Status Display")
    {
        if (settingValue == "Icon and Text")
            mStatusDisplay = 0;

        else if (settingValue == "Icon Only")
            mStatusDisplay = 1;

        else if (settingValue == "Text Only")
            mStatusDisplay = 2;

        else
            mStatusDisplay = 0;
    }
}
