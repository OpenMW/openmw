#include "settingsdialog.hpp"

#include <boost/math/common_factor.hpp>
#include <OgreRoot.h>

#include <QDesktopWidget>

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

QString getCurrentOgreResolution()
{
    Ogre::ConfigOptionMap& renderOpt =
            Ogre::Root::getSingleton().getRenderSystem()->getConfigOptions();
    Ogre::ConfigOptionMap::iterator it = renderOpt.begin();
    for(; it != renderOpt.end(); ++it)
    {
        if(it->first == "Video Mode" )
        {
            QRegExp re("^(\\d+ x \\d+)");
            if (re.indexIn(it->second.currentValue.c_str(), 0) > -1)
                return re.cap(1);
        }
    }
    return QString(); // found nothing
}

bool customCompare(const QString &s1, const QString &s2)
{
    int x1, x2, y1, y2;
    QRegExp re("^(\\d+) x (\\d+)");

    if(re.indexIn(s1) > -1)
    {
        x1 = re.cap(1).toInt();
        y1 = re.cap(2).toInt();
    }
    if(re.indexIn(s2) > -1)
    {
        x2 = re.cap(1).toInt();
        y2 = re.cap(2).toInt();
    }

    if(x1 == x2)
        return y1 > y2;
    else
        return x1 > x2;
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
                if(re.indexIn((*iter).c_str(), 0) > -1)
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
    qStableSort(result.begin(), result.end(), customCompare);
    return result;
}

}

CSVSettings::SettingsDialog::SettingsDialog(QTabWidget *parent)
    : QTabWidget (parent)
{
    setObjectName("User Settings");

    setupUi(this);

    // Set the maximum res we can set in windowed mode
    QRect res = getMaximumResolution();
    spinBox_x->setMaximum(res.width());
    spinBox_y->setMaximum(res.height());

    connect(checkBox_override, SIGNAL(toggled(bool)), this, SLOT(slotOverrideToggled(bool)));
    connect(comboBox_rendersystem, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slotRendererChanged(const QString&)));
    connect(radioButton_standard_res, SIGNAL(toggled(bool)), this, SLOT(slotStandardToggled(bool)));

    connect(comboBox_std_window_size, SIGNAL(mouseReleased()), this, SLOT(slotStandardClicked()));
    connect(spinBox_x, SIGNAL(mouseReleased()), this, SLOT(slotCustomClicked()));
    connect(spinBox_y, SIGNAL(mouseReleased()), this, SLOT(slotCustomClicked()));
}

void CSVSettings::SettingsDialog::slotStandardClicked()
{
    std::cout << "click" << std::endl;
    if(!radioButton_standard_res->isChecked())
        radioButton_standard_res->toggle();
}

void CSVSettings::SettingsDialog::slotCustomClicked()
{
    std::cout << "click" << std::endl;
    if(radioButton_standard_res->isChecked())
        radioButton_standard_res->toggle();
}

void CSVSettings::SettingsDialog::slotRendererChanged(const QString &renderer)
{
    comboBox_antialiasing->clear();
    comboBox_antialiasing->addItems(mModel->getOgreOptions(QString("FSAA"), renderer));

    comboBox_shaderlanguage->clear();
    comboBox_shaderlanguage->addItems(mModel->getShaderLanguageByRenderer(renderer));

    if(mModel->settingValue("Video/use settings.cfg") == "true")
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
}

void CSVSettings::SettingsDialog::slotOverrideToggled(bool checked)
{
    if(checked)
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
    {
        label_RenderingSubsystem->setEnabled(true);
        comboBox_rendersystem->setEnabled(true);
        label_Antialiasing->setEnabled(true);
        comboBox_antialiasing->setEnabled(true);
        checkBox_vsync->setEnabled(true);
        label_ShaderLanguage->setEnabled(true);
        comboBox_shaderlanguage->setEnabled(true);
    }
}

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
    int index = -1;

    // initialised in the constructor
    slotOverrideToggled(checkBox_override->isChecked());

    // Ogre initialised earlier
    slotRendererChanged(Ogre::Root::getSingleton().getRenderSystem()->getName().c_str());

    // antialiasing
    QString antialiasing = mModel->settingValue("Video/antialiasing");
    index = comboBox_antialiasing->findData(antialiasing, Qt::DisplayRole);
    if(index != -1)
        comboBox_antialiasing->setCurrentIndex(index);

    // vsync
    checkBox_vsync->setChecked(mModel->settingValue("Video/vsync") == "true");

    // shader lang
    QString shaderlang = mModel->settingValue("Shader/language");
    index = comboBox_shaderlanguage->findData(shaderlang, Qt::DisplayRole);
    if(index != -1)
        comboBox_shaderlanguage->setCurrentIndex(index);

    if(mModel->settingValue("Window Size/Width") != "")
        spinBox_x->setValue(mModel->settingValue("Window Size/Width").toInt());

    if(mModel->settingValue("Window Size/Height") != "")
        spinBox_y->setValue(mModel->settingValue("Window Size/Height").toInt());

    // update display resolution combo box
    comboBox_std_window_size->clear();
    comboBox_std_window_size->addItems(getAvailableResolutions());

    QString currRes = mModel->settingValue("Window Size/Width") + " x " +
                      mModel->settingValue("Window Size/Height");

    index = comboBox_std_window_size->findData(currRes, Qt::DisplayRole, Qt::MatchStartsWith);
    if(index != -1)
    {
        // show the values in ini file
        comboBox_std_window_size->setCurrentIndex(index);
        slotStandardToggled(true);
    }
    else
    {
        // show what's in Ogre instead
        index = comboBox_std_window_size->findData(getCurrentOgreResolution(), Qt::DisplayRole, Qt::MatchStartsWith);
        if(index != -1)
            comboBox_std_window_size->setCurrentIndex(index);

        radioButton_custom_res->setChecked(true);
        slotStandardToggled(false);
    }
}

