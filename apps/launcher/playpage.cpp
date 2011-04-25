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
    mProfileModel = new QStringListModel();

    mProfileComboBox = new QComboBox(playBox);
    mProfileComboBox->setObjectName("ProfileComboBox");
    //mProfileComboBox->setMinimumWidth(200);
    mProfileComboBox->setModel(mProfileModel);


    QSpacerItem *vSpacer1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QSpacerItem *vSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    playLayout->addItem(vSpacer1);
    playLayout->addWidget(playButton);
    playLayout->addWidget(profileLabel);
    playLayout->addWidget(mProfileComboBox);
    playLayout->addItem(vSpacer2);

    QHBoxLayout *pageLayout = new QHBoxLayout(this);
    QSpacerItem *hSpacer1 = new QSpacerItem(54, 90, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QSpacerItem *hSpacer2 = new QSpacerItem(54, 90, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pageLayout->addItem(hSpacer1);
    pageLayout->addWidget(playBox);
    pageLayout->addItem(hSpacer2);

}
//         verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
//         listWidget = new QListWidget(Dialog);
//         listWidget->setObjectName(QString::fromUtf8("listWidget"));
//         listWidget->setMaximumSize(QSize(16777215, 100));
//
//         verticalLayout_2->addWidget(listWidget);
//
//         groupBox = new QGroupBox(Dialog);
//         groupBox->setObjectName(QString::fromUtf8("groupBox"));
//         gridLayout_2 = new QGridLayout(groupBox);
//         gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
//         verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
//
//         gridLayout_2->addItem(verticalSpacer_2, 0, 1, 1, 1);
//
//         horizontalSpacer_4 = new QSpacerItem(54, 90, QSizePolicy::Expanding, QSizePolicy::Minimum);
//
//         gridLayout_2->addItem(horizontalSpacer_4, 1, 0, 1, 1);
//
//         horizontalSpacer_3 = new QSpacerItem(53, 90, QSizePolicy::Expanding, QSizePolicy::Minimum);
//
//         gridLayout_2->addItem(horizontalSpacer_3, 1, 2, 1, 1);
//
//         verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
//
//         gridLayout_2->addItem(verticalSpacer_3, 2, 1, 1, 1);
//
//         groupBox_2 = new QGroupBox(groupBox);
//         groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
//         groupBox_2->setMinimumSize(QSize(404, 383));
//         groupBox_2->setMaximumSize(QSize(404, 383));
//
//         groupBox_2->setFlat(true);
//         verticalLayout = new QVBoxLayout(groupBox_2);
//         verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
//         gridLayout = new QGridLayout();
//         gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
//         label = new QLabel(groupBox_2);
//         label->setObjectName(QString::fromUtf8("label"));
//         label->setStyleSheet(QString::fromUtf8(""));
//
//         gridLayout->addWidget(label, 2, 1, 1, 1);
//
//         comboBox = new QComboBox(groupBox_2);
//         comboBox->setObjectName(QString::fromUtf8("comboBox"));
//         comboBox->setMinimumSize(QSize(200, 0));
//         comboBox->setStyleSheet(QString::fromUtf8(""));
//
//         gridLayout->addWidget(comboBox, 3, 1, 1, 1);
//
//         horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
//
//         gridLayout->addItem(horizontalSpacer, 2, 2, 1, 1);
//
//         horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
//
//         gridLayout->addItem(horizontalSpacer_2, 2, 0, 1, 1);
//
//         verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Minimum);
//
//         gridLayout->addItem(verticalSpacer, 1, 1, 1, 1);
//
//         verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
//
//         gridLayout->addItem(verticalSpacer_4, 4, 1, 1, 1);
//
//         pushButton = new QPushButton(groupBox_2);
//         pushButton->setObjectName(QString::fromUtf8("pushButton"));
//         pushButton->setMinimumSize(QSize(200, 50));
//         pushButton->setMaximumSize(QSize(16777215, 16777215));
//         pushButton->setAutoFillBackground(false);
//         pushButton->setStyleSheet(QString::fromUtf8(""));
//         pushButton->setIconSize(QSize(32, 32));
//         pushButton->setAutoRepeat(false);
//         pushButton->setFlat(false);
//
//         gridLayout->addWidget(pushButton, 0, 1, 1, 1);
//
//
//         verticalLayout->addLayout(gridLayout);
//
//
//         gridLayout_2->addWidget(groupBox_2, 1, 1, 1, 1);
//
//
//         verticalLayout_2->addWidget(groupBox);
//
//         buttonBox = new QDialogButtonBox(Dialog);
//         buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
//         buttonBox->setOrientation(Qt::Horizontal);
//         buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
//
//         verticalLayout_2->addWidget(buttonBox);
//
//
