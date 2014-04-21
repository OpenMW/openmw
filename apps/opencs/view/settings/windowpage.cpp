#include "windowpage.hpp"

#include <QList>
#include <QListView>
#include <QGroupBox>
#include <QRadioButton>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStyle>

#ifdef Q_OS_MAC
#include <QPlastiqueStyle>
#endif

#include "../../model/settings/usersettings.hpp"
#include "groupblock.hpp"
#include "../../view/settings/abstractblock.hpp"

CSVSettings::WindowPage::WindowPage(QWidget *parent):
    AbstractPage("Window Size", parent)
{
    // Hacks to get the stylesheet look properly
#ifdef Q_OS_MAC
    QPlastiqueStyle *style = new QPlastiqueStyle;
    //profilesComboBox->setStyle(style);
#endif

    setupUi();
}

void CSVSettings::WindowPage::setupUi()
{
/*
    CustomBlockDef *windowSize = buildWindowSize(buildWindowSizeToggle(),
                                                 buildDefinedWindowSize(),
                                                 buildCustomWindowSize()
                                                 );

    mAbstractBlocks << buildBlock<ToggleBlock> (windowSize);

    foreach (AbstractBlock *block, mAbstractBlocks)
    {
        connect (block, SIGNAL (signalUpdateSetting (const QString &, const QString &)),
            this, SIGNAL (signalUpdateEditorSetting (const QString &, const QString &)) );
    }

    connect ( this,
              SIGNAL ( signalUpdateEditorSetting (const QString &, const QString &)),
              &(CSMSettings::UserSettings::instance()),
              SIGNAL ( signalUpdateEditorSetting (const QString &, const QString &)));*/

}
