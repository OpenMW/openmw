#include <QtGui>

#include "playpage.hpp"

PlayPage::PlayPage(QWidget *parent) : QWidget(parent)
{
    QWidget *playWidget = new QWidget(this);
    playWidget->setObjectName("PlayGroup");
    playWidget->setFixedSize(QSize(425, 375));

    mPlayButton = new QPushButton(tr("Play"), playWidget);
    mPlayButton->setObjectName("PlayButton");
    mPlayButton->setMinimumSize(QSize(200, 50));

    QLabel *profileLabel = new QLabel(tr("Current Profile:"), playWidget);
    profileLabel->setObjectName("ProfileLabel");

    QPlastiqueStyle *style = new QPlastiqueStyle;
    mProfilesComboBox = new QComboBox(playWidget);
    mProfilesComboBox->setObjectName("ProfilesComboBox");
    mProfilesComboBox->setStyle(style);

    QGridLayout *playLayout = new QGridLayout(playWidget);

    QSpacerItem *hSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QSpacerItem *hSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    QSpacerItem *vSpacer1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QSpacerItem *vSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    playLayout->addWidget(mPlayButton, 1, 1, 1, 1);
    playLayout->addWidget(profileLabel, 2, 1, 1, 1);
    playLayout->addWidget(mProfilesComboBox, 3, 1, 1, 1);
    playLayout->addItem(hSpacer1, 2, 0, 1, 1);
    playLayout->addItem(hSpacer2, 2, 2, 1, 1);
    playLayout->addItem(vSpacer1, 0, 1, 1, 1);
    playLayout->addItem(vSpacer2, 4, 1, 1, 1);

    QHBoxLayout *pageLayout = new QHBoxLayout(this);

    pageLayout->addWidget(playWidget);

}