#include "playpage.hpp"

#include <QListView>

#ifdef Q_OS_MAC
#include <QPlastiqueStyle>
#endif

PlayPage::PlayPage(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    // Hacks to get the stylesheet look properly
#ifdef Q_OS_MAC
    QPlastiqueStyle *style = new QPlastiqueStyle;
    profilesComboBox->setStyle(style);
#endif
    profilesComboBox->setView(new QListView());

    connect(profilesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentIndexChanged(int)));
    connect(playButton, SIGNAL(clicked()), this, SLOT(slotPlayClicked()));

}

void PlayPage::setProfilesComboBoxModel(QAbstractItemModel *model)
{
    profilesComboBox->setModel(model);
}

void PlayPage::setProfilesComboBoxIndex(int index)
{
    profilesComboBox->setCurrentIndex(index);
}

void PlayPage::slotCurrentIndexChanged(int index)
{
    emit profileChanged(index);
}

void PlayPage::slotPlayClicked()
{
    emit playButtonClicked();
}
