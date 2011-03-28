#include <QtGui>

#include "maindialog.h"
#include "datafilesdialog.h"
#include "settingsdialog.h"

MainDialog::MainDialog()
{
      
    QLabel *header = new QLabel(this);  
    header->setMinimumSize(QSize(400, 150));
    header->setPixmap(QPixmap(":resources/openmw_header.png"));

    // Buttons
    QPushButton *buttonStart = new QPushButton(this);
    buttonStart->setMinimumSize(QSize(200, 40));
    buttonStart->setText(tr("Start OpenMW"));
    
    QPushButton *buttonDataFiles = new QPushButton(this);
    buttonDataFiles->setMinimumSize(QSize(200, 40));
    buttonDataFiles->setText(tr("Data Files"));

    QPushButton *buttonSettings = new QPushButton(this);
    buttonSettings->setText(tr("Settings"));
    buttonSettings->setIcon(QIcon::fromTheme("preferences-other"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    buttonBox->addButton(buttonSettings, QDialogButtonBox::ActionRole);
        
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    QVBoxLayout *buttonsLayout = new QVBoxLayout();
    QHBoxLayout *bodyLayout = new QHBoxLayout();
    QHBoxLayout *dialogButtonsLayout = new QHBoxLayout();
    
    QSpacerItem *hSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QSpacerItem *hSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QSpacerItem *hSpacer3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    
    QSpacerItem *vSpacer1 = new QSpacerItem(40, 150, QSizePolicy::Minimum, QSizePolicy::Fixed);
    QSpacerItem *vSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    
    buttonsLayout->addItem(vSpacer1);
    buttonsLayout->addWidget(buttonStart);
    buttonsLayout->addWidget(buttonDataFiles);
    buttonsLayout->addItem(vSpacer2);
    
    bodyLayout->addItem(hSpacer1);
    bodyLayout->addLayout(buttonsLayout);
    bodyLayout->addItem(hSpacer2);
    
    dialogButtonsLayout->addItem(hSpacer3);
    dialogButtonsLayout->addWidget(buttonBox);
    
    dialogLayout->addLayout(bodyLayout);
    dialogLayout->addLayout(dialogButtonsLayout);
    
    setMinimumSize(QSize(400, 310));
    setMaximumSize(QSize(400, 310));

    setWindowTitle(tr("OpenMW Launcher"));
    QPixmap pixmap(":resources/openmw_icon.png");
    setWindowIcon(QIcon(pixmap));
    
    // Signals and slots
    connect(buttonStart, SIGNAL(clicked()), this, SLOT(start()));
    connect(buttonDataFiles, SIGNAL(clicked()), this, SLOT(showDataFiles()));
    connect(buttonSettings, SIGNAL(clicked()), this, SLOT(showSettings()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    
    openmw = new QProcess(NULL);

}

void MainDialog::start()
{
    // Start the game!
    openmw->start("./openmw");
    connect(openmw, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));  
}

void MainDialog::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString debuginfo = openmw->readAllStandardOutput();
       
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        // Close the launcher if the game is quitted
        close();
    } 
    
    if (exitCode != 0) {
        // An error occured whilst starting OpenMW

        // First check if readAllStandardOutput is empty
        // Finished gets signaled twice sometimes
        
        if (!debuginfo.isEmpty())
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Error");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("\nAn error occured while starting OpenMW."));
            msgBox.setDetailedText(debuginfo);
            msgBox.exec();   
        }
       
    }
}

void MainDialog::showDataFiles()
{
    DataFilesDialog dialog;
    dialog.exec();
}

void MainDialog::showSettings()
{
    SettingsDialog dialog;
    dialog.exec();
}
