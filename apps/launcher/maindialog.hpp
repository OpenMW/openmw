#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QStringListModel;
class QSettings;

class PlayPage;
class GraphicsPage;
class DataFilesPage;

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    MainDialog();

    //QStringListModel *mProfileModel;

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
    void play();


private:
    void createIcons();
    void createPages();
    void setupConfig();
    void closeEvent(QCloseEvent *event);

    QListWidget *mIconWidget;
    QStackedWidget *mPagesWidget;

    PlayPage *mPlayPage;
    GraphicsPage *mGraphicsPage;
    DataFilesPage *mDataFilesPage;

    QSettings *mGameConfig;
};

#endif
