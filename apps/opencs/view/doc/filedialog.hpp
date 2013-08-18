#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QDialog>
#include <QModelIndex>

#include "components/fileorderlist/contentselector.hpp"
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

class FileDialog : public FileOrderList::ContentSelector
{
    Q_OBJECT
public:
    explicit FileDialog(QWidget *parent = 0);

    void openFile();
    void newFile();
    void accepted();

    QString fileName();

signals:
    void openFiles();
    void createNewFile();
    
public slots:
    void accept();

private slots:
    //void updateViews();
    void updateOpenButton(const QStringList &items);
    void updateCreateButton(const QString &name);

    void createButtonClicked();

private:
    QLabel *mNameLabel;
    //LineEdit *mNameLineEdit;

    QPushButton *mCreateButton;
    QDialogButtonBox *mButtonBox;
};

#endif // FILEDIALOG_HPP
