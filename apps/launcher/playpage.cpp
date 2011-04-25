#include <QtGui>

#include "playpage.hpp"

PlayPage::PlayPage(QWidget *parent) : QWidget(parent)
{
    // TODO: Should be an install path
    QFile file("apps/launcher/resources/launcher.qss");

    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    QGroupBox *playBox = new QGroupBox(this);
    playBox->setObjectName("PlayBox");
    playBox->setFixedSize(QSize(425, 375));
    playBox->setFlat(true);

    QVBoxLayout *playLayout = new QVBoxLayout(playBox);

    QPushButton *playButton = new QPushButton(tr("Play"), playBox);
    playButton->setObjectName("PlayButton");
    //playButton->setMinimumSize(QSize(150, 50));

    QLabel *profileLabel = new QLabel(tr("Current Profile:"), playBox);
    profileLabel->setObjectName("ProfileLabel");

    // TODO: Cleanup
    mProfilesModel = new QStringListModel();

    mProfilesComboBox = new QComboBox(playBox);
    mProfilesComboBox->setObjectName("ProfileComboBox");
    //mProfileComboBox->setMinimumWidth(200);
    mProfilesComboBox->setModel(mProfilesModel);


    QSpacerItem *vSpacer1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QSpacerItem *vSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    playLayout->addItem(vSpacer1);
    playLayout->addWidget(playButton);
    playLayout->addWidget(profileLabel);
    playLayout->addWidget(mProfilesComboBox);
    playLayout->addItem(vSpacer2);

    QHBoxLayout *pageLayout = new QHBoxLayout(this);
    QSpacerItem *hSpacer1 = new QSpacerItem(54, 90, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QSpacerItem *hSpacer2 = new QSpacerItem(54, 90, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pageLayout->addItem(hSpacer1);
    pageLayout->addWidget(playBox);
    pageLayout->addItem(hSpacer2);

}