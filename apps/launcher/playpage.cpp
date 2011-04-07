#include <QtGui>

#include "playpage.hpp"

PlayPage::PlayPage(QWidget *parent) : QWidget(parent)
{
    QPushButton *playButton = new QPushButton(tr("Play"));
    playButton->setMinimumSize(QSize(150, 50));
    
    // TEST
    mProfileModel = new QStringListModel();
    QStringList profileList;
    profileList << "Other" << "Bla" << "No" << "SEGFAULT!";
    mProfileModel->setStringList(profileList);
    
    mProfileComboBox = new QComboBox(this);
    //mProfileComboBox->setMinimumWidth(200);
    mProfileComboBox->setModel(mProfileModel);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QSpacerItem *hSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QSpacerItem *hSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    buttonLayout->addItem(hSpacer1);
    buttonLayout->addWidget(playButton);
    buttonLayout->addItem(hSpacer2);
    
    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    
    pageLayout->addLayout(buttonLayout);
    pageLayout->addWidget(mProfileComboBox);
    
}