void CSVSettings::SettingsDialog::saveSettings()
{
#if 0
    //setting the definition in the model automatically syncs with the file
    foreach (const Page *page, mPages)
    {
        foreach (const View *view, page->views())
        {
            if (!view->serializable())
                continue;

            mModel->setDefinitions (view->viewKey(), view->selectedValues());
        }
    }
#endif
    std::cout << "closeEvent" << std::endl;

    // override
    if(checkBox_override->isChecked())
        mModel->setDefinitions("Video/use settings.cfg", QStringList("true"));
    else
        mModel->setDefinitions("Video/use settings.cfg", QStringList("false"));

    // render system
    mModel->setDefinitions("Video/render system",
                           QStringList(comboBox_rendersystem->currentText()));

    // vsync
    if(checkBox_vsync->isChecked())
        mModel->setDefinitions("Video/vsync", QStringList("true"));
    else
        mModel->setDefinitions("Video/vsync", QStringList("false"));

    // antialiasing
    mModel->setDefinitions("Video/antialiasing",
                           QStringList(comboBox_antialiasing->currentText()));
#if 0
    QRegExp reAA("^\\D*(\\d+)\\D*");
    if(reAA.indexIn(comboBox_antialiasing->currentText()) > -1)
        mModel->setDefinitions("Video/antialiasing", QStringList(reAA.cap(1)));
#endif

    // shader lang
    mModel->setDefinitions("Shader/language",
                           QStringList(comboBox_shaderlanguage->currentText()));

    // window size
    if(radioButton_standard_res->isChecked())
    {
        QRegExp re("^(\\d+) x (\\d+)");
        if(re.indexIn(comboBox_std_window_size->currentText()) > -1)
        {
            mModel->setDefinitions("Window Size/Width", QStringList(re.cap(1)));
            mModel->setDefinitions("Window Size/Height", QStringList(re.cap(2)));
        }
    }
    else
    {
        mModel->setDefinitions("Window Size/Width",
                               QStringList(QString::number(spinBox_x->value())));
        mModel->setDefinitions("Window Size/Height",
                               QStringList(QString::number(spinBox_y->value())));
    }

    mModel->saveDefinitions();
}

void CSVSettings::SettingsDialog::createConnections
                                    (const QList <CSMSettings::Setting *> &list)
{
#if 0
    foreach (const CSMSettings::Setting *setting, list)
    {
        View *masterView = findView (setting->page(), setting->name());

        CSMSettings::Connector *connector =
                                new CSMSettings::Connector (masterView, this);

        connect (masterView,
                 SIGNAL (viewUpdated(const QString &, const QStringList &)),
                 connector,
                 SLOT (slotUpdateSlaves())
                 );

        const CSMSettings::ProxyValueMap &proxyMap = setting->proxyLists();

        foreach (const QString &key, proxyMap.keys())
        {
            QStringList keyPair = key.split('/');

            if (keyPair.size() != 2)
                continue;

            View *slaveView = findView (keyPair.at(0), keyPair.at(1));

            if (!slaveView)
            {
                qWarning () << "Unable to create connection for view "
                            << key;
                continue;
            }

            QList <QStringList> proxyList = proxyMap.value (key);
            connector->addSlaveView (slaveView, proxyList);

            connect (slaveView,
                     SIGNAL (viewUpdated(const QString &, const QStringList &)),
                    connector,
                     SLOT (slotUpdateMaster()));
        }
    }
#endif
}

void CSVSettings::SettingsDialog::closeEvent (QCloseEvent *event)
{
    //SettingWindow::closeEvent() must be called first to ensure
    //model is updated
    //SettingWindow::closeEvent (event);
    QApplication::focusWidget()->clearFocus();

    saveSettings();
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
