#include <QSortFilterProxyModel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDataWidgetMapper>

#include "page.hpp"
#include "settingbox.hpp"
#include "view.hpp"
#include "booleanview.hpp"

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
    setLayout (new QGridLayout());

    mBox = new SettingBox (isHorizontal, false, this);

    layout()->addWidget(mBox);
}

void CSVSettings::Page::setupViews
                        (CSMSettings::DeclarationModel &declarationModel,
                         CSMSettings::DefinitionModel &definitionModel)
{
    QSortFilterProxyModel *filter = buildFilter
            (declarationModel, CSMSettings::Setting_Page, objectName());

    for (int i = 0; i < filter->rowCount(); i++)
    {
        QString settingName = filter->data (filter->index
                                    (i, CSMSettings::Setting_Name)).toString();

        const CSMSettings::Setting *setting =
           declarationModel.getSetting (objectName(), settingName);

        addView (definitionModel, setting);
    }
}

void CSVSettings::Page::addView (CSMSettings::DefinitionModel &model,
                                 const CSMSettings::Setting *setting)
{
    if (setting->viewType == ViewType_Undefined)
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
    mBox->layout()->addWidget (view->viewFrame());

    qDebug() << "Page::addView() definition count = " << model.rowCount();
}

void CSVSettings::Page::buildFactories()
{
    mViewFactories[ViewType_Boolean] = new BooleanViewFactory(this);
}

QSortFilterProxyModel *CSVSettings::Page::buildFilter
                                             (QAbstractItemModel &model,
                                              CSMSettings::SettingColumn column,
                                              const QString &expression)
{
    QSortFilterProxyModel *filter = new QSortFilterProxyModel (this);
    filter->setSourceModel (&model);
    filter->setFilterKeyColumn (column);
    filter->setFilterRegExp (expression);

    return filter;
}
