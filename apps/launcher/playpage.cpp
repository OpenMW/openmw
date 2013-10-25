#include "playpage.hpp"

#include <QListView>

#ifdef Q_OS_MAC
#include <QPlastiqueStyle>
#endif

Launcher::PlayPage::PlayPage(QWidget *parent) : QWidget(parent)
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

<<<<<<< Updated upstream
void PlayPage::setProfilesComboBoxModel(QAbstractItemModel *model)
=======
void Launcher::PlayPage::setProfilesModel(QAbstractItemModel *model)
>>>>>>> Stashed changes
{
    profilesComboBox->setModel(model);
}

<<<<<<< Updated upstream
void PlayPage::setProfilesComboBoxIndex(int index)
=======
void Launcher::PlayPage::setProfilesIndex(int index)
>>>>>>> Stashed changes
{
    profilesComboBox->setCurrentIndex(index);
}

<<<<<<< Updated upstream
void PlayPage::slotCurrentIndexChanged(int index)
{
    emit profileChanged(index);
}

void PlayPage::slotPlayClicked()
=======
void Launcher::PlayPage::slotPlayClicked()
>>>>>>> Stashed changes
{
    emit playButtonClicked();
}
