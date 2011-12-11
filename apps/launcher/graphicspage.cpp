#include <QtGui>

#include "graphicspage.hpp"

GraphicsPage::GraphicsPage(QWidget *parent) : QWidget(parent)
{
    QGroupBox *rendererGroup = new QGroupBox(tr("Renderer"), this);

    QLabel *rendererLabel = new QLabel(tr("Rendering Subsystem:"), rendererGroup);
    mRendererComboBox = new QComboBox(rendererGroup);

    // Layout for the combobox and label
    QGridLayout *renderSystemLayout = new QGridLayout();
    renderSystemLayout->addWidget(rendererLabel, 0, 0, 1, 1);
    renderSystemLayout->addWidget(mRendererComboBox, 0, 1, 1, 1);

    mRendererStackedWidget = new QStackedWidget(rendererGroup);

    QVBoxLayout *rendererGroupLayout = new QVBoxLayout(rendererGroup);

    rendererGroupLayout->addLayout(renderSystemLayout);
    rendererGroupLayout->addWidget(mRendererStackedWidget);

    // Display
    QGroupBox *displayGroup = new QGroupBox(tr("Display"), this);

    mDisplayStackedWidget = new QStackedWidget(displayGroup);

    QVBoxLayout *displayGroupLayout = new QVBoxLayout(displayGroup);
    QSpacerItem *vSpacer3 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

    displayGroupLayout->addWidget(mDisplayStackedWidget);
    displayGroupLayout->addItem(vSpacer3);

    // Layout for the whole page
    QVBoxLayout *pageLayout = new QVBoxLayout(this);

    pageLayout->addWidget(rendererGroup);
    pageLayout->addWidget(displayGroup);

    connect(mRendererComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(rendererChanged(const QString&)));

    createPages();
    setupConfig();
    setupOgre();

    readConfig();
}

