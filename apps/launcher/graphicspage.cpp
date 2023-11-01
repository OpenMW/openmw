#include "graphicspage.hpp"

#include "sdlinit.hpp"

#include <components/settings/values.hpp>

#include <QMessageBox>
#include <QScreen>

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
// We need to do this because of Qt: https://bugreports.qt-project.org/browse/QTBUG-22154
#define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif // MAC_OS_X_VERSION_MIN_REQUIRED

#include <SDL_video.h>

#include <array>
#include <numeric>

QString getAspect(int x, int y)
{
    int gcd = std::gcd(x, y);
    if (gcd == 0)
        return QString();

    int xaspect = x / gcd;
    int yaspect = y / gcd;
    // special case: 8 : 5 is usually referred to as 16:10
    if (xaspect == 8 && yaspect == 5)
        return QString("16:10");

    return QString(QString::number(xaspect) + ":" + QString::number(yaspect));
}

Launcher::GraphicsPage::GraphicsPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("GraphicsPage");
    setupUi(this);

    // Set the maximum res we can set in windowed mode
    QRect res = getMaximumResolution();
    customWidthSpinBox->setMaximum(res.width());
    customHeightSpinBox->setMaximum(res.height());

    connect(windowModeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
        &GraphicsPage::slotFullScreenChanged);
    connect(standardRadioButton, &QRadioButton::toggled, this, &GraphicsPage::slotStandardToggled);
    connect(screenComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &GraphicsPage::screenChanged);
    connect(framerateLimitCheckBox, &QCheckBox::toggled, this, &GraphicsPage::slotFramerateLimitToggled);
    connect(shadowDistanceCheckBox, &QCheckBox::toggled, this, &GraphicsPage::slotShadowDistLimitToggled);
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
        msgBox.setText(
            tr("<br><b>SDL_GetNumVideoDisplays failed:</b><br><br>") + QString::fromUtf8(SDL_GetError()) + "<br>");
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

    const int vsync = Settings::video().mVsyncMode;

    vSyncComboBox->setCurrentIndex(vsync);

    const Settings::WindowMode windowMode = Settings::video().mWindowMode;

    windowModeComboBox->setCurrentIndex(static_cast<int>(windowMode));
    handleWindowModeChange(windowMode);

    if (Settings::video().mWindowBorder)
        windowBorderCheckBox->setCheckState(Qt::Checked);

    // aaValue is the actual value (0, 1, 2, 4, 8, 16)
    const int aaValue = Settings::video().mAntialiasing;
    // aaIndex is the index into the allowed values in the pull down.
    const int aaIndex = antiAliasingComboBox->findText(QString::number(aaValue));
    if (aaIndex != -1)
        antiAliasingComboBox->setCurrentIndex(aaIndex);

    const int width = Settings::video().mResolutionX;
    const int height = Settings::video().mResolutionY;
    QString resolution = QString::number(width) + QString(" x ") + QString::number(height);
    screenComboBox->setCurrentIndex(Settings::video().mScreen);

    int resIndex = resolutionComboBox->findText(resolution, Qt::MatchStartsWith);

    if (resIndex != -1)
    {
        standardRadioButton->toggle();
        resolutionComboBox->setCurrentIndex(resIndex);
    }
    else
    {
        customRadioButton->toggle();
        customWidthSpinBox->setValue(width);
        customHeightSpinBox->setValue(height);
    }

    const float fpsLimit = Settings::video().mFramerateLimit;
    if (fpsLimit != 0)
    {
        framerateLimitCheckBox->setCheckState(Qt::Checked);
        framerateLimitSpinBox->setValue(fpsLimit);
    }

    // Lighting
    int lightingMethod = 1;
    switch (Settings::shaders().mLightingMethod)
    {
        case SceneUtil::LightingMethod::FFP:
            lightingMethod = 0;
            break;
        case SceneUtil::LightingMethod::PerObjectUniform:
            lightingMethod = 1;
            break;
        case SceneUtil::LightingMethod::SingleUBO:
            lightingMethod = 2;
            break;
    }
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

    shadowComputeSceneBoundsComboBox->setCurrentIndex(shadowComputeSceneBoundsComboBox->findText(
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

    Settings::video().mVsyncMode.set(static_cast<SDLUtil::VSyncMode>(vSyncComboBox->currentIndex()));
    Settings::video().mWindowMode.set(static_cast<Settings::WindowMode>(windowModeComboBox->currentIndex()));
    Settings::video().mWindowBorder.set(windowBorderCheckBox->checkState() == Qt::Checked);
    Settings::video().mAntialiasing.set(antiAliasingComboBox->currentText().toInt());

    int cWidth = 0;
    int cHeight = 0;
    if (standardRadioButton->isChecked())
    {
        QRegularExpression resolutionRe("^(\\d+) x (\\d+)");
        QRegularExpressionMatch match = resolutionRe.match(resolutionComboBox->currentText().simplified());
        if (match.hasMatch())
        {
            cWidth = match.captured(1).toInt();
            cHeight = match.captured(2).toInt();
        }
    }
    else
    {
        cWidth = customWidthSpinBox->value();
        cHeight = customHeightSpinBox->value();
    }

    Settings::video().mResolutionX.set(cWidth);
    Settings::video().mResolutionY.set(cHeight);
    Settings::video().mScreen.set(screenComboBox->currentIndex());

    if (framerateLimitCheckBox->checkState() != Qt::Unchecked)
    {
        Settings::video().mFramerateLimit.set(framerateLimitSpinBox->value());
    }
    else if (Settings::video().mFramerateLimit != 0)
    {
        Settings::video().mFramerateLimit.set(0);
    }

    // Lighting
    static constexpr std::array<SceneUtil::LightingMethod, 3> lightingMethodMap = {
        SceneUtil::LightingMethod::FFP,
        SceneUtil::LightingMethod::PerObjectUniform,
        SceneUtil::LightingMethod::SingleUBO,
    };
    Settings::shaders().mLightingMethod.set(lightingMethodMap[lightingMethodComboBox->currentIndex()]);

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
        msgBox.setText(
            tr("<br><b>SDL_GetNumDisplayModes failed:</b><br><br>") + QString::fromUtf8(SDL_GetError()) + "<br>");
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
            msgBox.setText(
                tr("<br><b>SDL_GetDisplayMode failed:</b><br><br>") + QString::fromUtf8(SDL_GetError()) + "<br>");
            msgBox.exec();
            return result;
        }

        QString resolution = QString::number(mode.w) + QString(" x ") + QString::number(mode.h);

        QString aspect = getAspect(mode.w, mode.h);
        if (aspect == QLatin1String("16:9") || aspect == QLatin1String("16:10"))
        {
            resolution.append(tr("\t(Wide ") + aspect + ")");
        }
        else if (aspect == QLatin1String("4:3"))
        {
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
    if (screen >= 0)
    {
        resolutionComboBox->clear();
        resolutionComboBox->addItems(mResolutionsPerScreen[screen]);
    }
}

void Launcher::GraphicsPage::slotFullScreenChanged(int mode)
{
    handleWindowModeChange(static_cast<Settings::WindowMode>(mode));
}

void Launcher::GraphicsPage::handleWindowModeChange(Settings::WindowMode mode)
{
    if (mode == Settings::WindowMode::Fullscreen || mode == Settings::WindowMode::WindowedFullscreen)
    {
        standardRadioButton->toggle();
        customRadioButton->setEnabled(false);
        customWidthSpinBox->setEnabled(false);
        customHeightSpinBox->setEnabled(false);
        windowBorderCheckBox->setEnabled(false);
    }
    else
    {
        customRadioButton->setEnabled(true);
        customWidthSpinBox->setEnabled(true);
        customHeightSpinBox->setEnabled(true);
        windowBorderCheckBox->setEnabled(true);
    }
}

void Launcher::GraphicsPage::slotStandardToggled(bool checked)
{
    if (checked)
    {
        resolutionComboBox->setEnabled(true);
        customWidthSpinBox->setEnabled(false);
        customHeightSpinBox->setEnabled(false);
    }
    else
    {
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
