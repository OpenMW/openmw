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

CSVSettings::BlankPage::BlankPage(QWidget *parent):
    AbstractPage("Blank", parent), mLayout(0)
{
    setupUi();
}

CSVSettings::BlankPage::BlankPage(const QString &title, QWidget *parent):
    AbstractPage(title, parent), mLayout(0)
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
    mLayout = new QVBoxLayout(pageBox);
    pageBox->setLayout(mLayout);
}
