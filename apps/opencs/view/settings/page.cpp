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

QMap <CSVSettings::ViewType, CSVSettings::IViewFactory *>
                                            CSVSettings::Page::mViewFactories;

CSVSettings::Page::Page(const QString &pageName,
                        CSMSettings::DeclarationModel &declarationModel,
                        CSMSettings::DefinitionModel &definitionModel,
                        bool isHorizontal, QWidget *parent) :
    QWidget(parent)
{
    setObjectName (pageName);

    if (mViewFactories.size() == 0)
        buildFactories();

    setupPage (isHorizontal);
    setupViews (declarationModel, definitionModel);

}

void CSVSettings::Page::setupPage (bool isHorizontal)
{
    mBox = new SettingBox (false, "", this);

    SettingLayout *layout = new SettingLayout();
    layout->addWidget (mBox, 0, 0);

    setLayout (layout);
}

void CSVSettings::Page::setupViews
                        (CSMSettings::DeclarationModel &declarationModel,
                         CSMSettings::DefinitionModel &definitionModel)
{
    QSortFilterProxyModel *filter = buildFilter
            (declarationModel, CSMSettings::Property_Page, objectName());

    for (int i = 0; i < filter->rowCount(); i++)
    {
        QString settingName = filter->data (filter->index
                                    (i, CSMSettings::Property_Name)).toString();

        const CSMSettings::Setting *setting =
           declarationModel.getSetting (objectName(), settingName);

        addView (definitionModel, setting);
    }
}

void CSVSettings::Page::addView (CSMSettings::DefinitionModel &model,
                                 const CSMSettings::Setting *setting)
{
   /* if (setting->viewType == ViewType_Undefined)
        return;

    View *view = 0;

    IViewFactory *factory = mViewFactories[setting->viewType];

    view = factory->createView(model,setting);

    if (!view)
    {
        qDebug() << "view creation failed - " << setting->settingName;
        return;
    }

    mViews.append (view);

    int viewRow = setting->viewRow;
    int viewCol = setting->viewColumn;

    if (viewRow == -1)
        viewRow = mViews.count() - 1;

    if (viewCol == -1)
        viewCol = 0;

    mBox->addWidget (view->viewFrame(), viewRow, viewCol);*/
}

void CSVSettings::Page::buildFactories()
{
    mViewFactories[ViewType_Boolean] = new BooleanViewFactory (this);
    mViewFactories[ViewType_Text] = new TextViewFactory (this);
}

QSortFilterProxyModel *CSVSettings::Page::buildFilter
                                             (QAbstractItemModel &model,
                                              CSMSettings::SettingProperty column,
                                              const QString &expression)
{
    QSortFilterProxyModel *filter = new QSortFilterProxyModel (this);
    filter->setSourceModel (&model);
    filter->setFilterKeyColumn (column);
    filter->setFilterRegExp (expression);

    return filter;
}
