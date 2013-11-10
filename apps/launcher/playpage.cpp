#include "playpage.hpp"

#include <QListView>

#ifdef Q_OS_MAC
#include <QPlastiqueStyle>
#endif

Launcher::PlayPage::PlayPage(QWidget *parent) : QWidget(parent)
{
    setObjectName ("PlayPage");
    setupUi(this);

    // Hacks to get the stylesheet look properly
#ifdef Q_OS_MAC
    QPlastiqueStyle *style = new QPlastiqueStyle;
    profilesComboBox->setStyle(style);
#endif
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
