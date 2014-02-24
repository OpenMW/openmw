#include "blankpage.hpp"

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
#include "toggleblock.hpp"

CSVSettings::BlankPage::BlankPage(QWidget *parent):
    AbstractPage("Blank", parent)
{

}

CSVSettings::BlankPage::BlankPage(const QString &title, QWidget *parent):
    AbstractPage(title, parent)
{
    // Hacks to get the stylesheet look properly
#ifdef Q_OS_MAC
    QPlastiqueStyle *style = new QPlastiqueStyle;
    //profilesComboBox->setStyle(style);
#endif

    setupUi();
}

void CSVSettings::BlankPage::setupUi()
{
    QGroupBox *pageBox = new QGroupBox(this);
    layout()->addWidget(pageBox);
}

void CSVSettings::BlankPage::initializeWidgets (const CSMSettings::SettingMap &settings)
{
    //iterate each item in each blocks in this section
    //validate the corresponding setting against the defined valuelist if any.
    foreach (AbstractBlock *block, mAbstractBlocks)
        block->updateSettings (settings);
}
