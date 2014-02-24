#include <QSortFilterProxyModel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDataWidgetMapper>

#include "page.hpp"
#include "settingbox.hpp"
#include "view.hpp"
#include "booleanview.hpp"
#include "textview.hpp"

#include "../../model/settings/declarationmodel.hpp"
#include "../../model/settings/definitionmodel.hpp"
#include "../../model/settings/setting.hpp"

#include <QDebug>
#include <QRadioButton>
#include <QStandardItem>

QMap <CSVSettings::ViewType, CSVSettings::IViewFactory *>
                                            CSVSettings::Page::mViewFactories;

CSVSettings::Page::Page(const QString &pageName,
             const QList <CSMSettings::Setting> &settingList, QWidget *parent) :
    QWidget(parent)
{
    setObjectName (pageName);

    if (mViewFactories.size() == 0)
        buildFactories();

    setupPage ();
    setupViews (settingList);

}

void CSVSettings::Page::setupPage ()
{
    mBox = new SettingBox (false, "", this);

    SettingLayout *layout = new SettingLayout();
    layout->addWidget (mBox, 0, 0);

    setLayout (layout);
}

void CSVSettings::Page::setupViews
                            (const QList <CSMSettings::Setting> &settingList)
{
    //convert valuelist to a SelectionModel / QStringListModel
    //convert values to a QStringListModel
    //pass separate models to view

    foreach (const CSMSettings::Setting &pageSetting, settingList)
        addView (pageSetting);
}

void CSVSettings::Page::addView (const CSMSettings::Setting &setting)
{
   if (setting.viewType() == ViewType_Undefined)
        return;

    View *view = 0;

    IViewFactory *factory = mViewFactories[setting.viewType()];

    view = factory->createView(setting);

    if (!view)
        return;

    mViews.append (view);

    int viewRow = setting.viewRow();
    int viewCol = setting.viewColumn();

    if (viewRow == -1)
        viewRow = mViews.count() - 1;

    if (viewCol == -1)
        viewCol = 0;

    mBox->addWidget (view->viewFrame(), viewRow, viewCol);
}

void CSVSettings::Page::buildFactories()
{
    mViewFactories[ViewType_Boolean] = new BooleanViewFactory (this);
    mViewFactories[ViewType_Text] = new TextViewFactory (this);
}
