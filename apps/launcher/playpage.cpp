#include <QtGui>

#include "playpage.hpp"

PlayPage::PlayPage(QWidget *parent) : QWidget(parent)
{
    // Load the stylesheet
    QFile file("resources/launcher.qss");

    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    QGroupBox *playBox = new QGroupBox(this);
    playBox->setFixedSize(QSize(425, 375));
    playBox->setFlat(true);

    QPushButton *playButton = new QPushButton(tr("Play"), playBox);
    playButton->setMinimumSize(QSize(200, 50));

    QLabel *profileLabel = new QLabel(tr("Current Profile:"), playBox);

    mProfilesModel = new QStringListModel();
    mProfilesComboBox = new QComboBox(playBox);
    mProfilesComboBox->setModel(mProfilesModel);

    QGridLayout *playLayout = new QGridLayout(playBox);

    QSpacerItem *hSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QSpacerItem *hSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    QSpacerItem *vSpacer1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QSpacerItem *vSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    playLayout->addWidget(playButton, 1, 1, 1, 1);
    playLayout->addWidget(profileLabel, 2, 1, 1, 1);
    playLayout->addWidget(mProfilesComboBox, 3, 1, 1, 1);
    playLayout->addItem(hSpacer1, 2, 0, 1, 1);
    playLayout->addItem(hSpacer2, 2, 2, 1, 1);
    playLayout->addItem(vSpacer1, 0, 1, 1, 1);
    playLayout->addItem(vSpacer2, 4, 1, 1, 1);

    QHBoxLayout *pageLayout = new QHBoxLayout(this);

    pageLayout->addWidget(playBox);

}