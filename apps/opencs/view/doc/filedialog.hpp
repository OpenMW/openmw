#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QDialog>

#ifndef Q_MOC_RUN

#include "adjusterwidget.hpp"

#ifndef CS_QT_STD_FILESYSTEM_PATH_DECLARED
#define CS_QT_STD_FILESYSTEM_PATH_DECLARED
Q_DECLARE_METATYPE(std::filesystem::path)
#endif

#endif

#include <filesystem>
#include <vector>

class QModelIndex;

namespace ContentSelectorView
{
    class ContentSelector;
}

namespace Ui
{
    class FileDialog;
}

namespace CSVDoc
{
    class FileWidget;

    class FileDialog : public QDialog
    {
        Q_OBJECT

    private:
        ContentSelectorView::ContentSelector* mSelector;
        std::unique_ptr<Ui::FileDialog> ui;
        ContentAction mAction;
        FileWidget* mFileWidget;
        AdjusterWidget* mAdjusterWidget;
        bool mDialogBuilt;

    public:
        explicit FileDialog(QWidget* parent = nullptr);

        ~FileDialog();

        void showDialog(ContentAction action);

        void addFiles(const std::vector<std::filesystem::path>& dataDirs);
        void setEncoding(const QString& encoding);
        void clearFiles();

        QString filename() const;
        QStringList selectedFilePaths();

        void setLocalData(const std::filesystem::path& localData);

    private:
        void buildNewFileView();
        void buildOpenFileView();

    signals:

        void signalOpenFiles(const std::filesystem::path& path);
        void signalCreateNewFile(const std::filesystem::path& path);

        void signalUpdateAcceptButton(bool, int);

    private slots:

        void slotNewFile();
        void slotOpenFile();
        void slotUpdateAcceptButton(int);
        void slotUpdateAcceptButton(const QString&, bool);
        void slotRejected();
        void slotAddonDataChanged(const QModelIndex& topleft, const QModelIndex& bottomright);
    };
}
#endif // FILEDIALOG_HPP
