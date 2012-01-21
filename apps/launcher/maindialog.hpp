#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>

#include <components/files/configurationmanager.hpp>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QStringList;
class QStringListModel;
class QString;

class PlayPage;
class GraphicsPage;
class DataFilesPage;

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    MainDialog();

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
    void play();
    void profileChanged(int index);


private:
    void createIcons();
    void createPages();
    void writeConfig();
    void closeEvent(QCloseEvent *event);

    QStringList readConfig(const QString &fileName);

    QListWidget *mIconWidget;
    QStackedWidget *mPagesWidget;

    PlayPage *mPlayPage;
    GraphicsPage *mGraphicsPage;
    DataFilesPage *mDataFilesPage;

    QStringList mDataDirs;
    bool mStrict;

    Files::ConfigurationManager mCfgMgr;
};

#endif
