#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QDialog>
#include <QModelIndex>

#include <boost/filesystem/path.hpp>

#ifndef CS_QT_BOOST_FILESYSTEM_PATH_DECLARED
#define CS_QT_BOOST_FILESYSTEM_PATH_DECLARED
Q_DECLARE_METATYPE (boost::filesystem::path)
#endif

#include "ui_filedialog.h"

class DataFilesModel;
class PluginsProxyModel;

namespace ContentSelectorView
{
    class ContentSelector;
}

namespace CSVDoc
{
    class FileWidget;
    class AdjusterWidget;

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
        AdjusterWidget *mAdjusterWidget;

    public:

        explicit FileDialog(QWidget *parent = 0);
        void showDialog (DialogType dialogType);

        void addFiles (const QString &path);

        QString filename() const;
        QStringList selectedFilePaths();

        void setLocalData (const boost::filesystem::path& localData);

    private:

        void buildNewFileView();
        void buildOpenFileView();

    signals:

        void signalOpenFiles (const boost::filesystem::path &path);
        void signalCreateNewFile (const boost::filesystem::path &path);

        void signalUpdateAcceptButton (bool, int);

    private slots:

        void slotNewFile();
        void slotOpenFile();
        void slotUpdateAcceptButton (int);
        void slotUpdateAcceptButton (const QString &, bool);
        void slotRejected();
    };
}
#endif // FILEDIALOG_HPP
