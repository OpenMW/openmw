#include "graphicspage.hpp"

#include <QDesktopWidget>
#include <QMessageBox>
#include <QDir>

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
// We need to do this because of Qt: https://bugreports.qt-project.org/browse/QTBUG-22154
#define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif // MAC_OS_X_VERSION_MIN_REQUIRED

#include <SDL_video.h>

#include <OgreRoot.h>
#include <OgreRenderSystem.h>

#include <boost/math/common_factor.hpp>

#include <components/files/configurationmanager.hpp>

#include <components/contentselector/model/naturalsort.hpp>

#include "settings/graphicssettings.hpp"

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

Launcher::GraphicsPage::GraphicsPage(Files::ConfigurationManager &cfg, GraphicsSettings &graphicsSetting, QWidget *parent)
    : mOgre(NULL)
    , mSelectedRenderSystem(NULL)
    , mOpenGLRenderSystem(NULL)
    , mDirect3DRenderSystem(NULL)
    , mCfgMgr(cfg)
    , mGraphicsSettings(graphicsSetting)
    , QWidget(parent)
{
    setObjectName ("GraphicsPage");
    setupUi(this);

    // Set the maximum res we can set in windowed mode
    QRect res = getMaximumResolution();
    customWidthSpinBox->setMaximum(res.width());
    customHeightSpinBox->setMaximum(res.height());

    connect(rendererComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(rendererChanged(const QString&)));
    connect(fullScreenCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotFullScreenChanged(int)));
    connect(standardRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotStandardToggled(bool)));
    connect(screenComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(screenChanged(int)));

}

bool Launcher::GraphicsPage::setupOgre()
{
    try
    {
        mOgre = mOgreInit.init(mCfgMgr.getLogPath().string() + "/launcherOgre.log");
    }
    catch(Ogre::Exception &ex)
    {
        QString ogreError = QString::fromUtf8(ex.getFullDescription().c_str());
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error creating Ogre::Root");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Failed to create the Ogre::Root object</b><br><br> \
        Press \"Show Details...\" for more information.<br>"));
        msgBox.setDetailedText(ogreError);
        msgBox.exec();

        qCritical("Error creating Ogre::Root, the error reported was:\n %s", qPrintable(ogreError));
        return false;
    }

    // Get the available renderers and put them in the combobox
    const Ogre::RenderSystemList &renderers = mOgre->getAvailableRenderers();

    for (Ogre::RenderSystemList::const_iterator r = renderers.begin(); r != renderers.end(); ++r) {
        mSelectedRenderSystem = *r;
        rendererComboBox->addItem((*r)->getName().c_str());
    }

    QString openGLName = QString("OpenGL Rendering Subsystem");
    QString direct3DName = QString("Direct3D9 Rendering Subsystem");

    // Create separate rendersystems
    mOpenGLRenderSystem = mOgre->getRenderSystemByName(openGLName.toStdString());
    mDirect3DRenderSystem = mOgre->getRenderSystemByName(direct3DName.toStdString());

    if (!mOpenGLRenderSystem && !mDirect3DRenderSystem) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error creating renderer"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not select a valid render system</b><br><br> \
                          Please make sure Ogre plugins were installed correctly.<br>"));
        msgBox.exec();
        return false;
    }

    // Now fill the GUI elements
    int index = rendererComboBox->findText(mGraphicsSettings.value(QString("Video/render system")));
    if ( index != -1) {
        rendererComboBox->setCurrentIndex(index);
    } else {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        rendererComboBox->setCurrentIndex(rendererComboBox->findText(direct3DName));
#else
        rendererComboBox->setCurrentIndex(rendererComboBox->findText(openGLName));
#endif
    }

    antiAliasingComboBox->clear();
    antiAliasingComboBox->addItems(getAvailableOptions(QString("FSAA"), mSelectedRenderSystem));

    return true;
}

