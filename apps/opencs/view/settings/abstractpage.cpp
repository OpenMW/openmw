#include "abstractpage.hpp"

#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMargins>

CSVSettings::AbstractPage::AbstractPage(QWidget *parent):
    QWidget(parent)
{
}

CSVSettings::AbstractPage::AbstractPage(const QString &pageName, QWidget *parent):
    QWidget(parent)
{
    QWidget::setObjectName (pageName);
}

CSVSettings::AbstractPage::~AbstractPage()
{
}

CSMSettings::SettingList *CSVSettings::AbstractPage::getSettings()
{
    CSMSettings::SettingList *settings = new CSMSettings::SettingList();

    foreach (AbstractBlock *block, mAbstractBlocks)
    {
        CSMSettings::SettingList *groupSettings = block->getSettings();
        settings->append (*groupSettings);
    }

    return settings;
}
