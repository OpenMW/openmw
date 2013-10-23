#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QDialog>
#include <QModelIndex>

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
        void createNewFile();

        void signalUpdateCreateButton (bool, int);
        void signalUpdateCreateButtonFlags(int);

    public slots:

    private slots:

        void slotUpdateCreateButton (int);
        void slotUpdateCreateButton (const QString &, bool);
    };
}
#endif // FILEDIALOG_HPP
