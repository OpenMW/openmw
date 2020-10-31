#include "datadisplaydelegate.hpp"

#include "../../model/prefs/state.hpp"

#include <QApplication>
#include <QPainter>

CSVWorld::DataDisplayDelegate::DataDisplayDelegate(const ValueList &values,
                                                   const IconList &icons,
                                                   CSMWorld::CommandDispatcher *dispatcher,
                                                   CSMDoc::Document& document,
                                                   const std::string &pageName,
                                                   const std::string &settingName,
                                                   QObject *parent)
    : EnumDelegate (values, dispatcher, document, parent), mDisplayMode (Mode_TextOnly),
      mIcons (icons), mIconSize (QSize(16, 16)),
      mHorizontalMargin(QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1),
      mTextLeftOffset(8), mSettingKey (pageName + '/' + settingName)
{
    buildPixmaps();

    if (!pageName.empty())
        updateDisplayMode (CSMPrefs::get()[pageName][settingName].toString());
}

void CSVWorld::DataDisplayDelegate::buildPixmaps ()
{
    if (!mPixmaps.empty())
        mPixmaps.clear();

    IconList::iterator it = mIcons.begin();

    while (it != mIcons.end())
    {
        mPixmaps.emplace_back (it->mValue, it->mIcon.pixmap (mIconSize) );
        ++it;
    }
}

void CSVWorld::DataDisplayDelegate::setIconSize(const QSize& size)
{
    mIconSize = size;
    buildPixmaps();
}

void CSVWorld::DataDisplayDelegate::setTextLeftOffset(int offset)
{
    mTextLeftOffset = offset;
}

QSize CSVWorld::DataDisplayDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = EnumDelegate::sizeHint(option, index);

    int valueIndex = getValueIndex(index);
    if (valueIndex != -1)
    {
        if (mDisplayMode == Mode_IconOnly)
        {
            size.setWidth(mIconSize.width() + 2 * mHorizontalMargin);
        }
        else if (mDisplayMode == Mode_IconAndText)
        {
            size.setWidth(size.width() + mIconSize.width() + mTextLeftOffset);
        }

        if (mDisplayMode != Mode_TextOnly)
        {
            size.setHeight(qMax(size.height(), mIconSize.height()));
        }
    }
    return size;
}

void CSVWorld::DataDisplayDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    //default to enum delegate's paint method for text-only conditions
    if (mDisplayMode == Mode_TextOnly)
        EnumDelegate::paint(painter, option, index);
    else
    {
        int valueIndex = getValueIndex(index);
        if (valueIndex != -1)
        {
            paintIcon(painter, option, valueIndex);
        }
    }

    painter->restore();
}

void CSVWorld::DataDisplayDelegate::paintIcon (QPainter *painter, const QStyleOptionViewItem &option, int index) const
{
    QRect iconRect = option.rect;
    QRect textRect = iconRect;

    iconRect.setLeft(iconRect.left() + mHorizontalMargin);
    iconRect.setRight(option.rect.right() - mHorizontalMargin);
    if (mDisplayMode == Mode_IconAndText)
    {
        iconRect.setWidth(mIconSize.width());
        textRect.setLeft(iconRect.right() + mTextLeftOffset);
        textRect.setRight(option.rect.right() - mHorizontalMargin);

        QString text = option.fontMetrics.elidedText(mValues.at(index).second,
                                                     option.textElideMode,
                                                     textRect.width());
        QApplication::style()->drawItemText(painter,
                                            textRect,
                                            Qt::AlignLeft | Qt::AlignVCenter,
                                            option.palette,
                                            true,
                                            text);
    }
    QApplication::style()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, mPixmaps.at(index).second);
}

void CSVWorld::DataDisplayDelegate::updateDisplayMode (const std::string &mode)
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

void CSVWorld::DataDisplayDelegate::settingChanged (const CSMPrefs::Setting *setting)
{
    if (*setting==mSettingKey)
        updateDisplayMode (setting->toString());
}


void CSVWorld::DataDisplayDelegateFactory::add (int enumValue, const QString& enumName, const QString& iconFilename)
{
    EnumDelegateFactory::add(enumValue, enumName);

    Icon icon;
    icon.mValue = enumValue;
    icon.mName = enumName;
    icon.mIcon = QIcon(iconFilename);

    for (auto it=mIcons.begin(); it!=mIcons.end(); ++it)
    {
        if (it->mName > enumName)
        {
            mIcons.insert(it, icon);
            return;
        }
    }

    mIcons.push_back(icon);
}

CSVWorld::CommandDelegate *CSVWorld::DataDisplayDelegateFactory::makeDelegate (
    CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent) const
{
    return new DataDisplayDelegate (mValues, mIcons, dispatcher, document, "", "", parent);
}
