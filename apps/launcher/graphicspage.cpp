#include "graphicspage.hpp"

#include <QDesktopWidget>
#include <QMessageBox>
#include <QDir>

#include <cstdlib>

#include <boost/math/common_factor.hpp>

#include <components/files/configurationmanager.hpp>
#include <components/files/ogreplugin.hpp>

#include <components/fileorderlist/utils/naturalsort.hpp>

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

GraphicsPage::GraphicsPage(Files::ConfigurationManager &cfg, GraphicsSettings &graphicsSetting, QWidget *parent)
    : mCfgMgr(cfg)
    , mGraphicsSettings(graphicsSetting)
    , QWidget(parent)
{
    setupUi(this);

    // Set the maximum res we can set in windowed mode
    QRect res = QApplication::desktop()->screenGeometry();
    customWidthSpinBox->setMaximum(res.width());
    customHeightSpinBox->setMaximum(res.height());

    connect(rendererComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(rendererChanged(const QString&)));
    connect(fullScreenCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotFullScreenChanged(int)));
    connect(standardRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotStandardToggled(bool)));

}

bool GraphicsPage::setupOgre()
{
    // Create a log manager so we can surpress debug text to stdout/stderr
    Ogre::LogManager* logMgr = OGRE_NEW Ogre::LogManager;
    logMgr->createLog((mCfgMgr.getLogPath().string() + "/launcherOgre.log"), true, false, false);

    try
    {
        mOgre = new Ogre::Root("", "", "./launcherOgre.log");
    }
    catch(Ogre::Exception &ex)
    {
        QString ogreError = QString::fromStdString(ex.getFullDescription().c_str());
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


    std::string pluginDir;
    const char* pluginEnv = getenv("OPENMW_OGRE_PLUGIN_DIR");
    if (pluginEnv)
        pluginDir = pluginEnv;
    else
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        pluginDir = ".\\";
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        pluginDir = OGRE_PLUGIN_DIR;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        pluginDir = OGRE_PLUGIN_DIR_REL;
#endif
    }

    QDir dir(QString::fromStdString(pluginDir));
    pluginDir = dir.absolutePath().toStdString();

    Files::loadOgrePlugin(pluginDir, "RenderSystem_GL", *mOgre);
    Files::loadOgrePlugin(pluginDir, "RenderSystem_GL3Plus", *mOgre);
    Files::loadOgrePlugin(pluginDir, "RenderSystem_Direct3D9", *mOgre);

#ifdef ENABLE_PLUGIN_GL
    mGLPlugin = new Ogre::GLPlugin();
    mOgre->installPlugin(mGLPlugin);
#endif
#ifdef ENABLE_PLUGIN_Direct3D9
    mD3D9Plugin = new Ogre::D3D9Plugin();
    mOgre->installPlugin(mD3D9Plugin);
#endif

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
                          Please make sure the plugins.cfg file exists and contains a valid rendering plugin.<br>"));
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
    resolutionComboBox->clear();
    antiAliasingComboBox->addItems(getAvailableOptions(QString("FSAA"), mSelectedRenderSystem));
    resolutionComboBox->addItems(getAvailableResolutions(mSelectedRenderSystem));

    // Load the rest of the values
    loadSettings();
    return true;
}

void GraphicsPage::loadSettings()
{
    if (mGraphicsSettings.value(QString("Video/vsync")) == QLatin1String("true"))
        vSyncCheckBox->setCheckState(Qt::Checked);

    if (mGraphicsSettings.value(QString("Video/fullscreen")) == QLatin1String("true"))
        fullScreenCheckBox->setCheckState(Qt::Checked);

    int aaIndex = antiAliasingComboBox->findText(mGraphicsSettings.value(QString("Video/antialiasing")));
    if (aaIndex != -1)
        antiAliasingComboBox->setCurrentIndex(aaIndex);

    QString width = mGraphicsSettings.value(QString("Video/resolution x"));
    QString height = mGraphicsSettings.value(QString("Video/resolution y"));
    QString resolution = width + QString(" x ") + height;

    int resIndex = resolutionComboBox->findText(resolution, Qt::MatchStartsWith);

    if (resIndex != -1) {
        standardRadioButton->toggle();
        resolutionComboBox->setCurrentIndex(resIndex);
    } else {
        customRadioButton->toggle();
        customWidthSpinBox->setValue(width.toInt());
        customHeightSpinBox->setValue(height.toInt());

    }
}

