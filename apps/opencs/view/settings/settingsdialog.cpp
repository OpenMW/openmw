#include "settingsdialog.hpp"

#include <QListWidgetItem>
#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QtGui>

#include "../../model/settings/usersettings.hpp"

//#include "ui_settingstab.h"

#include "page.hpp"

#include <QApplication>

#include <QSplitter>

#include <QTreeView>
#include <QListView>
#include <QTableView>

#include <QStandardItemModel>
#include <QStandardItem>
#include <OgreRoot.h>

#include <boost/math/common_factor.hpp>
#include <QMessageBox>
//#include <QTranslator>
#include <QDesktopWidget>

namespace
{
// copied from launcher
QString getAspect(int x, int y)
{
    int gcd = boost::math::gcd (x, y);
    int xaspect = x / gcd;
    int yaspect = y / gcd;
    // special case: 8 : 5 is usually referred to as 16:10
    if (xaspect == 8 && yaspect == 5)
        return QString("16:10");

    return QString(QString::number(xaspect) + ":" + QString::number(yaspect));
}

QRect getMaximumResolution()
{
    QRect max;
    int screens = QApplication::desktop()->screenCount();
    for(int i = 0; i < screens; ++i)
    {
        QRect res = QApplication::desktop()->screenGeometry(i);
        if(res.width() > max.width())
            max.setWidth(res.width());
        if(res.height() > max.height())
            max.setHeight(res.height());
    }
    return max;
}

QString getCurrentResolution()
{
    QString result;

    Ogre::ConfigOptionMap& renderOpt =
            Ogre::Root::getSingleton().getRenderSystem()->getConfigOptions();
    Ogre::ConfigOptionMap::iterator it = renderOpt.begin();
    for(;it != renderOpt.end(); ++it)
    {
        if(it->first == "Video Mode" )
        {
            QRegExp re("^(\\d+) x (\\d+)");
            int pos = re.indexIn(it->second.currentValue.c_str(), 0);
            if (pos > -1)
            {
                QString aspect = getAspect(re.cap(1).toInt(), re.cap(2).toInt());
                result = re.cap(1) + QString(" x ") + re.cap(2);
                if (aspect == QLatin1String("16:9") || aspect == QLatin1String("16:10"))
                {
                    //result.append(QObject::tr("\t(Wide ") + aspect + ")");

                }
                else if (aspect == QLatin1String("4:3"))
                {
                    //result.append(QObject::tr("\t(Standard 4:3)"));
                }
            }
        }
    }
    return result;
}

QStringList getAvailableResolutions()
{
    // store available rendering devices and available resolutions
    QStringList result;

    Ogre::ConfigOptionMap& renderOpt = Ogre::Root::getSingleton().getRenderSystem()->getConfigOptions();
    Ogre::ConfigOptionMap::iterator it = renderOpt.begin();
    for(;it != renderOpt.end(); ++it)
    {
        if(it->first == "Rendering Device" )
        {
            if(it->second.possibleValues.empty())
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle(QObject::tr("Error retrieving rendering device"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(QObject::tr("<br><b>Ogre Rendering Device empty<./b><br><br>"));
                msgBox.exec();
                return result;
            }
            //std::cout << "rd current: " << it->second.currentValue << std::endl; // FIXME: debug
            // Store Available Rendering Devices
            std::vector<std::string>::iterator iter = it->second.possibleValues.begin();
            for(;iter != it->second.possibleValues.end(); ++iter)
            {
                std::cout << "rd: " << *iter << std::endl; // FIXME: debug
            }
        }
        if(it->first == "Video Mode" )
        {
            if(it->second.possibleValues.empty())
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle(QObject::tr("Error receiving resolutions"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(QObject::tr("<br><b>Ogre Video Modes empty.</b><br><br>"));
                msgBox.exec();
                return result;
            }
            // FIXME: how to default to the current value?
            std::cout << "vm current: " << it->second.currentValue << std::endl; // FIXME: debug
            // Store Available Resolutions
            std::vector<std::string>::iterator iter = it->second.possibleValues.begin();
            for(;iter != it->second.possibleValues.end(); ++iter)
            {
                // extract x and y values
                QRegExp re("^(\\d+) x (\\d+)");
                int pos = re.indexIn((*iter).c_str(), 0);
                if (pos > -1)
                {
                    QString aspect = getAspect(re.cap(1).toInt(), re.cap(2).toInt());
                    QString resolution = re.cap(1) + QString(" x ") + re.cap(2);
                    if (aspect == QLatin1String("16:9") || aspect == QLatin1String("16:10")) {
                        resolution.append(QObject::tr("\t(Wide ") + aspect + ")");

                    } else if (aspect == QLatin1String("4:3")) {
                        resolution.append(QObject::tr("\t(Standard 4:3)"));
                    }
                    result.append(resolution);
                }
            }
        }
    }
    result.removeDuplicates();
    return result;
}

}

CSVSettings::SettingsDialog::SettingsDialog(QTabWidget *parent)
    : /*mStackedWidget (0), mDebugMode (false),*/ QTabWidget (parent)
{
    setObjectName("User Settings");

    setupUi(this);

    // Set the maximum res we can set in windowed mode
    QRect res = getMaximumResolution();
    spinBox_x->setMaximum(res.width());
    spinBox_y->setMaximum(res.height());

    //connect (mPageListWidget,
             //SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
             //this,
             //SLOT (slotChangePage (QListWidgetItem*, QListWidgetItem*)));
}

// FIXME: don't need this one, as we don't use pages
void CSVSettings::SettingsDialog::slotChangePage
                                (QListWidgetItem *cur, QListWidgetItem *prev)
{
   //mStackedWidget->changePage
                    //(mPageListWidget->row (cur), mPageListWidget->row (prev));

   layout()->activate();
   setFixedSize(minimumSizeHint());
}

// FIXME: don't need this one since we use setupUi
void CSVSettings::SettingsDialog::setupDialog()
{
    //create central widget with it's layout and immediate children
    QWidget *centralWidget = new QGroupBox (this);

    centralWidget->setLayout (new QHBoxLayout());
    centralWidget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred);
    //setCentralWidget (centralWidget);
    //setDockOptions (QMainWindow::AllowNestedDocks);

    buildPageListWidget (centralWidget);
    buildStackedWidget (centralWidget);
}

void CSVSettings::SettingsDialog::buildPages()
{
#if 0
    SettingWindow::createPages ();

    QFontMetrics fm (QApplication::font());

    foreach (Page *page, SettingWindow::pages())
    {
        QString pageName = page->objectName();

        //int textWidth = fm.width(pageName);

        //new QListWidgetItem (pageName, mPageListWidget);
        //mPageListWidget->setFixedWidth (textWidth + 50);

        //mStackedWidget->addWidget (&dynamic_cast<QWidget &>(*(page)));
    }

    //resize (mStackedWidget->sizeHint());
#endif
}

void CSVSettings::SettingsDialog::buildPageListWidget (QWidget *centralWidget)
{
    //mPageListWidget = new QListWidget (centralWidget);
    //mPageListWidget->setMinimumWidth(50);
    //mPageListWidget->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Expanding);

    //mPageListWidget->setSelectionBehavior (QAbstractItemView::SelectItems);

    //centralWidget->layout()->addWidget(mPageListWidget);
}

void CSVSettings::SettingsDialog::buildStackedWidget (QWidget *centralWidget)
{
    //mStackedWidget = new ResizeableStackedWidget (centralWidget);

    //centralWidget->layout()->addWidget (mStackedWidget);
}

void CSVSettings::SettingsDialog::closeEvent (QCloseEvent *event)
{
    //SettingWindow::closeEvent() must be called first to ensure
    //model is updated
    //SettingWindow::closeEvent (event);

    //saveSettings();
}

void CSVSettings::SettingsDialog::show()
{
    //if (pages().isEmpty())
    {
        //buildPages();
        //setViewValues();
    }

    if(CSMSettings::UserSettings::instance().settingValue("Video/use settings.cfg").toStdString() == "true")
    {
        label_RenderingSubsystem->setEnabled(false);
        comboBox_rendersystem->setEnabled(false);
        label_Antialiasing->setEnabled(false);
        comboBox_antialiasing->setEnabled(false);
        checkBox_vsync->setEnabled(false);
        label_ShaderLanguage->setEnabled(false);
        comboBox_shaderlanguage->setEnabled(false);

    }
    else
        checkBox_override->setChecked(false);

    if(CSMSettings::UserSettings::instance().settingValue("Video/fullscreen").toStdString() == "true")
    {
        checkBox_fullscreen->setChecked(true);

        label_Resolution->setEnabled(false);
        radioButton_standard_res->setEnabled(false);
        radioButton_custom_res->setEnabled(false);
        comboBox_std_window_size->setEnabled(false);
        spinBox_x->setEnabled(false);
        spinBox_y->setEnabled(false);
    }
    else
    {
        checkBox_fullscreen->setChecked(false);
    }

    // update display resolution combo box
    // FIXME: update opencs window size
    comboBox_std_window_size->clear();
    comboBox_std_window_size->addItems(getAvailableResolutions());
    int index = comboBox_std_window_size->findData(getCurrentResolution(),
                                                   Qt::DisplayRole,
                                                   Qt::MatchStartsWith);
    if(index != -1)
        comboBox_std_window_size->setCurrentIndex(index);

    // place the widget and make it visible
    QWidget *currView = QApplication::activeWindow();
    if(currView)
    {
        // place at the center of the window with focus
        QSize size = currView->size();
        move(currView->geometry().x()+(size.width() - frameGeometry().width())/2,
        currView->geometry().y()+(size.height() - frameGeometry().height())/2);
    }
    else
    {
        // something's gone wrong, place at the center of the screen
        QPoint screenCenter = QApplication::desktop()->screenGeometry().center();
        move(screenCenter - QPoint(frameGeometry().width()/2,
        frameGeometry().height()/2));
    }
    QWidget::show();
}
