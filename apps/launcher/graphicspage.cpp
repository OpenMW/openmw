#include "graphicspage.hpp"

#include <QDesktopWidget>
#include <QMessageBox>
#include <QDir>
#include <QScreen>

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
// We need to do this because of Qt: https://bugreports.qt-project.org/browse/QTBUG-22154
#define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif // MAC_OS_X_VERSION_MIN_REQUIRED

#include <SDL_video.h>

#include <numeric>

#include <components/files/configurationmanager.hpp>

QString getAspect(int x, int y)
{
    int gcd = std::gcd (x, y);
    if (gcd == 0)
        return QString();

    int xaspect = x / gcd;
    int yaspect = y / gcd;
    // special case: 8 : 5 is usually referred to as 16:10
    if (xaspect == 8 && yaspect == 5)
        return QString("16:10");

    return QString(QString::number(xaspect) + ":" + QString::number(yaspect));
}

Launcher::GraphicsPage::GraphicsPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName ("GraphicsPage");
    setupUi(this);

    // Set the maximum res we can set in windowed mode
    QRect res = getMaximumResolution();
    customWidthSpinBox->setMaximum(res.width());
    customHeightSpinBox->setMaximum(res.height());

    connect(fullScreenCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotFullScreenChanged(int)));
    connect(standardRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotStandardToggled(bool)));
    connect(screenComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(screenChanged(int)));
    connect(framerateLimitCheckBox, SIGNAL(toggled(bool)), this, SLOT(slotFramerateLimitToggled(bool)));
    connect(shadowDistanceCheckBox, SIGNAL(toggled(bool)), this, SLOT(slotShadowDistLimitToggled(bool)));

}

bool Launcher::GraphicsPage::setupSDL()
{
    bool sdlConnectSuccessful = initSDL();
    if (!sdlConnectSuccessful)
    {
        return false;
    }

    int displays = SDL_GetNumVideoDisplays();

    if (displays < 0)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error receiving number of screens"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>SDL_GetNumVideoDisplays failed:</b><br><br>") + QString::fromUtf8(SDL_GetError()) + "<br>");
        msgBox.exec();
        return false;
    }

    screenComboBox->clear();
    mResolutionsPerScreen.clear();
    for (int i = 0; i < displays; i++)
    {
        mResolutionsPerScreen.append(getAvailableResolutions(i));
        screenComboBox->addItem(QString(tr("Screen ")) + QString::number(i + 1));
    }
    screenChanged(0);

    // Disconnect from SDL processes
    quitSDL();

    return true;
}

bool Launcher::GraphicsPage::loadSettings()
{
    if (!setupSDL())
        return false;

    // Visuals
    if (Settings::Manager::getBool("vsync", "Video"))
        vSyncCheckBox->setCheckState(Qt::Checked);

    if (Settings::Manager::getBool("fullscreen", "Video"))
        fullScreenCheckBox->setCheckState(Qt::Checked);

    if (Settings::Manager::getBool("window border", "Video"))
        windowBorderCheckBox->setCheckState(Qt::Checked);

    // aaValue is the actual value (0, 1, 2, 4, 8, 16)
    int aaValue = Settings::Manager::getInt("antialiasing", "Video");
    // aaIndex is the index into the allowed values in the pull down.
    int aaIndex = antiAliasingComboBox->findText(QString::number(aaValue));
    if (aaIndex != -1)
        antiAliasingComboBox->setCurrentIndex(aaIndex);

    int width = Settings::Manager::getInt("resolution x", "Video");
    int height = Settings::Manager::getInt("resolution y", "Video");
    QString resolution = QString::number(width) + QString(" x ") + QString::number(height);
    screenComboBox->setCurrentIndex(Settings::Manager::getInt("screen", "Video"));

    int resIndex = resolutionComboBox->findText(resolution, Qt::MatchStartsWith);

    if (resIndex != -1) {
        standardRadioButton->toggle();
        resolutionComboBox->setCurrentIndex(resIndex);
    } else {
        customRadioButton->toggle();
        customWidthSpinBox->setValue(width);
        customHeightSpinBox->setValue(height);
    }

    float fpsLimit = Settings::Manager::getFloat("framerate limit", "Video");
    if (fpsLimit != 0)
    {
        framerateLimitCheckBox->setCheckState(Qt::Checked);
        framerateLimitSpinBox->setValue(fpsLimit);
    }

    // Lighting
    int lightingMethod = 1;
    if (Settings::Manager::getString("lighting method", "Shaders") == "legacy")
        lightingMethod = 0;
    else if (Settings::Manager::getString("lighting method", "Shaders") == "shaders")
        lightingMethod = 2;
    lightingMethodComboBox->setCurrentIndex(lightingMethod);

    // Shadows
    if (Settings::Manager::getBool("actor shadows", "Shadows"))
        actorShadowsCheckBox->setCheckState(Qt::Checked);
    if (Settings::Manager::getBool("player shadows", "Shadows"))
        playerShadowsCheckBox->setCheckState(Qt::Checked);
    if (Settings::Manager::getBool("terrain shadows", "Shadows"))
        terrainShadowsCheckBox->setCheckState(Qt::Checked);
    if (Settings::Manager::getBool("object shadows", "Shadows"))
        objectShadowsCheckBox->setCheckState(Qt::Checked);
    if (Settings::Manager::getBool("enable indoor shadows", "Shadows"))
        indoorShadowsCheckBox->setCheckState(Qt::Checked);

    shadowComputeSceneBoundsComboBox->setCurrentIndex(
        shadowComputeSceneBoundsComboBox->findText(
            QString(tr(Settings::Manager::getString("compute scene bounds", "Shadows").c_str()))));

    int shadowDistLimit = Settings::Manager::getInt("maximum shadow map distance", "Shadows");
    if (shadowDistLimit > 0)
    {
        shadowDistanceCheckBox->setCheckState(Qt::Checked);
        shadowDistanceSpinBox->setValue(shadowDistLimit);
    }

    float shadowFadeStart = Settings::Manager::getFloat("shadow fade start", "Shadows");
    if (shadowFadeStart != 0)
        fadeStartSpinBox->setValue(shadowFadeStart);

    int shadowRes = Settings::Manager::getInt("shadow map resolution", "Shadows");
    int shadowResIndex = shadowResolutionComboBox->findText(QString::number(shadowRes));
    if (shadowResIndex != -1)
        shadowResolutionComboBox->setCurrentIndex(shadowResIndex);

    return true;
}