bool Launcher::GraphicsPage::setupSDL()
{
    int displays = SDL_GetNumVideoDisplays();

    if (displays < 0)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error receiving number of screens"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>SDL_GetNumDisplayModes failed:</b><br><br>") + QString::fromUtf8(SDL_GetError()) + "<br>");
        msgBox.exec();
        return false;
    }

    screenComboBox->clear();
    for (int i = 0; i < displays; i++)
    {
        screenComboBox->addItem(QString(tr("Screen ")) + QString::number(i + 1));
    }

    return true;
}

bool Launcher::GraphicsPage::loadSettings()
{
    if (!setupSDL())
        return false;
    if (!mOgre && !setupOgre())
        return false;

    if (mGraphicsSettings.value(QString("Video/vsync")) == QLatin1String("true"))
        vSyncCheckBox->setCheckState(Qt::Checked);

    if (mGraphicsSettings.value(QString("Video/fullscreen")) == QLatin1String("true"))
        fullScreenCheckBox->setCheckState(Qt::Checked);

    if (mGraphicsSettings.value(QString("Video/window border")) == QLatin1String("true"))
        windowBorderCheckBox->setCheckState(Qt::Checked);

    int aaIndex = antiAliasingComboBox->findText(mGraphicsSettings.value(QString("Video/antialiasing")));
    if (aaIndex != -1)
        antiAliasingComboBox->setCurrentIndex(aaIndex);

    QString width = mGraphicsSettings.value(QString("Video/resolution x"));
    QString height = mGraphicsSettings.value(QString("Video/resolution y"));
    QString resolution = width + QString(" x ") + height;
    QString screen = mGraphicsSettings.value(QString("Video/screen"));

    screenComboBox->setCurrentIndex(screen.toInt());

    int resIndex = resolutionComboBox->findText(resolution, Qt::MatchStartsWith);

    if (resIndex != -1) {
        standardRadioButton->toggle();
        resolutionComboBox->setCurrentIndex(resIndex);
    } else {
        customRadioButton->toggle();
        customWidthSpinBox->setValue(width.toInt());
        customHeightSpinBox->setValue(height.toInt());

    }

    return true;
}

void Launcher::GraphicsPage::saveSettings()
{
    vSyncCheckBox->checkState() ? mGraphicsSettings.setValue(QString("Video/vsync"), QString("true"))
                                 : mGraphicsSettings.setValue(QString("Video/vsync"), QString("false"));

    fullScreenCheckBox->checkState() ? mGraphicsSettings.setValue(QString("Video/fullscreen"), QString("true"))
                                      : mGraphicsSettings.setValue(QString("Video/fullscreen"), QString("false"));

    windowBorderCheckBox->checkState() ? mGraphicsSettings.setValue(QString("Video/window border"), QString("true"))
                                      : mGraphicsSettings.setValue(QString("Video/window border"), QString("false"));

    mGraphicsSettings.setValue(QString("Video/antialiasing"), antiAliasingComboBox->currentText());
    mGraphicsSettings.setValue(QString("Video/render system"), rendererComboBox->currentText());


    if (standardRadioButton->isChecked()) {
        QRegExp resolutionRe(QString("(\\d+) x (\\d+).*"));

        if (resolutionRe.exactMatch(resolutionComboBox->currentText().simplified())) {
            mGraphicsSettings.setValue(QString("Video/resolution x"), resolutionRe.cap(1));
            mGraphicsSettings.setValue(QString("Video/resolution y"), resolutionRe.cap(2));
        }
    } else {
        mGraphicsSettings.setValue(QString("Video/resolution x"), QString::number(customWidthSpinBox->value()));
        mGraphicsSettings.setValue(QString("Video/resolution y"), QString::number(customHeightSpinBox->value()));
    }

    mGraphicsSettings.setValue(QString("Video/screen"), QString::number(screenComboBox->currentIndex()));
}