void GraphicsPage::createPages()
{
    // OpenGL rendering settings
    QWidget *mOGLRendererPage = new QWidget();

    QLabel *OGLRTTLabel = new QLabel(tr("Preferred RTT Mode:"), mOGLRendererPage);
    mOGLRTTComboBox = new QComboBox(mOGLRendererPage);

    QLabel *OGLAntiAliasingLabel = new QLabel(tr("Antialiasing:"), mOGLRendererPage);
    mOGLAntiAliasingComboBox = new QComboBox(mOGLRendererPage);

    QGridLayout *OGLRendererLayout = new QGridLayout(mOGLRendererPage);
    QSpacerItem *vSpacer1 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

    OGLRendererLayout->addWidget(OGLRTTLabel, 0, 0, 1, 1);
    OGLRendererLayout->addWidget(mOGLRTTComboBox, 0, 1, 1, 1);
    OGLRendererLayout->addWidget(OGLAntiAliasingLabel, 1, 0, 1, 1);
    OGLRendererLayout->addWidget(mOGLAntiAliasingComboBox, 1, 1, 1, 1);
    OGLRendererLayout->addItem(vSpacer1, 2, 1, 1, 1);

    // OpenGL display settings
    QWidget *mOGLDisplayPage = new QWidget();

    QLabel *OGLResolutionLabel = new QLabel(tr("Resolution:"), mOGLDisplayPage);
    mOGLResolutionComboBox = new QComboBox(mOGLDisplayPage);

    QLabel *OGLFrequencyLabel = new QLabel(tr("Display Frequency:"), mOGLDisplayPage);
    mOGLFrequencyComboBox = new QComboBox(mOGLDisplayPage);

    mOGLVSyncCheckBox = new QCheckBox(tr("Vertical Sync"), mOGLDisplayPage);
    mOGLFullScreenCheckBox = new QCheckBox(tr("Full Screen"), mOGLDisplayPage);

    QGridLayout *OGLDisplayLayout = new QGridLayout(mOGLDisplayPage);
    QSpacerItem *vSpacer2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Minimum);

    OGLDisplayLayout->addWidget(OGLResolutionLabel, 0, 0, 1, 1);
    OGLDisplayLayout->addWidget(mOGLResolutionComboBox, 0, 1, 1, 1);
    OGLDisplayLayout->addWidget(OGLFrequencyLabel, 1, 0, 1, 1);
    OGLDisplayLayout->addWidget(mOGLFrequencyComboBox, 1, 1, 1, 1);

    OGLDisplayLayout->addItem(vSpacer2, 2, 1, 1, 1);
    OGLDisplayLayout->addWidget(mOGLVSyncCheckBox, 3, 0, 1, 1);
    OGLDisplayLayout->addWidget(mOGLFullScreenCheckBox, 6, 0, 1, 1);

    // Direct3D rendering settings
    QWidget *mD3DRendererPage = new QWidget();

    QLabel *D3DRenderDeviceLabel = new QLabel(tr("Rendering Device:"), mD3DRendererPage);
    mD3DRenderDeviceComboBox = new QComboBox(mD3DRendererPage);

    QLabel *D3DAntiAliasingLabel = new QLabel(tr("Antialiasing:"), mD3DRendererPage);
    mD3DAntiAliasingComboBox = new QComboBox(mD3DRendererPage);

    QLabel *D3DFloatingPointLabel = new QLabel(tr("Floating-point Mode:"), mD3DRendererPage);
    mD3DFloatingPointComboBox = new QComboBox(mD3DRendererPage);

    mD3DNvPerfCheckBox = new QCheckBox(tr("Allow NVPerfHUD"), mD3DRendererPage);

    QGridLayout *D3DRendererLayout = new QGridLayout(mD3DRendererPage);
    QSpacerItem *vSpacer3 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Minimum);
    QSpacerItem *vSpacer4 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

    D3DRendererLayout->addWidget(D3DRenderDeviceLabel, 0, 0, 1, 1);
    D3DRendererLayout->addWidget(mD3DRenderDeviceComboBox, 0, 1, 1, 1);
    D3DRendererLayout->addWidget(D3DAntiAliasingLabel, 1, 0, 1, 1);
    D3DRendererLayout->addWidget(mD3DAntiAliasingComboBox, 1, 1, 1, 1);
    D3DRendererLayout->addWidget(D3DFloatingPointLabel, 2, 0, 1, 1);
    D3DRendererLayout->addWidget(mD3DFloatingPointComboBox, 2, 1, 1, 1);
    D3DRendererLayout->addItem(vSpacer3, 3, 1, 1, 1);
    D3DRendererLayout->addWidget(mD3DNvPerfCheckBox, 4, 0, 1, 1);
    D3DRendererLayout->addItem(vSpacer4, 5, 1, 1, 1);

    // Direct3D display settings
    QWidget *mD3DDisplayPage = new QWidget();

    QLabel *D3DResolutionLabel = new QLabel(tr("Resolution:"), mD3DDisplayPage);
    mD3DResolutionComboBox = new QComboBox(mD3DDisplayPage);

    mD3DVSyncCheckBox = new QCheckBox(tr("Vertical Sync"), mD3DDisplayPage);
    mD3DFullScreenCheckBox = new QCheckBox(tr("Full Screen"), mD3DDisplayPage);

    QGridLayout *mD3DDisplayLayout = new QGridLayout(mD3DDisplayPage);
    QSpacerItem *vSpacer5 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Minimum);

    mD3DDisplayLayout->addWidget(D3DResolutionLabel, 0, 0, 1, 1);
    mD3DDisplayLayout->addWidget(mD3DResolutionComboBox, 0, 1, 1, 1);
    mD3DDisplayLayout->addItem(vSpacer5, 1, 1, 1, 1);
    mD3DDisplayLayout->addWidget(mD3DVSyncCheckBox, 2, 0, 1, 1);
    mD3DDisplayLayout->addWidget(mD3DFullScreenCheckBox, 5, 0, 1, 1);

    // Add the created pages
    mRendererStackedWidget->addWidget(mOGLRendererPage);
    mRendererStackedWidget->addWidget(mD3DRendererPage);

    mDisplayStackedWidget->addWidget(mOGLDisplayPage);
    mDisplayStackedWidget->addWidget(mD3DDisplayPage);
}

void GraphicsPage::setupConfig()
{
    QString ogreCfg = mCfg.getOgreConfigPath().string().c_str();
    QFile file(ogreCfg);
    mOgreConfig = new QSettings(ogreCfg, QSettings::IniFormat);
}