void Launcher::GraphicsPage::saveSettings()
{
    // Visuals

    // Ensure we only set the new settings if they changed. This is to avoid cluttering the
    // user settings file (which by definition should only contain settings the user has touched)
    bool cVSync = vSyncCheckBox->checkState();
    if (cVSync != Settings::Manager::getBool("vsync", "Video"))
        Settings::Manager::setBool("vsync", "Video", cVSync);

    bool cFullScreen = fullScreenCheckBox->checkState();
    if (cFullScreen != Settings::Manager::getBool("fullscreen", "Video"))
        Settings::Manager::setBool("fullscreen", "Video", cFullScreen);

    bool cWindowBorder = windowBorderCheckBox->checkState();
    if (cWindowBorder != Settings::Manager::getBool("window border", "Video"))
        Settings::Manager::setBool("window border", "Video", cWindowBorder);

    int cAAValue = antiAliasingComboBox->currentText().toInt();
    if (cAAValue != Settings::Manager::getInt("antialiasing", "Video"))
        Settings::Manager::setInt("antialiasing", "Video", cAAValue);

    int cWidth = 0;
    int cHeight = 0;
    if (standardRadioButton->isChecked()) {
        QRegExp resolutionRe(QString("(\\d+) x (\\d+).*"));
        if (resolutionRe.exactMatch(resolutionComboBox->currentText().simplified())) {
            cWidth = resolutionRe.cap(1).toInt();
            cHeight = resolutionRe.cap(2).toInt();
        }
    } else {
        cWidth = customWidthSpinBox->value();
        cHeight = customHeightSpinBox->value();
    }

    if (cWidth != Settings::Manager::getInt("resolution x", "Video"))
        Settings::Manager::setInt("resolution x", "Video", cWidth);

    if (cHeight != Settings::Manager::getInt("resolution y", "Video"))
        Settings::Manager::setInt("resolution y", "Video", cHeight);

    int cScreen = screenComboBox->currentIndex();
    if (cScreen != Settings::Manager::getInt("screen", "Video"))
        Settings::Manager::setInt("screen", "Video", cScreen);

    if (framerateLimitCheckBox->checkState() != Qt::Unchecked)
    {
        float cFpsLimit = framerateLimitSpinBox->value();
        if (cFpsLimit != Settings::Manager::getFloat("framerate limit", "Video"))
            Settings::Manager::setFloat("framerate limit", "Video", cFpsLimit);
    }
    else if (Settings::Manager::getFloat("framerate limit", "Video") != 0)
    {
        Settings::Manager::setFloat("framerate limit", "Video", 0);
    }

    // Lighting
    static std::array<std::string, 3> lightingMethodMap = {"legacy", "shaders compatibility", "shaders"};
    Settings::Manager::setString("lighting method", "Shaders", lightingMethodMap[lightingMethodComboBox->currentIndex()]);

    // Shadows
    int cShadowDist = shadowDistanceCheckBox->checkState() != Qt::Unchecked ? shadowDistanceSpinBox->value() : 0;
    if (Settings::Manager::getInt("maximum shadow map distance", "Shadows") != cShadowDist)
        Settings::Manager::setInt("maximum shadow map distance", "Shadows", cShadowDist);
    float cFadeStart = fadeStartSpinBox->value();
    if (cShadowDist > 0 && Settings::Manager::getFloat("shadow fade start", "Shadows") != cFadeStart)
        Settings::Manager::setFloat("shadow fade start", "Shadows", cFadeStart);

    bool cActorShadows = actorShadowsCheckBox->checkState();
    bool cObjectShadows = objectShadowsCheckBox->checkState();
    bool cTerrainShadows = terrainShadowsCheckBox->checkState();
    bool cPlayerShadows = playerShadowsCheckBox->checkState();
    if (cActorShadows || cObjectShadows || cTerrainShadows || cPlayerShadows)
    {
        if (!Settings::Manager::getBool("enable shadows", "Shadows"))
            Settings::Manager::setBool("enable shadows", "Shadows", true);
        if (Settings::Manager::getBool("actor shadows", "Shadows") != cActorShadows)
            Settings::Manager::setBool("actor shadows", "Shadows", cActorShadows);
        if (Settings::Manager::getBool("player shadows", "Shadows") != cPlayerShadows)
            Settings::Manager::setBool("player shadows", "Shadows", cPlayerShadows);
        if (Settings::Manager::getBool("object shadows", "Shadows") != cObjectShadows)
            Settings::Manager::setBool("object shadows", "Shadows", cObjectShadows);
        if (Settings::Manager::getBool("terrain shadows", "Shadows") != cTerrainShadows)
            Settings::Manager::setBool("terrain shadows", "Shadows", cTerrainShadows);
    }
    else
    {
        if (Settings::Manager::getBool("enable shadows", "Shadows"))
            Settings::Manager::setBool("enable shadows", "Shadows", false);
        if (Settings::Manager::getBool("actor shadows", "Shadows"))
            Settings::Manager::setBool("actor shadows", "Shadows", false);
        if (Settings::Manager::getBool("player shadows", "Shadows"))
            Settings::Manager::setBool("player shadows", "Shadows", false);
        if (Settings::Manager::getBool("object shadows", "Shadows"))
            Settings::Manager::setBool("object shadows", "Shadows", false);
        if (Settings::Manager::getBool("terrain shadows", "Shadows"))
            Settings::Manager::setBool("terrain shadows", "Shadows", false);
    }

    bool cIndoorShadows = indoorShadowsCheckBox->checkState();
    if (Settings::Manager::getBool("enable indoor shadows", "Shadows") != cIndoorShadows)
        Settings::Manager::setBool("enable indoor shadows", "Shadows", cIndoorShadows);

    int cShadowRes = shadowResolutionComboBox->currentText().toInt();
    if (cShadowRes != Settings::Manager::getInt("shadow map resolution", "Shadows"))
        Settings::Manager::setInt("shadow map resolution", "Shadows", cShadowRes);

    auto cComputeSceneBounds = shadowComputeSceneBoundsComboBox->currentText().toStdString();
    if (cComputeSceneBounds != Settings::Manager::getString("compute scene bounds", "Shadows"))
        Settings::Manager::setString("compute scene bounds", "Shadows", cComputeSceneBounds);
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

        QString resolution = QString::number(mode.w) + QString(" x ") + QString::number(mode.h);

        QString aspect = getAspect(mode.w, mode.h);
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

    for (QScreen* screen : QGuiApplication::screens())
    {
        QRect res = screen->geometry();
        if (res.width() > max.width())
            max.setWidth(res.width());
        if (res.height() > max.height())
            max.setHeight(res.height());
    }
    return max;
}

void Launcher::GraphicsPage::screenChanged(int screen)
{
    if (screen >= 0) {
        resolutionComboBox->clear();
        resolutionComboBox->addItems(mResolutionsPerScreen[screen]);
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

void Launcher::GraphicsPage::slotFramerateLimitToggled(bool checked)
{
    framerateLimitSpinBox->setEnabled(checked);
}

void Launcher::GraphicsPage::slotShadowDistLimitToggled(bool checked)
{
    shadowDistanceSpinBox->setEnabled(checked);
    fadeStartSpinBox->setEnabled(checked);
}
