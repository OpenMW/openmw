#include <QSortFilterProxyModel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "settingpage.hpp"
#include "../../model/settings/settingmodel.hpp"
#include "settingbox.hpp"
#include "settingview.hpp"
#include "../../model/settings/setting.hpp"

#include <QDataWidgetMapper>

CSVSettings::SettingPage::SettingPage(const QString &pageName, CSMSettings::SettingModel *model,
                                      bool isHorizontal, QWidget *parent) :
    QWidget(parent), mBox (new SettingBox (Orient_Horizontal, false, this))
{
    setObjectName(pageName);

    mSectionFilter = new CSMSettings::SectionFilter (this);
    mSectionFilter->setSourceModel(model);
    mSectionFilter->setFilterRegExp(pageName);
    mSectionFilter->setFilterKeyColumn (1);
    mSectionFilter->setDynamicSortFilter (true);

    CSVSettings::Orientation orientation = Orient_Horizontal;

    if (!isHorizontal)
        orientation = Orient_Vertical;

    mBox = new SettingBox (orientation, false, this);

    for (int i = 0; i < mSectionFilter->rowCount(); ++i)
    {
        addView (mSectionFilter->getSetting (i));
    }
}

void CSVSettings::SettingPage::addView (const CSMSettings::Setting *setting)
{
    SettingView *view = 0;

    QWidget *parentWidget = static_cast<QWidget *>(parent());

    view = new SettingView(setting, this);

    if (!view)
        return;

    view->setModel (mSectionFilter);
    mViews.append (view);
    mBox->layout()->addWidget (view);
}
