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

CsSettings::AbstractPage::AbstractPage(QWidget *parent):
    QWidget(parent)
{
}

CsSettings::AbstractPage::AbstractPage(const QString &pageName, QWidget *parent):
    QWidget(parent)
{
    QWidget::setObjectName (pageName);
}

CsSettings::AbstractPage::~AbstractPage()
{
}

CsSettings::SettingList *CsSettings::AbstractPage::getSettings()
{
    SettingList *settings = new SettingList();

    foreach (AbstractBlock *block, mAbstractBlocks)
    {
        SettingList *groupSettings = block->getSettings();
        settings->append (*groupSettings);
    }

    return settings;
}