void GraphicsPage::setupOgre()
{
    QString pluginCfg = mCfg.getPluginsConfigPath().string().c_str();
    QFile file(pluginCfg);

    // Create a log manager so we can surpress debug text to stdout/stderr
    Ogre::LogManager* logMgr = OGRE_NEW Ogre::LogManager;
    logMgr->createLog((mCfg.getLogPath().string() + "/launcherOgre.log"), true, false, false);

    QString ogreCfg = QString::fromStdString(mCfg.getOgreConfigPath().string());
    file.setFileName(ogreCfg);

    //we need to check that the path to the configuration file exists before we
    //try and create an instance of Ogre::Root otherwise Ogre raises an exception
    QDir configDir = QFileInfo(file).dir();
    if ( !configDir.exists() && !configDir.mkpath(configDir.path()) )
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error creating config file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(QString(tr("<br><b>Failed to create the configuration file</b><br><br> \
        Make sure you have write access to<br>%1<br><br>")).arg(configDir.path()));
        msgBox.exec();

        QApplication::exit(1);
        return;
    }

    try
    {
        mOgre = new Ogre::Root(pluginCfg.toStdString(), file.fileName().toStdString(), "./launcherOgre.log");
    }
    catch(Ogre::Exception &ex)
    {
        QString ogreError = QString::fromStdString(ex.getFullDescription().c_str());
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error creating Ogre::Root");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Failed to create the Ogre::Root object</b><br><br> \
        Make sure the plugins.cfg is present and valid.<br><br> \
        Press \"Show Details...\" for more information.<br>"));
        msgBox.setDetailedText(ogreError);
        msgBox.exec();

        qCritical("Error creating Ogre::Root, the error reported was:\n %s", qPrintable(ogreError));

        QApplication::exit(1);
        return;
    }

    // Get the available renderers and put them in the combobox
    const Ogre::RenderSystemList &renderers = mOgre->getAvailableRenderers();

    for (Ogre::RenderSystemList::const_iterator r = renderers.begin(); r != renderers.end(); ++r) {
        mSelectedRenderSystem = *r;
        mRendererComboBox->addItem((*r)->getName().c_str());
    }

    int index = mRendererComboBox->findText(mOgreConfig->value("Render System").toString());

    if ( index != -1) {
        mRendererComboBox->setCurrentIndex(index);
    }

    // Create separate rendersystems
    QString openGLName = mRendererComboBox->itemText(mRendererComboBox->findText(QString("OpenGL"), Qt::MatchStartsWith));
    QString direct3DName = mRendererComboBox->itemText(mRendererComboBox->findText(QString("Direct3D"), Qt::MatchStartsWith));

    mOpenGLRenderSystem = mOgre->getRenderSystemByName(openGLName.toStdString());
    mDirect3DRenderSystem = mOgre->getRenderSystemByName(direct3DName.toStdString());

    if (!mOpenGLRenderSystem && !mDirect3DRenderSystem) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error creating renderer");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not select a valid render system</b><br><br> \
        Please make sure the plugins.cfg file exists and contains a valid rendering plugin.<br>"));
        msgBox.exec();

        QApplication::exit(1);
        return;
    }

    // Now fill the GUI elements
    // OpenGL
    if (mOpenGLRenderSystem) {
        mOGLRTTComboBox->addItems(getAvailableOptions(QString("RTT Preferred Mode"), mOpenGLRenderSystem));
        mOGLAntiAliasingComboBox->addItems(getAvailableOptions(QString("FSAA"), mOpenGLRenderSystem));

        QStringList videoModes = getAvailableOptions(QString("Video Mode"), mOpenGLRenderSystem);
        // Remove extraneous spaces
        videoModes.replaceInStrings(QRegExp("\\s{2,}"), QString(" "));
        videoModes.replaceInStrings(QRegExp("^\\s"), QString());

        mOGLResolutionComboBox->addItems(videoModes);
        mOGLFrequencyComboBox->addItems(getAvailableOptions(QString("Display Frequency"), mOpenGLRenderSystem));
    }

    // Direct3D
    if (mDirect3DRenderSystem) {
        mD3DRenderDeviceComboBox->addItems(getAvailableOptions(QString("Rendering Device"), mDirect3DRenderSystem));
        mD3DAntiAliasingComboBox->addItems(getAvailableOptions(QString("FSAA"), mDirect3DRenderSystem));
        mD3DFloatingPointComboBox->addItems(getAvailableOptions(QString("Floating-point mode"), mDirect3DRenderSystem));

        QStringList videoModes = getAvailableOptions(QString("Video Mode"), mDirect3DRenderSystem);
        // Remove extraneous spaces
        videoModes.replaceInStrings(QRegExp("\\s{2,}"), QString(" "));
        videoModes.replaceInStrings(QRegExp("^\\s"), QString());
        mD3DResolutionComboBox->addItems(videoModes);
    }
}

