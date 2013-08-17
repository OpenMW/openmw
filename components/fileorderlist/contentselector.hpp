#ifndef CONTENTSELECTOR_HPP
#define CONTENTSELECTOR_HPP

#include <QWidget>

#include "ui_datafilespage.h"

class DataFilesModel;
class PluginsProxyModel;
class QSortFilterProxyModel;

namespace FileOrderList
{
    class MasterProxyModel;

    class ContentSelector : public QWidget, protected Ui::DataFilesPage
    {
        Q_OBJECT

    protected:

        DataFilesModel *mDataFilesModel;
        MasterProxyModel *mMasterProxyModel;
        PluginsProxyModel *mPluginsProxyModel;

    public:
        explicit ContentSelector(QWidget *parent = 0);

        void buildModelsAndViews();

        void addFiles(const QString &path);
        void setEncoding(const QString &encoding);
        void setCheckState(QModelIndex index);
        QStringList checkedItemsPaths();

    private slots:
        void updateViews();
    };
}

#endif // CONTENTSELECTOR_HPP
