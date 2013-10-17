#include <QSortFilterProxyModel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "settingpage.hpp"
#include "../../model/settings/settingmodel.hpp"
#include "settingbox.hpp"
#include "settingview.hpp"
#include "../../model/settings/setting.hpp"

CSVSettings::SettingPage::SettingPage(const QString &pageName, CSMSettings::SettingModel *model,
                                      bool isHorizontal, QWidget *parent) :
    QWidget(parent), mBox (new SettingBox (Orient_Horizontal, false, this))
{
    setObjectName(pageName);

    mSectionFilter = new CSMSettings::SectionFilter (this);
    mSectionFilter->setDynamicSortFilter (true);
    mSectionFilter->setSourceModel(model);
    mSectionFilter->setFilterRegExp(pageName);
    mSectionFilter->setFilterKeyColumn (1);

    CSVSettings::Orientation orientation = Orient_Horizontal;

    if (!isHorizontal)
        orientation = Orient_Vertical;

    mBox = new SettingBox (orientation, false, this);

    for (int i = 0; i < mSectionFilter->rowCount(); ++i)
    {
        const CSMSettings::Setting *setting = mSectionFilter->getSetting (i);
        addView (setting->widgetType(), setting->name(), setting->isHorizontal());
    }
}

void CSVSettings::SettingPage::addView (WidgetType widgetType, const QString &viewName, bool isHorizontal)
{
    SettingView *view = 0;

    QWidget *parentWidget = static_cast<QWidget *>(parent());

    view = new SettingView(viewName, widgetType, isHorizontal, parentWidget);

    if (!view)
        return;

    view->setModel (mSectionFilter);
    mViews.append (view);
    mBox->layout()->addWidget (view);
}
