#ifndef CONTENTSELECTOR_HPP
#define CONTENTSELECTOR_HPP

#include <QDialog>

#include "ui_datafilespage.h"

namespace EsxModel
{
    class DataFilesModel;
    class PluginsProxyModel;
    class MasterProxyModel;
}

class QSortFilterProxyModel;

namespace EsxView
{
    class ContentSelector : public QDialog, protected Ui::DataFilesPage
    {
        Q_OBJECT

    protected:

        EsxModel::DataFilesModel *mDataFilesModel;
        EsxModel::MasterProxyModel *mMasterProxyModel;
        EsxModel::PluginsProxyModel *mPluginsProxyModel;

    public:
        explicit ContentSelector(QWidget *parent = 0);

        void buildModelsAndViews();

        void addFiles(const QString &path);
        void setEncoding(const QString &encoding);
        void setPluginCheckState();
        void setCheckState(QModelIndex index, QSortFilterProxyModel *model);
        QStringList checkedItemsPaths();
        void on_checkAction_triggered();

    signals:
        void profileChanged(int index);

    private slots:
        void updateViews();
        void slotCurrentProfileIndexChanged(int index);
        void slotCurrentMasterIndexChanged(int index);
        void slotPluginTableItemClicked(const QModelIndex &index);
    };
}

#endif // CONTENTSELECTOR_HPP
