#include <QtGui>

#include "settingsdialog.h"

SettingsDialog::SettingsDialog()
{
    QTabWidget *tabWidget = new QTabWidget(this);
    QWidget *graphicsTab = new QWidget();

    // Render group
    QGroupBox *groupRender = new QGroupBox(tr("Renderer"), graphicsTab);
    groupRender->setMinimumWidth(450);

    QVBoxLayout *groupRenderLayout = new QVBoxLayout(groupRender);
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    QVBoxLayout *pageLayout = new QVBoxLayout(graphicsTab);

    QGridLayout *renderLayout = new QGridLayout();

    QLabel *labelRender = new QLabel(tr("Rendering Subsystem:"), groupRender);
    comboRender = new QComboBox(groupRender);

    QLabel *labelRTT = new QLabel(tr("Preferred RTT Mode:"), groupRender);
    comboRTT = new QComboBox(groupRender);

    QLabel *labelAA = new QLabel(tr("Antialiasing:"), groupRender);
    comboAA = new QComboBox(groupRender);

    renderLayout->addWidget(labelRender, 0, 0, 1, 1);
    renderLayout->addWidget(comboRender, 0, 1, 1, 1);

    QSpacerItem *vSpacer1 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Minimum);
    renderLayout->addItem(vSpacer1, 1, 1, 1, 1);

    renderLayout->addWidget(labelRTT, 2, 0, 1, 1);
    renderLayout->addWidget(comboRTT, 2, 1, 1, 1);
    renderLayout->addWidget(labelAA, 3, 0, 1, 1);
    renderLayout->addWidget(comboAA, 3, 1, 1, 1);

    QSpacerItem *vSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    groupRenderLayout->addLayout(renderLayout);
    groupRenderLayout->addItem(vSpacer2);

    pageLayout->addWidget(groupRender);

    // Display group
    QGroupBox *groupDisplay = new QGroupBox(tr("Display"), graphicsTab);
    QVBoxLayout *groupDisplayLayout = new QVBoxLayout(groupDisplay);

    QGridLayout *displayLayout = new QGridLayout();

    QLabel *labelResolution = new QLabel(tr("Resolution:"), groupDisplay);
    comboResolution = new QComboBox(groupDisplay);

    QLabel *labelFrequency = new QLabel(tr("Display Frequency:"), groupDisplay);
    comboFrequency = new QComboBox(groupDisplay);

    checkVSync = new QCheckBox(tr("Vertical Sync"), groupDisplay);
    checkGamma = new QCheckBox(tr("sRGB Gamma Conversion"), groupDisplay);
    checkFullScreen = new QCheckBox(tr("Full Screen"), groupDisplay);

    displayLayout->addWidget(labelResolution, 0, 0, 1, 1);
    displayLayout->addWidget(comboResolution, 0, 1, 1, 1);
    displayLayout->addWidget(labelFrequency, 1, 0, 1, 1);
    displayLayout->addWidget(comboFrequency, 1, 1, 1, 1);

    QSpacerItem *vSpacer3 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Minimum);
    displayLayout->addItem(vSpacer3, 2, 1, 1, 1);

    displayLayout->addWidget(checkVSync, 3, 0, 1, 1);
    displayLayout->addWidget(checkGamma, 3, 1, 1, 1);
    displayLayout->addWidget(checkFullScreen, 6, 0, 1, 1);

    QSpacerItem *vSpacer4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    groupDisplayLayout->addLayout(displayLayout);
    groupDisplayLayout->addItem(vSpacer4);

    pageLayout->addWidget(groupDisplay);


    tabWidget->addTab(graphicsTab, QString(tr("Graphics")));
    tabWidget->setCurrentIndex(0);

    dialogLayout->addWidget(tabWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    dialogLayout->addWidget(buttonBox);

    setWindowTitle(tr("Settings"));
    setWindowIcon(QIcon::fromTheme("preferences-other"));

    // Ogre Settings
    // TODO: Find out a way to do this nice and platform-independent
    QString filepath = QDir::homePath();
    filepath.append("/.config/openmw/ogre.cfg");

    ogreConfig = new QSettings(filepath, QSettings::IniFormat);

    // Signals and slots
    connect(comboRender, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(rendererChanged(const QString&)));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(writeConfig()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));

     // Ogre stuff
    root = new Ogre::Root("plugins.cfg");

    // Get the available renderers and put them in the combobox
    const Ogre::RenderSystemList &renderers = root->getAvailableRenderers();

    for (Ogre::RenderSystemList::const_iterator r = renderers.begin(); r != renderers.end(); ++r) {
        comboRender->addItem((*r)->getName().c_str());
    }

    int index = comboRender->findText(getConfigValue("Render System"));

    if ( index != -1) {
        comboRender->setCurrentIndex(index);
    }

}

