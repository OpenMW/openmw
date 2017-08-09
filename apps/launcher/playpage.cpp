#include "playpage.hpp"

#include <QListView>

Launcher::PlayPage::PlayPage(QWidget *parent) : QWidget(parent)
{
    setObjectName ("PlayPage");
    setupUi(this);

    profilesComboBox->setView(new QListView());

    connect(profilesComboBox, SIGNAL(activated(int)), this, SIGNAL (signalProfileChanged(int)));
    connect(playButton, SIGNAL(clicked()), this, SLOT(slotPlayClicked()));

}

void Launcher::PlayPage::setProfilesModel(QAbstractItemModel *model)
{
    profilesComboBox->setModel(model);
}

void Launcher::PlayPage::setProfilesIndex(int index)
{
    profilesComboBox->setCurrentIndex(index);
}

void Launcher::PlayPage::slotPlayClicked()
{
    emit playButtonClicked();
}