void GraphicsPage::saveSettings()
{
    vSyncCheckBox->checkState() ? mGraphicsSettings.setValue(QString("Video/vsync"), QString("true"))
                                 : mGraphicsSettings.setValue(QString("Video/vsync"), QString("false"));

    fullScreenCheckBox->checkState() ? mGraphicsSettings.setValue(QString("Video/fullscreen"), QString("true"))
                                      : mGraphicsSettings.setValue(QString("Video/fullscreen"), QString("false"));

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
}

QStringList GraphicsPage::getAvailableOptions(const QString &key, Ogre::RenderSystem *renderer)
{
    QStringList result;

    uint row = 0;
    Ogre::ConfigOptionMap options = renderer->getConfigOptions();

    for (Ogre::ConfigOptionMap::iterator i = options.begin (); i != options.end (); i++, row++)
    {
        Ogre::StringVector::iterator opt_it;
        uint idx = 0;

        for (opt_it = i->second.possibleValues.begin();
             opt_it != i->second.possibleValues.end(); opt_it++, idx++)
        {
            if (strcmp (key.toStdString().c_str(), i->first.c_str()) == 0) {
                result << ((key == "FSAA") ? QString("MSAA ") : QString("")) + QString::fromStdString((*opt_it).c_str()).simplified();
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

QStringList GraphicsPage::getAvailableResolutions(Ogre::RenderSystem *renderer)
{
    QString key("Video Mode");
    QStringList result;

    uint row = 0;
    Ogre::ConfigOptionMap options = renderer->getConfigOptions();

    for (Ogre::ConfigOptionMap::iterator i = options.begin (); i != options.end (); i++, row++)
    {
        if (key.toStdString() != i->first)
            continue;

        Ogre::StringVector::iterator opt_it;
        uint idx = 0;

        for (opt_it = i->second.possibleValues.begin ();
             opt_it != i->second.possibleValues.end (); opt_it++, idx++)
        {
            QRegExp resolutionRe(QString("(\\d+) x (\\d+).*"));
            QString resolution = QString::fromStdString(*opt_it).simplified();

            if (resolutionRe.exactMatch(resolution)) {

                int width = resolutionRe.cap(1).toInt();
                int height = resolutionRe.cap(2).toInt();

                QString aspect = getAspect(width, height);
                QString cleanRes = resolutionRe.cap(1) + QString(" x ") + resolutionRe.cap(2);

                if (aspect == QLatin1String("16:9") || aspect == QLatin1String("16:10")) {
                    cleanRes.append(tr("\t(Wide ") + aspect + ")");

                } else if (aspect == QLatin1String("4:3")) {
                    cleanRes.append(tr("\t(Standard 4:3)"));
                }
                // do not add duplicate resolutions
                if (!result.contains(cleanRes))
                    result.append(cleanRes);
            }
        }
    }

    // Sort the resolutions in descending order
    qSort(result.begin(), result.end(), naturalSortGreaterThanCI);

    return result;
}

void GraphicsPage::rendererChanged(const QString &renderer)
{
    mSelectedRenderSystem = mOgre->getRenderSystemByName(renderer.toStdString());

    antiAliasingComboBox->clear();
    resolutionComboBox->clear();

    antiAliasingComboBox->addItems(getAvailableOptions(QString("FSAA"), mSelectedRenderSystem));
    resolutionComboBox->addItems(getAvailableResolutions(mSelectedRenderSystem));
}

void GraphicsPage::slotFullScreenChanged(int state)
{
    if (state == Qt::Checked) {
        standardRadioButton->toggle();
        customRadioButton->setEnabled(false);
        customWidthSpinBox->setEnabled(false);
        customHeightSpinBox->setEnabled(false);
    } else {
        customRadioButton->setEnabled(true);
        customWidthSpinBox->setEnabled(true);
        customHeightSpinBox->setEnabled(true);
    }
}

void GraphicsPage::slotStandardToggled(bool checked)
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
