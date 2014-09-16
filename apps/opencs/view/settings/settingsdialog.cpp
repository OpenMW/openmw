#include "settingsdialog.hpp"

#include <boost/math/common_factor.hpp>
#include <OgreRoot.h>

#include <QDesktopWidget>

#include <components/contentselector/model/naturalsort.hpp>
#include "../../model/settings/usersettings.hpp"

namespace
{
// copied from the launcher & adapted

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
    Ogre::ConfigOptionMap& renderOpt =
            Ogre::Root::getSingleton().getRenderSystem()->getConfigOptions();
    Ogre::ConfigOptionMap::iterator it = renderOpt.begin();
    for(; it != renderOpt.end(); ++it)
    {
        if(it->first == "Video Mode" )
        {
            QRegExp re("^(\\d+ x \\d+)");
            int pos = re.indexIn(it->second.currentValue.c_str(), 0);
            if (pos > -1)
                return re.cap(1);
        }
    }
    return QString(); // found nothing
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
                return result;  // FIXME: add error message
            }
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
                return result;  // FIXME: add error message
            }
            // FIXME: how to default to the current value?
            std::cout << "vm current: " << it->second.currentValue << std::endl; // FIXME: debug
            // Store Available Resolutions
            std::vector<std::string>::iterator iter = it->second.possibleValues.begin();
            for(; iter != it->second.possibleValues.end(); ++iter)
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

QStringList getAvailableOptions(const QString &key)
{
    QStringList result;

    Ogre::ConfigOptionMap& renderOpt =
        Ogre::Root::getSingleton().getRenderSystem()->getConfigOptions();
    Ogre::ConfigOptionMap::iterator it = renderOpt.begin();

    uint row = 0;
    for(; it != renderOpt.end(); ++it, ++row)
    {
        Ogre::StringVector::iterator opt_it = it->second.possibleValues.begin();
        uint idx = 0;

        for(; opt_it != it->second.possibleValues.end(); ++opt_it, ++idx)
        {
            if(strcmp (key.toStdString().c_str(), it->first.c_str()) == 0)
            {
                result << ((key == "FSAA") ? QString("MSAA ") : QString(""))
                    + QString::fromStdString((*opt_it).c_str()).simplified();
            }
        }
    }

    // Sort ascending
    qSort(result.begin(), result.end(), naturalSortLessThanCI);

    // Replace the zero option with Off
    int index = result.indexOf("MSAA 0");

    if(index != -1)
        result.replace(index, QObject::tr("Off"));

    return result;
}

}

CSVSettings::SettingsDialog::SettingsDialog(QTabWidget *parent)
    : /*mDebugMode (false),*/ QTabWidget (parent)
{
    setObjectName("User Settings");

    setupUi(this);

    // Set the maximum res we can set in windowed mode
    QRect res = getMaximumResolution();
    spinBox_x->setMaximum(res.width());
    spinBox_y->setMaximum(res.height());

    connect(comboBox_rendersystem, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(rendererChanged(const QString&)));
    connect(radioButton_standard_res, SIGNAL(toggled(bool)), this, SLOT(slotStandardToggled(bool)));
}

void CSVSettings::SettingsDialog::rendererChanged()
{
    comboBox_antialiasing->clear();
    comboBox_antialiasing->addItems(getAvailableOptions(QString("FSAA")));
}

// FIXME: what to do with updating window size
void CSVSettings::SettingsDialog::slotStandardToggled(bool checked)
{
    if (checked)
    {
        comboBox_std_window_size->setEnabled(true);
        spinBox_x->setEnabled(false);
        spinBox_y->setEnabled(false);
    }
    else
    {
        comboBox_std_window_size->setEnabled(false);
        spinBox_x->setEnabled(true);
        spinBox_y->setEnabled(true);
    }
}

void CSVSettings::SettingsDialog::setViewValues()
{
    //rendererChanged(Ogre::Root::getSingleton().getRenderSystemByName(renderer.toStdString()));
    rendererChanged(); // setup antialiasing options

    if(CSMSettings::UserSettings::instance().settingValue("Video/use settings.cfg") == "true")
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

    if(CSMSettings::UserSettings::instance().settingValue("Window Size/Width") != "")
    {
        spinBox_x->setValue(
            CSMSettings::UserSettings::instance().settingValue("Window Size/Width").toInt());
    }

    if(CSMSettings::UserSettings::instance().settingValue("Window Size/Height") != "")
    {
        spinBox_y->setValue(
            CSMSettings::UserSettings::instance().settingValue("Window Size/Height").toInt());
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

    slotStandardToggled(radioButton_standard_res->isChecked() ? true : false);
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
    setViewValues();

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