void GraphicsPage::readConfig()
{
    // Read the config file settings
    if (mOpenGLRenderSystem) {

        int index = mOGLRTTComboBox->findText(getConfigValue("RTT Preferred Mode", mOpenGLRenderSystem));
        if ( index != -1) {
            mOGLRTTComboBox->setCurrentIndex(index);
        }

        index = mOGLAntiAliasingComboBox->findText(getConfigValue("FSAA", mOpenGLRenderSystem));
        if ( index != -1){
            mOGLAntiAliasingComboBox->setCurrentIndex(index);
        }

        index = mOGLResolutionComboBox->findText(getConfigValue("Video Mode", mOpenGLRenderSystem));
        if ( index != -1) {
            mOGLResolutionComboBox->setCurrentIndex(index);
        }

        index = mOGLFrequencyComboBox->findText(getConfigValue("Display Frequency", mOpenGLRenderSystem));
        if ( index != -1) {
            mOGLFrequencyComboBox->setCurrentIndex(index);
        }

        // Now we do the same for the checkboxes
        if (getConfigValue("VSync", mOpenGLRenderSystem) == QLatin1String("Yes")) {
            mOGLVSyncCheckBox->setCheckState(Qt::Checked);
        }

        if (getConfigValue("Full Screen", mOpenGLRenderSystem) == QLatin1String("Yes")) {
            mOGLFullScreenCheckBox->setCheckState(Qt::Checked);
        }
    }

    if (mDirect3DRenderSystem) {

        int index = mD3DRenderDeviceComboBox->findText(getConfigValue("Rendering Device", mDirect3DRenderSystem));
        if ( index != -1) {
            mD3DRenderDeviceComboBox->setCurrentIndex(index);
        }

        index = mD3DAntiAliasingComboBox->findText(getConfigValue("FSAA", mDirect3DRenderSystem));
        if ( index != -1) {
            mD3DAntiAliasingComboBox->setCurrentIndex(index);
        }

        index = mD3DFloatingPointComboBox->findText(getConfigValue("Floating-point mode", mDirect3DRenderSystem));
        if ( index != -1) {
            mD3DFloatingPointComboBox->setCurrentIndex(index);
        }

        index = mD3DResolutionComboBox->findText(getConfigValue("Video Mode", mDirect3DRenderSystem));
        if ( index != -1) {
            mD3DResolutionComboBox->setCurrentIndex(index);
        }

        if (getConfigValue("Allow NVPerfHUD", mDirect3DRenderSystem) == QLatin1String("Yes")) {
                mD3DNvPerfCheckBox->setCheckState(Qt::Checked);
        }

        if (getConfigValue("VSync", mDirect3DRenderSystem) == QLatin1String("Yes")) {
                mD3DVSyncCheckBox->setCheckState(Qt::Checked);
        }

        if (getConfigValue("Full Screen", mDirect3DRenderSystem) == QLatin1String("Yes")) {
                mD3DFullScreenCheckBox->setCheckState(Qt::Checked);
        }
    }
}

