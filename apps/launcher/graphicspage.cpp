#include <QtGui>

#include <cstdlib>

#include <boost/math/common_factor.hpp>
#include <boost/filesystem.hpp>

#include <components/files/configurationmanager.hpp>
#include <components/settings/settings.hpp>

#include "graphicspage.hpp"
#include "naturalsort.hpp"

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

GraphicsPage::GraphicsPage(Files::ConfigurationManager &cfg, QWidget *parent)
    : QWidget(parent)
    , mCfgMgr(cfg)
{
    QGroupBox *rendererGroup = new QGroupBox(tr("Renderer"), this);

    QLabel *rendererLabel = new QLabel(tr("Rendering Subsystem:"), rendererGroup);
    mRendererComboBox = new QComboBox(rendererGroup);

    // Layout for the combobox and label
    QGridLayout *renderSystemLayout = new QGridLayout();
    renderSystemLayout->addWidget(rendererLabel, 0, 0, 1, 1);
    renderSystemLayout->addWidget(mRendererComboBox, 0, 1, 1, 1);

    // Display
    QGroupBox *displayGroup = new QGroupBox(tr("Display"), this);

    mVSyncCheckBox = new QCheckBox(tr("Vertical Sync"), displayGroup);
    mFullScreenCheckBox = new QCheckBox(tr("Full Screen"), displayGroup);

    QLabel *antiAliasingLabel = new QLabel(tr("Antialiasing:"), displayGroup);
    QLabel *resolutionLabel = new QLabel(tr("Resolution:"), displayGroup);

    mResolutionComboBox = new QComboBox(displayGroup);
    mAntiAliasingComboBox = new QComboBox(displayGroup);

    QVBoxLayout *rendererGroupLayout = new QVBoxLayout(rendererGroup);
    rendererGroupLayout->addLayout(renderSystemLayout);

    QGridLayout *displayGroupLayout = new QGridLayout(displayGroup);
    displayGroupLayout->addWidget(mVSyncCheckBox, 0, 0, 1, 1);
    displayGroupLayout->addWidget(mFullScreenCheckBox, 1, 0, 1, 1);
    displayGroupLayout->addWidget(antiAliasingLabel, 2, 0, 1, 1);
    displayGroupLayout->addWidget(mAntiAliasingComboBox, 2, 1, 1, 1);
    displayGroupLayout->addWidget(resolutionLabel, 3, 0, 1, 1);
    displayGroupLayout->addWidget(mResolutionComboBox, 3, 1, 1, 1);

    // Layout for the whole page
    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    QSpacerItem *vSpacer1 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

    pageLayout->addWidget(rendererGroup);
    pageLayout->addWidget(displayGroup);
    pageLayout->addItem(vSpacer1);

    connect(mRendererComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(rendererChanged(const QString&)));
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

    std::string glPlugin = std::string(pluginDir) + "/RenderSystem_GL" + OGRE_PLUGIN_DEBUG_SUFFIX;
    if (boost::filesystem::exists(glPlugin + ".so") || boost::filesystem::exists(glPlugin + ".dll"))
        mOgre->loadPlugin (glPlugin);

    std::string dxPlugin = std::string(pluginDir) + "/RenderSystem_Direct3D9" + OGRE_PLUGIN_DEBUG_SUFFIX;
    if (boost::filesystem::exists(dxPlugin + ".so") || boost::filesystem::exists(dxPlugin + ".dll"))
        mOgre->loadPlugin (dxPlugin);

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
        mRendererComboBox->addItem((*r)->getName().c_str());
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
    int index = mRendererComboBox->findText(QString::fromStdString(Settings::Manager::getString("render system", "Video")));

    if ( index != -1) {
        mRendererComboBox->setCurrentIndex(index);
    }
    else
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        mRendererComboBox->setCurrentIndex(mRendererComboBox->findText(direct3DName));
#else
        mRendererComboBox->setCurrentIndex(mRendererComboBox->findText(openGLName));
#endif
    }

    mAntiAliasingComboBox->clear();
    mResolutionComboBox->clear();
    mAntiAliasingComboBox->addItems(getAvailableOptions(QString("FSAA"), mSelectedRenderSystem));
    mResolutionComboBox->addItems(getAvailableResolutions(mSelectedRenderSystem));

    readConfig();
    return true;
}

void GraphicsPage::readConfig()
{
    if (Settings::Manager::getBool("vsync", "Video"))
        mVSyncCheckBox->setCheckState(Qt::Checked);

    if (Settings::Manager::getBool("fullscreen", "Video"))
        mFullScreenCheckBox->setCheckState(Qt::Checked);

    int aaIndex = mAntiAliasingComboBox->findText(QString::fromStdString(Settings::Manager::getString("antialiasing", "Video")));
    if (aaIndex != -1)
        mAntiAliasingComboBox->setCurrentIndex(aaIndex);

    QString resolution = QString::number(Settings::Manager::getInt("resolution x", "Video"));
    resolution.append(" x " + QString::number(Settings::Manager::getInt("resolution y", "Video")));

    int resIndex = mResolutionComboBox->findText(resolution, Qt::MatchStartsWith);
    if (resIndex != -1)
        mResolutionComboBox->setCurrentIndex(resIndex);
}

void GraphicsPage::writeConfig()
{
    Settings::Manager::setBool("vsync", "Video", mVSyncCheckBox->checkState());
    Settings::Manager::setBool("fullscreen", "Video", mFullScreenCheckBox->checkState());
    Settings::Manager::setString("antialiasing", "Video", mAntiAliasingComboBox->currentText().toStdString());
    Settings::Manager::setString("render system", "Video", mRendererComboBox->currentText().toStdString());

    // Get the current resolution, but with the tabs replaced with a single space
    QString resolution = mResolutionComboBox->currentText().simplified();
    QStringList tokens = resolution.split(" ", QString::SkipEmptyParts);

    int resX = tokens.at(0).toInt();
    int resY = tokens.at(2).toInt();
    Settings::Manager::setInt("resolution x", "Video", resX);
    Settings::Manager::setInt("resolution y", "Video", resY);
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
        for (opt_it = i->second.possibleValues.begin ();
        opt_it != i->second.possibleValues.end (); opt_it++, idx++)
        {

            if (strcmp (key.toStdString().c_str(), i->first.c_str()) == 0)
            {
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
    QString key ("Video Mode");
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
            QString qval = QString::fromStdString(*opt_it).simplified();
            // remove extra tokens after the resolution (for example bpp, can be there or not depending on rendersystem)
            QStringList tokens = qval.split(" ", QString::SkipEmptyParts);
            assert (tokens.size() >= 3);
            QString resolutionStr = tokens.at(0) + QString(" x ") + tokens.at(2);

            // do not add duplicate resolutions
            if (!result.contains(resolutionStr)) {

                QString aspect = getAspect(tokens.at(0).toInt(),tokens.at(2).toInt());

                if (aspect == QLatin1String("16:9") || aspect == QLatin1String("16:10")) {
                    resolutionStr.append(tr("\t(Widescreen ") + aspect + ")");

                } else if (aspect == QLatin1String("4:3")) {
                    resolutionStr.append(tr("\t(Standard 4:3)"));
                }

                result << resolutionStr;
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

    mAntiAliasingComboBox->clear();
    mResolutionComboBox->clear();

    mAntiAliasingComboBox->addItems(getAvailableOptions(QString("FSAA"), mSelectedRenderSystem));
    mResolutionComboBox->addItems(getAvailableResolutions(mSelectedRenderSystem));
}
