#include <QApplication>
#include <QStandardItemModel>

#include "../../model/settings/setting.hpp"
#include "../../model/settings/settingmanager.hpp"
#include "settingwindow.hpp"
#include "page.hpp"

#include <QDebug>
CSVSettings::SettingWindow::SettingWindow(QWidget *parent) :
    QMainWindow(parent)
{
}

void CSVSettings::SettingWindow::createPages
                                    (CSMSettings::SettingManager &manager)
{
    QStringList builtPages;

    for (int i = 0; i < manager.model().rowCount(); i++)
    {
        QString pageName = manager.model().item(i, CSMSettings::Property_Page)
                                            ->data(Qt::DisplayRole).toString();

        if (builtPages.contains (pageName))
            continue;

        QList <CSMSettings::Setting> settingList = manager.getSettings(pageName);

        mPages.append (new Page (pageName, settingList, this));

        builtPages.append (pageName);

    }
}

void CSVSettings::SettingWindow::closeEvent (QCloseEvent *event)
{
    QApplication::focusWidget()->clearFocus();
}

QSortFilterProxyModel *CSVSettings::SettingWindow::buildFilter
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
