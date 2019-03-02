#include "graphicspage.hpp"

#include <csignal>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QDir>

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
// We need to do this because of Qt: https://bugreports.qt-project.org/browse/QTBUG-22154
#define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif // MAC_OS_X_VERSION_MIN_REQUIRED

#include <SDL_video.h>

#include <components/files/configurationmanager.hpp>
#include <components/misc/gcd.hpp>

QString getAspect(int x, int y)
{
    int gcd = Misc::gcd (x, y);
    int xaspect = x / gcd;
    int yaspect = y / gcd;
    // special case: 8 : 5 is usually referred to as 16:10
    if (xaspect == 8 && yaspect == 5)
        return QString("16:10");

    return QString(QString::number(xaspect) + ":" + QString::number(yaspect));
}

Launcher::GraphicsPage::GraphicsPage(Files::ConfigurationManager &cfg, Settings::Manager &engineSettings, QWidget *parent)
    : QWidget(parent)
    , mCfgMgr(cfg)
    , mEngineSettings(engineSettings)
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

}

bool Launcher::GraphicsPage::setupSDL()
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    bool sdlConnectSuccessful = initSDL();
    if (!sdlConnectSuccessful)
    {
        return false;
    }
#endif

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

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    // Disconnect from SDL processes
    quitSDL();
#endif

    return true;
}

bool Launcher::GraphicsPage::loadSettings()
{
    if (!setupSDL())
        return false;

    if (mEngineSettings.getBool("vsync", "Video"))
        vSyncCheckBox->setCheckState(Qt::Checked);

    if (mEngineSettings.getBool("fullscreen", "Video"))
        fullScreenCheckBox->setCheckState(Qt::Checked);

    if (mEngineSettings.getBool("window border", "Video"))
        windowBorderCheckBox->setCheckState(Qt::Checked);

    // aaValue is the actual value (0, 1, 2, 4, 8, 16)
    int aaValue = mEngineSettings.getInt("antialiasing", "Video");
    // aaIndex is the index into the allowed values in the pull down.
    int aaIndex = antiAliasingComboBox->findText(QString::number(aaValue));
    if (aaIndex != -1)
        antiAliasingComboBox->setCurrentIndex(aaIndex);

    int width = mEngineSettings.getInt("resolution x", "Video");
    int height = mEngineSettings.getInt("resolution y", "Video");
    QString resolution = QString::number(width) + QString(" x ") + QString::number(height);
    screenComboBox->setCurrentIndex(mEngineSettings.getInt("screen", "Video"));

    int resIndex = resolutionComboBox->findText(resolution, Qt::MatchStartsWith);

    if (resIndex != -1) {
        standardRadioButton->toggle();
        resolutionComboBox->setCurrentIndex(resIndex);
    } else {
        customRadioButton->toggle();
        customWidthSpinBox->setValue(width);
        customHeightSpinBox->setValue(height);
    }

    return true;
}

void Launcher::GraphicsPage::saveSettings()
{
    // Ensure we only set the new settings if they changed. This is to avoid cluttering the
    // user settings file (which by definition should only contain settings the user has touched)
    bool cVSync = vSyncCheckBox->checkState();
    if (cVSync != mEngineSettings.getBool("vsync", "Video"))
        mEngineSettings.setBool("vsync", "Video", cVSync);

    bool cFullScreen = fullScreenCheckBox->checkState();
    if (cFullScreen != mEngineSettings.getBool("fullscreen", "Video"))
        mEngineSettings.setBool("fullscreen", "Video", cFullScreen);

    bool cWindowBorder = windowBorderCheckBox->checkState();
    if (cWindowBorder != mEngineSettings.getBool("window border", "Video"))
        mEngineSettings.setBool("window border", "Video", cWindowBorder);

    int cAAValue = antiAliasingComboBox->currentText().toInt();
    if (cAAValue != mEngineSettings.getInt("antialiasing", "Video"))
        mEngineSettings.setInt("antialiasing", "Video", cAAValue);

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

    if (cWidth != mEngineSettings.getInt("resolution x", "Video"))
        mEngineSettings.setInt("resolution x", "Video", cWidth);

    if (cHeight != mEngineSettings.getInt("resolution y", "Video"))
        mEngineSettings.setInt("resolution y", "Video", cHeight);

    int cScreen = screenComboBox->currentIndex();
    if (cScreen != mEngineSettings.getInt("screen", "Video"))
        mEngineSettings.setInt("screen", "Video", cScreen);
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
