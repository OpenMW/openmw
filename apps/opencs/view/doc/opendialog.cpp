#include <QVBoxLayout>
#include <QDialogButtonBox>

#include <components/fileorderlist/datafileslist.hpp>

#include "opendialog.hpp"

OpenDialog::OpenDialog(QWidget * parent) : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    mFileSelector = new DataFilesList(mCfgMgr, this);
    layout->addWidget(mFileSelector);
    mFileSelector->setupDataFiles();
    
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox);
    
    setLayout(layout);
    setWindowTitle(tr("Open"));
}

void OpenDialog::getFileList(std::vector<boost::filesystem::path>& paths)
{
    mFileSelector->getSelectedFiles(paths);
}
