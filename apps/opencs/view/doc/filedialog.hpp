#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QDialog>
#include <QModelIndex>

#include "components/contentselector/view/contentselector.hpp"
#include "ui_datafilespage.h"

class QDialogButtonBox;
class QSortFilterProxyModel;
class QAbstractItemModel;
class QPushButton;
class QStringList;
class QString;
class QMenu;
class QLabel;

class DataFilesModel;
class PluginsProxyModel;

namespace ContentSelectorView
{
    class LineEdit;
}

namespace CSVDoc
{
    class FileDialog : public ContentSelectorView::ContentSelector
    {
        Q_OBJECT
    public:
        explicit FileDialog(QWidget *parent = 0);

        void openFile();
        void newFile();

        QString fileName();

    signals:
        void openFiles();
        void createNewFile();

    public slots:

    private slots:
        //void updateViews();
        void updateOpenButton(const QStringList &items);
        void updateCreateButton(const QString &name);
    };
}
#endif // FILEDIALOG_HPP
