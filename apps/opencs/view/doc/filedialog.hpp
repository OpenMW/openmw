#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QDialog>
#include <QModelIndex>

<<<<<<< HEAD
#include "ui_filedialog.h"
=======
#include <boost/filesystem/path.hpp>

#include "components/contentselector/view/contentselector.hpp"
#include "ui_datafilespage.h"

#ifndef CS_QT_BOOST_FILESYSTEM_PATH_DECLARED
#define CS_QT_BOOST_FILESYSTEM_PATH_DECLARED
Q_DECLARE_METATYPE (boost::filesystem::path)
#endif

class QDialogButtonBox;
class QSortFilterProxyModel;
class QAbstractItemModel;
class QPushButton;
class QStringList;
class QString;
class QMenu;
class QLabel;
>>>>>>> 3146af34d642a28b15b55f7eb9999d8ac50168a0

class DataFilesModel;
class PluginsProxyModel;

namespace ContentSelectorView
{
    class ContentSelector;
}

namespace CSVDoc
{
    class FileWidget;

    class FileDialog : public QDialog
    {
        Q_OBJECT

    public:

        enum DialogType
        {
            DialogType_New,
            DialogType_Open
        };

    private:

        ContentSelectorView::ContentSelector *mSelector;
        Ui::FileDialog ui;
        DialogType mDialogType;
        FileWidget *mFileWidget;

    public:

        explicit FileDialog(QWidget *parent = 0);
        void showDialog (DialogType dialogType);

        void addFiles (const QString &path);

        QString filename() const;
        QStringList selectedFilePaths();

    private:

        void buildNewFileView();
        void buildOpenFileView();

    signals:

        void openFiles();
        void createNewFile ();

        void signalUpdateCreateButton (bool, int);
        void signalUpdateCreateButtonFlags(int);

    private slots:

        void slotUpdateCreateButton (int);
        void slotUpdateCreateButton (const QString &, bool);
        void slotRejected();
    };
}
#endif // FILEDIALOG_HPP
