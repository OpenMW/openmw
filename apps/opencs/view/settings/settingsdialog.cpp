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
            Ogre::StringVector::iterator iter = it->second.possibleValues.begin();
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
            Ogre::StringVector::iterator iter = it->second.possibleValues.begin();
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
    sbWidth->setMaximum(res.width());
    sbHeight->setMaximum(res.height());

    // to enable mouse events on disabled widgets
    cmbStdWinSize->installEventFilter(this);
    sbWidth->installEventFilter(this);
    sbHeight->installEventFilter(this);

    connect(cbOverride, SIGNAL(toggled(bool)), this, SLOT(slotOverrideToggled(bool)));
    connect(cmbRenderSys, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slotRendererChanged(const QString&)));
    //connect(rbStdWinSize, SIGNAL(toggled(bool)), this, SLOT(slotStandardToggled(bool)));

    // to update the checkbox on the view menu
    connect(cbStatusBar, SIGNAL(toggled(bool)), this, SIGNAL (toggleStatusBar(bool)));
}

bool CSVSettings::SettingsDialog::eventFilter(QObject *target, QEvent *event)
{
    QWidget *item = qobject_cast<QWidget *>(target);
    if (target == cmbStdWinSize)
    {
        if (!item->isEnabled() && (event->type() == QEvent::MouseButtonRelease))
        {
            //rbCustWinSize->setChecked(false);
            //rbStdWinSize->setChecked(true);
            //slotStandardToggled(true);

            return false;
        }
    }
    else if (target == sbWidth || target == sbHeight)
    {
        if (!item->isEnabled() && (event->type() == QEvent::MouseButtonPress))
        {
            //rbStdWinSize->setChecked(false);
            //rbCustWinSize->setChecked(true);
            //slotStandardToggled(false);

            return false;
        }
    }
    return QTabWidget::eventFilter(target, event);
}

void CSVSettings::SettingsDialog::slotRendererChanged(const QString &renderer)
{
    cmbAntiAlias->clear();
    cmbAntiAlias->addItems(mModel->getOgreOptions(QString("FSAA"), renderer));

    cmbShaderLang->clear();
    cmbShaderLang->addItems(mModel->getShaderLanguageByRenderer(renderer));

    if(mModel->settingValue("Video/use settings.cfg") == "true")
    {
        labRenderSys->setEnabled(false);
        cmbRenderSys->setEnabled(false);
        labAntiAlias->setEnabled(false);
        cmbAntiAlias->setEnabled(false);
        //cbVsync->setEnabled(false);
        labShaderLang->setEnabled(false);
        cmbShaderLang->setEnabled(false);
    }
    else
        cbOverride->setChecked(false);
}

void CSVSettings::SettingsDialog::slotOverrideToggled(bool checked)
{
    if(checked)
    {
        labRenderSys->setEnabled(false);
        cmbRenderSys->setEnabled(false);
        labAntiAlias->setEnabled(false);
        cmbAntiAlias->setEnabled(false);
        //cbVsync->setEnabled(false);
        labShaderLang->setEnabled(false);
        cmbShaderLang->setEnabled(false);
    }
    else
    {
        labRenderSys->setEnabled(true);
        cmbRenderSys->setEnabled(true);
        labAntiAlias->setEnabled(true);
        cmbAntiAlias->setEnabled(true);
        //cbVsync->setEnabled(true);
        labShaderLang->setEnabled(true);
        cmbShaderLang->setEnabled(true);
    }
}

void CSVSettings::SettingsDialog::slotStandardToggled(bool checked)
{
    if (checked)
    {
        cmbStdWinSize->setEnabled(true);
        sbWidth->setEnabled(false);
        sbHeight->setEnabled(false);
    }
    else
    {
        cmbStdWinSize->setEnabled(false);
        sbWidth->setEnabled(true);
        sbHeight->setEnabled(true);
    }
}

