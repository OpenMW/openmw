#include <QSortFilterProxyModel>
#include <QDataWidgetMapper>
#include <QHBoxLayout>

#include "settingview.hpp"
#include "widgetfactory.hpp"
#include "apps/opencs/model/settings/setting.hpp"
#include "support.hpp"

#include <QDebug>

CSVSettings::SettingView::SettingView (const QString &viewName, WidgetType widgetType, bool isHorizontal, QWidget *parent) :
    QGroupBox(parent), mWidgetFactory (0), mSettingFilter (0), mDataAdapter (0)
 {
    setObjectName (viewName);
    setTitle (viewName);
    setupView (isHorizontal);
    makeFactory (widgetType);
 }

void CSVSettings::SettingView::buildWidgets ()
{


}

void CSVSettings::SettingView::setModel (QSortFilterProxyModel *settingModel)
{
    mSettingFilter = new QSortFilterProxyModel (this);
    mSettingFilter->setFilterFixedString (objectName());
    mSettingFilter->setFilterKeyColumn (0);
    mSettingFilter->setSourceModel(settingModel);
    mSettingFilter->setDynamicSortFilter (true);

    qDebug() << "building widget based on section: " << objectName() << "; records: " << mSettingFilter->rowCount();
    qDebug() << "record value: " << mSettingFilter->data(mSettingFilter->index(0,2,QModelIndex()), Qt::DisplayRole).toString();

    buildWidgets();
}

//view layout code


const QString CSVSettings::SettingView::INVISIBLE_BOX_STYLE =
        QString::fromUtf8("QSettingBox { border: 0px; padding 0px; margin: 0px;}");

void CSVSettings::SettingView::setupView(bool isHorizontal)
{
    setFlat (true);
    mVisibleBoxStyle = styleSheet();

    setStyleSheet (INVISIBLE_BOX_STYLE);

    if (isHorizontal)
        setLayout (new QHBoxLayout);
    else
        setLayout (new QVBoxLayout);
}

bool CSVSettings::SettingView::borderVisibile() const
{
    return (styleSheet() != INVISIBLE_BOX_STYLE);
}

void CSVSettings::SettingView::setTitle (const QString &title)
{
    if (borderVisibile() )
    {
        QGroupBox::setTitle (title);
        setMinimumWidth();
    }
}

void CSVSettings::SettingView::setBorderVisibility (bool value)
{
    if (value)
        setStyleSheet(mVisibleBoxStyle);
    else
        setStyleSheet(INVISIBLE_BOX_STYLE);
}

void CSVSettings::SettingView::setMinimumWidth()
{
    //set minimum width to accommodate title, if needed
    //1.5 multiplier to account for bold title.
    QFontMetrics fm (font());
    int minWidth = fm.width(title());
    QGroupBox::setMinimumWidth (minWidth * 1.5);
}