QStringList Launcher::GraphicsPage::getAvailableOptions(const QString &key, Ogre::RenderSystem *renderer)
{
    QStringList result;

    uint row = 0;
    Ogre::ConfigOptionMap options = renderer->getConfigOptions();

    for (Ogre::ConfigOptionMap::iterator i = options.begin (); i != options.end (); ++i, ++row)
    {
        Ogre::StringVector::iterator opt_it;
        uint idx = 0;

        for (opt_it = i->second.possibleValues.begin();
             opt_it != i->second.possibleValues.end(); ++opt_it, ++idx)
        {
            if (strcmp (key.toStdString().c_str(), i->first.c_str()) == 0) {
                result << ((key == "FSAA") ? QString("MSAA ") : QString("")) + QString::fromUtf8((*opt_it).c_str()).simplified();
            }
        }
    }

    // Sort ascending
    qSort(result.begin(), result.end(), naturalSortLessThanCI);

    // Replace the zero option with Off
    int index = result.indexOf("MSAA 0");

    if (index != -1)
        result.replace(index, tr("Off"));

    return result;
}

QStringList Launcher::GraphicsPage::getAvailableResolutions(int screen)
{
    QStringList result;
    SDL_DisplayMode mode;
    int modeIndex, modes = SDL_GetNumDisplayModes(screen);

    if (modes < 0)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error receiving resolutions"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>SDL_GetNumDisplayModes failed:</b><br><br>") + QString::fromUtf8(SDL_GetError()) + "<br>");
        msgBox.exec();
        return result;
    }

    for (modeIndex = 0; modeIndex < modes; modeIndex++)
    {
        if (SDL_GetDisplayMode(screen, modeIndex, &mode) < 0)
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error receiving resolutions"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<br><b>SDL_GetDisplayMode failed:</b><br><br>") + QString::fromUtf8(SDL_GetError()) + "<br>");
            msgBox.exec();
            return result;
        }

        QString aspect = getAspect(mode.w, mode.h);
        QString resolution = QString::number(mode.w) + QString(" x ") + QString::number(mode.h);

        if (aspect == QLatin1String("16:9") || aspect == QLatin1String("16:10")) {
            resolution.append(tr("\t(Wide ") + aspect + ")");

        } else if (aspect == QLatin1String("4:3")) {
            resolution.append(tr("\t(Standard 4:3)"));
        }

        result.append(resolution);
    }

    result.removeDuplicates();
    return result;
}

QRect Launcher::GraphicsPage::getMaximumResolution()
{
    QRect max;
    int screens = QApplication::desktop()->screenCount();
    for (int i = 0; i < screens; ++i)
    {
        QRect res = QApplication::desktop()->screenGeometry(i);
        if (res.width() > max.width())
            max.setWidth(res.width());
        if (res.height() > max.height())
            max.setHeight(res.height());
    }
    return max;
}

void Launcher::GraphicsPage::rendererChanged(const QString &renderer)
{
    mSelectedRenderSystem = mOgre->getRenderSystemByName(renderer.toStdString());

    antiAliasingComboBox->clear();

    antiAliasingComboBox->addItems(getAvailableOptions(QString("FSAA"), mSelectedRenderSystem));
}

void Launcher::GraphicsPage::screenChanged(int screen)
{
    if (screen >= 0) {
        resolutionComboBox->clear();
        resolutionComboBox->addItems(getAvailableResolutions(screen));
    }
}

void Launcher::GraphicsPage::slotFullScreenChanged(int state)
{
    if (state == Qt::Checked) {
        standardRadioButton->toggle();
        customRadioButton->setEnabled(false);
        customWidthSpinBox->setEnabled(false);
        customHeightSpinBox->setEnabled(false);
        windowBorderCheckBox->setEnabled(false);
    } else {
        customRadioButton->setEnabled(true);
        customWidthSpinBox->setEnabled(true);
        customHeightSpinBox->setEnabled(true);
        windowBorderCheckBox->setEnabled(true);
    }
}

void Launcher::GraphicsPage::slotStandardToggled(bool checked)
{
    if (checked) {
        resolutionComboBox->setEnabled(true);
        customWidthSpinBox->setEnabled(false);
        customHeightSpinBox->setEnabled(false);
    } else {
        resolutionComboBox->setEnabled(false);
        customWidthSpinBox->setEnabled(true);
        customHeightSpinBox->setEnabled(true);
    }
}