QStringList SettingsDialog::getAvailableOptions(const QString& key)
{
    QStringList result;

    uint row = 0;
    Ogre::ConfigOptionMap options = mSelectedRenderSystem->getConfigOptions();

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

void SettingsDialog::rendererChanged(const QString& renderer)
{
    // Set the render system to the selected one
    mSelectedRenderSystem = root->getRenderSystemByName(renderer.toStdString().c_str());

    comboRTT->addItems(getAvailableOptions("RTT Preferred Mode"));
    comboAA->addItems(getAvailableOptions("FSAA"));
    comboResolution->addItems(getAvailableOptions("Video Mode"));
    comboFrequency->addItems(getAvailableOptions("Display Frequency"));

    // Get the value for the option the config file, find if it's in the combobox.
    // If it's found, set the current index to that value, otherwise: ignore.

    int index = comboRTT->findText(getConfigValue("RTT Preferred Mode"));
    if ( index != -1)
    comboRTT->setCurrentIndex(index);

    index = comboAA->findText(getConfigValue("FSAA"));
    if ( index != -1)
    comboAA->setCurrentIndex(index);

    index = comboResolution->findText(getConfigValue("Video Mode"));
    if ( index != -1)
    comboResolution->setCurrentIndex(index);

    index = comboFrequency->findText(getConfigValue("Display Frequency"));
    if ( index != -1)
    comboFrequency->setCurrentIndex(index);

    // Now we do the same for the checkboxes
    if (QString::compare(getConfigValue("VSync"), QString("Yes")) == 0) {
        checkVSync->setCheckState(Qt::Checked);
    }

    if (QString::compare(getConfigValue("sRGB Gamma Conversion"), QString("Yes")) == 0) {
        checkGamma->setCheckState(Qt::Checked);
    }

    if (QString::compare(getConfigValue("Full Screen"), QString("Yes")) == 0) {
        checkFullScreen->setCheckState(Qt::Checked);
    }

}

QString SettingsDialog::getConfigValue(const QString& key)
{
    QString result;

    ogreConfig->beginGroup(mSelectedRenderSystem->getName().c_str());
    result = ogreConfig->value(key).toString();
    ogreConfig->endGroup();

    return result;
}

void SettingsDialog::writeConfig()
{
    // Get the values from the GUI elements and write them to the config file

    // Custom write method: We cannot use QSettings because it does not accept spaces

    QString keyName;
    QString fileName;

    QFile file(ogreConfig->fileName());

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        // File could not be opened, TODO error
        close();

    QTextStream out(&file);

    out << "Render System=" << mSelectedRenderSystem->getName().c_str() << endl << endl;

    // add brackets to the renderer's name
    QString renderer = mSelectedRenderSystem->getName().c_str();
    renderer.prepend("[");
    renderer.append("]");
    out << renderer << endl;

    out << "Display Frequency=" << comboFrequency->currentText() << endl;
    out << "FSAA=" << comboAA->currentText() << endl;

    if (checkFullScreen->checkState() == Qt::Checked) {
        out << "Full Screen=Yes" << endl;
    } else {
        out << "Full Screen=No" << endl;
    }

    out << "RTT Preferred Mode=" << comboRTT->currentText() << endl;

    if (checkVSync->checkState() == Qt::Checked) {
        out << "VSync=Yes" << endl;
    } else {
        out << "VSync=No" << endl;
    }

    out << "Video Mode=" << comboResolution->currentText() << endl;

    if (checkGamma->checkState() == Qt::Checked) {
        out << "sRGB Gamma Conversion=Yes" << endl;
    } else {
        out << "sRGB Gamma Conversion=No" << endl;
    }

    file.close();

    close(); // Exit dialog
}

