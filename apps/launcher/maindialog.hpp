#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QMainWindow>

#include <components/files/configurationmanager.hpp>
#include <components/settings/settings.hpp>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QStringList;
class QStringListModel;
class QString;

class PlayPage;
class GraphicsPage;
class DataFilesPage;

class MainDialog : public QMainWindow
{
    Q_OBJECT

public:
    MainDialog();

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
    void play();
    void profileChanged(int index);
    bool setup();

private:
    void createIcons();
    void createPages();
    void closeEvent(QCloseEvent *event);

    QListWidget *mIconWidget;
    QStackedWidget *mPagesWidget;

    PlayPage *mPlayPage;
    GraphicsPage *mGraphicsPage;
    DataFilesPage *mDataFilesPage;

    Files::ConfigurationManager mCfgMgr;
    Settings::Manager mSettings;
};

#endif
