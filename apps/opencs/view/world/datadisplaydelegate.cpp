#include "datadisplaydelegate.hpp"
#include "../../model/settings/usersettings.hpp"

#include <QApplication>
#include <QPainter>

CSVWorld::DataDisplayDelegate::DataDisplayDelegate(const ValueList &values,
                                                   const IconList &icons,
                                                   CSMWorld::CommandDispatcher *dispatcher,
                                                   CSMDoc::Document& document,
                                                   const QString &pageName,
                                                   const QString &settingName,
                                                   QObject *parent)
    : EnumDelegate (values, dispatcher, document, parent), mDisplayMode (Mode_TextOnly),
      mIcons (icons), mIconSize (QSize(16, 16)), mIconLeftOffset(3),
      mTextLeftOffset(8), mSettingKey (pageName + '/' + settingName)
{
    mTextAlignment.setAlignment (Qt::AlignLeft | Qt::AlignVCenter );

    buildPixmaps();

    QString value =
            CSMSettings::UserSettings::instance().settingValue (mSettingKey);

    updateDisplayMode(value);
}

void CSVWorld::DataDisplayDelegate::buildPixmaps ()
{
    if (!mPixmaps.empty())
        mPixmaps.clear();

    IconList::iterator it = mIcons.begin();

    while (it != mIcons.end())
    {
        mPixmaps.push_back (std::make_pair (it->first, it->second.pixmap (mIconSize) ) );
        ++it;
    }
}

void CSVWorld::DataDisplayDelegate::setIconSize(const QSize& size)
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

void CSVWorld::DataDisplayDelegate::updateUserSetting (const QString &name,
                                                        const QStringList &list)
{
    if (list.isEmpty())
        return;

    QString value = list.at(0);

    if (name == mSettingKey)
        updateDisplayMode (value);
}

void CSVWorld::DataDisplayDelegate::updateDisplayMode (const QString &mode)
{
    if (mode == "Icon and Text")
        mDisplayMode = Mode_IconAndText;

    else if (mode == "Icon Only")
        mDisplayMode = Mode_IconOnly;

    else if (mode == "Text Only")
        mDisplayMode = Mode_TextOnly;
}

CSVWorld::DataDisplayDelegate::~DataDisplayDelegate()
{
}

void CSVWorld::DataDisplayDelegateFactory::add (int enumValue, QString enumName, QString iconFilename)
{
    mIcons.push_back (std::make_pair(enumValue, QIcon(iconFilename)));
    EnumDelegateFactory::add(enumValue, enumName);

}

CSVWorld::CommandDelegate *CSVWorld::DataDisplayDelegateFactory::makeDelegate (
    CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent) const
{
    return new DataDisplayDelegate (mValues, mIcons, dispatcher, document, "", "", parent);
}