void GraphicsPage::writeConfig()
{
    mOgre->setRenderSystem(mSelectedRenderSystem);

    if (mDirect3DRenderSystem) {
        // Nvidia Performance HUD
        if (mD3DNvPerfCheckBox->checkState() == Qt::Checked) {
            mDirect3DRenderSystem->setConfigOption("Allow NVPerfHUD", "Yes");
        } else {
            mDirect3DRenderSystem->setConfigOption("Allow NVPerfHUD", "No");
        }

        // Antialiasing
        mDirect3DRenderSystem->setConfigOption("FSAA", mD3DAntiAliasingComboBox->currentText().toStdString());

        // Full screen
        if (mD3DFullScreenCheckBox->checkState() == Qt::Checked) {
            mDirect3DRenderSystem->setConfigOption("Full Screen", "Yes");
        } else {
            mDirect3DRenderSystem->setConfigOption("Full Screen", "No");
        }

        // Rendering device
        mDirect3DRenderSystem->setConfigOption("Rendering Device", mD3DRenderDeviceComboBox->currentText().toStdString());

        // VSync
        if (mD3DVSyncCheckBox->checkState() == Qt::Checked) {
            mDirect3DRenderSystem->setConfigOption("VSync", "Yes");
        } else {
            mDirect3DRenderSystem->setConfigOption("VSync", "No");
        }

        // Resolution
        mDirect3DRenderSystem->setConfigOption("Video Mode", mD3DResolutionComboBox->currentText().toStdString());
    }

    if (mOpenGLRenderSystem) {
        // Display Frequency
        mOpenGLRenderSystem->setConfigOption("Display Frequency", mOGLFrequencyComboBox->currentText().toStdString());

        // Antialiasing
        mOpenGLRenderSystem->setConfigOption("FSAA", mOGLAntiAliasingComboBox->currentText().toStdString());

        // Full screen
        if (mOGLFullScreenCheckBox->checkState() == Qt::Checked) {
            mOpenGLRenderSystem->setConfigOption("Full Screen", "Yes");
        } else {
            mOpenGLRenderSystem->setConfigOption("Full Screen", "No");
        }

        // RTT mode
        mOpenGLRenderSystem->setConfigOption("RTT Preferred Mode", mOGLRTTComboBox->currentText().toStdString());

        // VSync
        if (mOGLVSyncCheckBox->checkState() == Qt::Checked) {
            mOpenGLRenderSystem->setConfigOption("VSync", "Yes");
        } else {
            mOpenGLRenderSystem->setConfigOption("VSync", "No");
        }

        // Resolution
        mOpenGLRenderSystem->setConfigOption("Video Mode", mOGLResolutionComboBox->currentText().toStdString());
    }

    // Now we validate the options
    QString ogreError = QString::fromStdString(mSelectedRenderSystem->validateConfigOptions());

    if (!ogreError.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error validating Ogre configuration");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>A problem occured while validating the graphics options</b><br><br> \
        The graphics options could not be saved.<br><br> \
        Press \"Show Details...\" for more information.<br>"));
        msgBox.setDetailedText(ogreError);
        msgBox.exec();

        Ogre::LogManager::getSingletonPtr()->logMessage( "Caught exception in validateConfigOptions");

        qCritical("Error validating configuration");

        QApplication::exit(1);
        return;
    }

    // Write the settings to the config file


    try
    {
        mOgre->saveConfig();
    }
    catch(Ogre::Exception &ex)
    {
        QString ogreError = QString::fromStdString(ex.getFullDescription().c_str());
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error writing Ogre configuration file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not write the graphics configuration</b><br><br> \
        Please make sure you have the right permissions and try again.<br><br> \
        Press \"Show Details...\" for more information.<br>"));
        msgBox.setDetailedText(ogreError);
        msgBox.exec();

        qCritical("Error saving Ogre configuration, the error reported was:\n %s", qPrintable(ogreError));

        QApplication::exit(1);
    }

}

QString GraphicsPage::getConfigValue(const QString &key, Ogre::RenderSystem *renderer)
{
    QString result;

    mOgreConfig->beginGroup(renderer->getName().c_str());
    result = mOgreConfig->value(key).toString();
    mOgreConfig->endGroup();

    return result;
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
                     result << (*opt_it).c_str();
             }

    }

    return result;
}

void GraphicsPage::rendererChanged(const QString &renderer)
{
    if (renderer.contains("Direct3D")) {
        mRendererStackedWidget->setCurrentIndex(1);
        mDisplayStackedWidget->setCurrentIndex(1);
    }

    if (renderer.contains("OpenGL")) {
        mRendererStackedWidget->setCurrentIndex(0);
        mDisplayStackedWidget->setCurrentIndex(0);
    }

    mSelectedRenderSystem = mOgre->getRenderSystemByName(renderer.toStdString());
}