void CSVSettings::SettingsDialog::setViewValues()
{
    int index = -1;

    // initialised in the constructor
    slotOverrideToggled(cbOverride->isChecked());

    // Ogre renderer
    cmbRenderSys->clear();
    cmbRenderSys->addItems(mModel->getOgreRenderers());
    //slotRendererChanged(Ogre::Root::getSingleton().getRenderSystem()->getName().c_str());

    // antialiasing
    QString antialiasing = mModel->settingValue("Video/antialiasing");
    index = cmbAntiAlias->findData(antialiasing, Qt::DisplayRole);
    if(index != -1)
        cmbAntiAlias->setCurrentIndex(index);

    // vsync
    //cbVsync->setChecked(mModel->settingValue("Video/vsync") == "true");
    cbVsync->setChecked(false); // disable vsync option for now
    cbVsync->setEnabled(false); // disable vsync option for now

    // shader lang
    QString shaderlang = mModel->settingValue("General/shader mode");
    index = cmbShaderLang->findData(shaderlang, Qt::DisplayRole);
    if(index != -1)
        cmbShaderLang->setCurrentIndex(index);

    if(mModel->settingValue("Window Size/Width") != "")
        sbWidth->setValue(mModel->settingValue("Window Size/Width").toInt());

    if(mModel->settingValue("Window Size/Height") != "")
        sbHeight->setValue(mModel->settingValue("Window Size/Height").toInt());

    // update display resolution combo box
    cmbStdWinSize->clear();
    cmbStdWinSize->addItems(getAvailableResolutions());

    QString currRes = mModel->settingValue("Window Size/Width") + " x " +
                      mModel->settingValue("Window Size/Height");

    index = cmbStdWinSize->findData(currRes, Qt::DisplayRole, Qt::MatchStartsWith);
    if(index != -1)
    {
        // show the values in ini file
        cmbStdWinSize->setCurrentIndex(index);
        slotStandardToggled(true);
    }
    else
    {
        // show what's in Ogre instead
        index = cmbStdWinSize->findData(getCurrentOgreResolution(),
                                        Qt::DisplayRole, Qt::MatchStartsWith);
        if(index != -1)
            cmbStdWinSize->setCurrentIndex(index);

        //rbCustWinSize->setChecked(true);
        slotStandardToggled(false);
    }

    // status bar
    cbStatusBar->setChecked(mModel->settingValue("Display/show statusbar") == "true");

    // display format
    QString recStat = mModel->settingValue("Display Format/Record Status Display");
    index = cmbRecStatus->findData(recStat, Qt::DisplayRole);
    if(index != -1)
        cmbRecStatus->setCurrentIndex(index);

    QString refIdType = mModel->settingValue("Display Format/Referenceable ID Type Display");
    index = cmbRefIdType->findData(refIdType, Qt::DisplayRole);
    if(index != -1)
        cmbRefIdType->setCurrentIndex(index);
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
    if(cbOverride->isChecked())
        mModel->setDefinitions("Video/use settings.cfg", QStringList("true"));
    else
        mModel->setDefinitions("Video/use settings.cfg", QStringList("false"));

    // render system
    mModel->setDefinitions("Video/render system",
                           QStringList(cmbRenderSys->currentText()));

    // vsync
    if(cbVsync->isChecked())
        mModel->setDefinitions("Video/vsync", QStringList("true"));
    else
        mModel->setDefinitions("Video/vsync", QStringList("false"));

    // antialiasing
    mModel->setDefinitions("Video/antialiasing",
                           QStringList(cmbAntiAlias->currentText()));
#if 0
    QRegExp reAA("^\\D*(\\d+)\\D*");
    if(reAA.indexIn(cmbAntiAlias->currentText()) > -1)
        mModel->setDefinitions("Video/antialiasing", QStringList(reAA.cap(1)));
#endif

    // shader lang
    mModel->setDefinitions("General/shader mode",
                           QStringList(cmbShaderLang->currentText().toLower()));

    // window size
    if(0) //rbStdWinSize->isChecked())
    {
        QRegExp re("^(\\d+) x (\\d+)");
        if(re.indexIn(cmbStdWinSize->currentText()) > -1)
        {
            mModel->setDefinitions("Window Size/Width", QStringList(re.cap(1)));
            mModel->setDefinitions("Window Size/Height", QStringList(re.cap(2)));
        }
    }
    else
    {
        mModel->setDefinitions("Window Size/Width",
                               QStringList(QString::number(sbWidth->value())));
        mModel->setDefinitions("Window Size/Height",
                               QStringList(QString::number(sbHeight->value())));
    }

    // status bar
    if(cbStatusBar->isChecked())
        mModel->setDefinitions("Display/show statusbar", QStringList("true"));
    else
        mModel->setDefinitions("Display/show statusbar", QStringList("false"));

    // display format
    mModel->setDefinitions("Display Format/Record Status Display",
                           QStringList(cmbRecStatus->currentText()));
    mModel->setDefinitions("Display Format/Referenceable ID Type Display",
                           QStringList(cmbRefIdType->currentText()));

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
