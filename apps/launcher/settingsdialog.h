#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QSettings>

#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreConfigFile.h>
#include <OgreConfigDialog.h>
#include <OgreException.h>
#include <OgreLogManager.h>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog();
    QStringList getAvailableOptions(const QString& key);
    
    Ogre::Root *root;
    Ogre::RenderSystem *mSelectedRenderSystem;
    
    QComboBox *comboRender;
    QComboBox *comboRTT;
    QComboBox *comboAA;
    QComboBox *comboResolution;
    QComboBox *comboFrequency;
    
    QCheckBox *checkVSync;
    QCheckBox *checkGamma;
    QCheckBox *checkFullScreen;
    
    QSettings *ogreConfig;
    
    QString getConfigValue(const QString& key);
    
public slots:
    void rendererChanged(const QString& renderer);
    void writeConfig();
};

#endif
