#include "usersettingsdialog.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>
#include <QTabWidget>
#include <QMessageBox>
#include <QTextCodec>
#include <QFile>
#include <QPushButton>
#include <QDockWidget>

#include "blankpage.hpp"
#include "editorpage.hpp"
#include "support.hpp"

#include "settingwidget.hpp"
#include <QDebug>

CsSettings::UserSettingsDialog::UserSettingsDialog(QMainWindow *parent) :
    QMainWindow (parent), mUserSettings (mCfgMgr), mStackedWidget (0)
{
    setWindowTitle(QString::fromUtf8 ("User Settings"));
    buildPages();
    setWidgetStates (loadSettings());
    positionWindow ();

    connect (mListWidget,
             SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
             this,
             SLOT (slotChangePage (QListWidgetItem*, QListWidgetItem*)));
}

CsSettings::UserSettingsDialog::~UserSettingsDialog()
{
}

void CsSettings::UserSettingsDialog::closeEvent (QCloseEvent *event)
{
    writeSettings();
}

void CsSettings::UserSettingsDialog::setWidgetStates (SectionMap settingsMap)
{
    //iterate the tabWidget's pages (sections)
    for (int i = 0; i < mStackedWidget->count(); i++)
    {
        //get the settings defined for the entire section
        CsSettings::SettingMap *settings = settingsMap [mStackedWidget->widget(i)->objectName()];

        //if found, initialize the page's widgets
        if (settings)
        {
            AbstractPage *page = getAbstractPage (i);
            page->initializeWidgets(*settings);
        }
    }
}

void CsSettings::UserSettingsDialog::buildPages()
{
    //craete central widget with it's layout and immediate children
    QWidget *centralWidget = new QWidget (this);

    mListWidget = new QListWidget (centralWidget);
    mStackedWidget = new QStackedWidget (centralWidget);

    QLayout* dialogLayout = new QHBoxLayout();

    dialogLayout->addWidget (mListWidget);
    dialogLayout->addWidget (mStackedWidget);

    centralWidget->setLayout (dialogLayout);

    setCentralWidget (centralWidget);
    setDockOptions (QMainWindow::AllowNestedDocks);
    //uncomment to test with sample editor page.
    //createSamplePage();
    createPage<BlankPage>("Page1");
    createPage<BlankPage>("Page2");
    createPage<BlankPage>("Page3");
}

void CsSettings::UserSettingsDialog::createSamplePage()
{
    //add pages to stackedwidget and items to listwidget
    CsSettings::AbstractPage *page
            = new CsSettings::EditorPage(this);

    mStackedWidget->addWidget (page);

    new QListWidgetItem (page->objectName(), mListWidget);

    connect ( page, SIGNAL ( signalUpdateEditorSetting (const QString &, const QString &)),
              this, SIGNAL ( signalUpdateEditorSetting (const QString &, const QString &)));
}

void CsSettings::UserSettingsDialog::positionWindow ()
{
    QRect scr = QApplication::desktop()->screenGeometry();

    move(scr.center().x() - (width() / 2), scr.center().y() - (height() / 2));

}

CsSettings::SectionMap CsSettings::UserSettingsDialog::loadSettings ()
{
    QString userPath = QString::fromStdString(mCfgMgr.getUserPath().string());

    mPaths.append(QString("opencs.cfg"));
    mPaths.append(userPath + QString("opencs.cfg"));

    SectionMap settingsMap;

    foreach (const QString &path, mPaths)
    {
        qDebug() << "Loading config file:" << qPrintable(path);
        QFile file(path);

        if (file.exists())
        {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenCS configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(QObject::tr("<br><b>Could not open %0 for reading</b><br><br> \
                                  Please make sure you have the right permissions \
                                  and try again.<br>").arg(file.fileName()));
                msgBox.exec();
                return settingsMap;
            }

            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            mUserSettings.getSettings(stream, settingsMap);
        }

        file.close();
    }

    return settingsMap;
}

void CsSettings::UserSettingsDialog::writeSettings()
{
    QMap<QString, SettingList *> settings;

    for (int i = 0; i < mStackedWidget->count(); ++i)
    {
        AbstractPage *page = getAbstractPage (i);
        settings [page->objectName()] = page->getSettings();
    }

    mUserSettings.writeFile(mUserSettings.openFile(mPaths.back()), settings);

}

CsSettings::AbstractPage *CsSettings::UserSettingsDialog::getAbstractPage (int index)
{
    return dynamic_cast<AbstractPage *>(mStackedWidget->widget(index));
}

void CsSettings::UserSettingsDialog::slotChangePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    if (!(current == previous))
        mStackedWidget->setCurrentIndex (mListWidget->row(current));
}
