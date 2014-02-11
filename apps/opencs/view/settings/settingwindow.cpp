#include <QApplication>

#include "settingwindow.hpp"
#include "../../model/settings/settingmodel.hpp"
#include "../../model/settings/declarationmodel.hpp"
#include "../../model/settings/definitionmodel.hpp"
#include "page.hpp"

#include <QDebug>
CSVSettings::SettingWindow::SettingWindow(QWidget *parent) :
    QMainWindow(parent)
{
}

void CSVSettings::SettingWindow::createPages (CSMSettings::SettingModel &model)
{
    QStringList builtPages;

    //CSMSettings::DeclarationModel &decModel = model.declarationModel();
    //CSMSettings::DefinitionModel &defModel = model.definitionModel();

//    for (int i = 0; i < decModel.rowCount(); i++)
    {
       // QModelIndex idx = decModel.index (i, CSMSettings::Property_Page);
       // QString pageName = decModel.data (idx).toString();

   //     if (builtPages.contains (pageName))
   //         continue;

  //      builtPages << pageName;

     //   mPages << new Page (pageName, decModel, defModel, false, this);
    }
}

void CSVSettings::SettingWindow::closeEvent (QCloseEvent *event)
{
    QApplication::focusWidget()->clearFocus();
}
