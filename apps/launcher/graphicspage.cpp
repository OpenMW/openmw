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
    : QWidget(parent)
    , mCfgMgr(cfg)
    , mGraphicsSettings(graphicsSetting)
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
