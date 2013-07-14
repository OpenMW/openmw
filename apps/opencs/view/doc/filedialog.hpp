#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QDialog>
#include <QModelIndex>

#include "ui_datafilespage.h"

class QDialogButtonBox;
class QSortFilterProxyModel;
class QAbstractItemModel;
class QPushButton;
class QStringList;
class QString;
class QMenu;

class DataFilesModel;
class PluginsProxyModel;

class FileDialog : public QDialog, private Ui::DataFilesPage
{
    Q_OBJECT
public:
    explicit FileDialog(QWidget *parent = 0);
    void addFiles(const QString &path);
    void setEncoding(const QString &encoding);

    void openFile();
    void newFile();
    void accepted();

    QStringList checkedItemsPaths();
    QString fileName();

signals:
    void openFiles();
    void createNewFile();
    
public slots:
    void accept();

private slots:
    void updateViews();
    void updateOpenButton(const QStringList &items);
    void updateCreateButton(const QString &name);
    void setCheckState(QModelIndex index);

    void filterChanged(const QString &filter);

    void createButtonClicked();

private:
    QLabel *mNameLabel;
    LineEdit *mNameLineEdit;

    QPushButton *mCreateButton;
    QDialogButtonBox *mButtonBox;

    DataFilesModel *mDataFilesModel;

    PluginsProxyModel *mPluginsProxyModel;
    QSortFilterProxyModel *mMastersProxyModel;
    QSortFilterProxyModel *mFilterProxyModel;
};

#endif // FILEDIALOG_HPP
