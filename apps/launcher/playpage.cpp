#include "playpage.hpp"

#include <QtGui>

PlayPage::PlayPage(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    // Hacks to get the stylesheet look properly on different platforms
    QPlastiqueStyle *style = new QPlastiqueStyle;
    QFont font = QApplication::font();
    font.setPointSize(12); // Fixes problem with overlapping items

    profilesComboBox->setStyle(style);
    profilesComboBox->setFont(font);

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
