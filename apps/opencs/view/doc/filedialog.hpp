#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QDialog>
#include <QModelIndex>
#include "../../../../components/contentselector/view/contentselector.hpp"

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
    class FileDialog : public QDialog
    {
        Q_OBJECT

        unsigned char mOpenFileFlags;
        unsigned char mNewFileFlags;

    public:
        explicit FileDialog(QWidget *parent = 0);

        void openFile();
        void newFile();
        void addFiles (const QString &path);

        QString filename();
        QStringList selectedFilepaths();

    private:

        void showDialog();

    signals:

        void openFiles();
        void createNewFile();

    public slots:

        void slotRejected();

    private slots:

    };
}
#endif // FILEDIALOG_HPP
