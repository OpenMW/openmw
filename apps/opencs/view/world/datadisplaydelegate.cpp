#include "datadisplaydelegate.hpp"
#include <QApplication>
#include <QPainter>

CSVWorld::DataDisplayDelegate::DataDisplayDelegate(const ValueList &values,
                                                   const IconList &icons,
                                                   QUndoStack &undoStack, QObject *parent)
    : EnumDelegate (values, undoStack, parent), mDisplayMode (Mode_TextOnly), mIcons (icons)
    , mIconSize (QSize(16, 16)), mIconLeftOffset(3), mTextLeftOffset(8)
{
    mTextAlignment.setAlignment (Qt::AlignLeft | Qt::AlignVCenter );

    buildPixmaps();
}

void CSVWorld::DataDisplayDelegate::buildPixmaps ()
{
    if (mPixmaps.size() > 0)
        mPixmaps.clear();

    IconList::iterator it = mIcons.begin();

    while (it != mIcons.end())
    {
        mPixmaps.push_back (std::make_pair (it->first, it->second.pixmap (mIconSize) ) );
        it++;
    }
}

void CSVWorld::DataDisplayDelegate::setIconSize(const QSize size)
{
    mIconSize = size;
    buildPixmaps();
}

void CSVWorld::DataDisplayDelegate::setIconLeftOffset(int offset)
{
    mIconLeftOffset = offset;
}

void CSVWorld::DataDisplayDelegate::setTextLeftOffset(int offset)
{
    mTextLeftOffset = offset;
}

void CSVWorld::DataDisplayDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    //default to enum delegate's paint method for text-only conditions
    if (mDisplayMode == Mode_TextOnly)
        EnumDelegate::paint(painter, option, index);
    else
    {
        unsigned int i = 0;

        for (; i < mValues.size(); ++i)
        {
            if (mValues.at(i).first == index.data().toInt())
                break;
        }

        if (i < mValues.size() )
            paintIcon (painter, option, i);
    }

    painter->restore();
}

void CSVWorld::DataDisplayDelegate::paintIcon (QPainter *painter, const QStyleOptionViewItem &option, int index) const
{
    //function-level statics
    QRect iconRect = option.rect;
    QRect textRect = iconRect;

    const QString &text = mValues.at(index).second;

    iconRect.setSize (mIconSize);
    iconRect.translate(mIconLeftOffset, (option.rect.height() - iconRect.height())/2);

    if (mDisplayMode == Mode_IconAndText )
    {
        textRect.translate (iconRect.width() + mTextLeftOffset, 0 );
        painter->drawText (textRect, text, mTextAlignment);
    }
    else
        iconRect.translate( (option.rect.width() - iconRect.width()) / 2, 0);

    painter->drawPixmap (iconRect, mPixmaps.at(index).second);
}

CSVWorld::DataDisplayDelegate::~DataDisplayDelegate()
{
    mIcons.clear();
    mPixmaps.clear();
}

void CSVWorld::DataDisplayDelegateFactory::add (int enumValue, QString enumName, QString iconFilename)
{
    mIcons.push_back (std::make_pair(enumValue, QIcon(iconFilename)));
    EnumDelegateFactory::add(enumValue, enumName);

}

CSVWorld::CommandDelegate *CSVWorld::DataDisplayDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{

    return new DataDisplayDelegate (mValues, mIcons, undoStack, parent);
}
