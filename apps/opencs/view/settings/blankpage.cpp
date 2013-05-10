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

CsSettings::BlankPage::BlankPage(QWidget *parent):
    AbstractPage("Blank", parent)
{
    initPage();
}

CsSettings::BlankPage::BlankPage(const QString &title, QWidget *parent):
    AbstractPage(title, parent)
{
    initPage();
}

void CsSettings::BlankPage::initPage()
{
    // Hacks to get the stylesheet look properly
#ifdef Q_OS_MAC
    QPlastiqueStyle *style = new QPlastiqueStyle;
    //profilesComboBox->setStyle(style);
#endif

    setupUi();
}

void CsSettings::BlankPage::setupUi()
{
    QGroupBox *pageBox = new QGroupBox(this);
    QLayout* pageLayout = new QVBoxLayout();

    setLayout(pageLayout);
    pageLayout->addWidget(pageBox);
}

void CsSettings::BlankPage::initializeWidgets (const SettingMap &settings)
{
    //iterate each item in each blocks in this section
    //validate the corresponding setting against the defined valuelist if any.
    foreach (AbstractBlock *block, mAbstractBlocks)
        block->updateSettings (settings);
}
