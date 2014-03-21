#include <QVBoxLayout>
#include <QDialogButtonBox>

#include <components/fileorderlist/datafileslist.hpp>

#include "opendialog.hpp"

OpenDialog::OpenDialog(QWidget * parent) : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    mFileSelector = new DataFilesList(mCfgMgr, this);
    layout->addWidget(mFileSelector);

    /// \todo move config to Editor class and add command line options.
    // We use the Configuration Manager to retrieve the configuration values
    boost::program_options::variables_map variables;
    boost::program_options::options_description desc;

    desc.add_options()
    ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken())
    ("data-local", boost::program_options::value<std::string>()->default_value(""))
    ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)->default_value(false))
    ("encoding", boost::program_options::value<std::string>()->default_value("win1252"));

    boost::program_options::notify(variables);

    mCfgMgr.readConfiguration(variables, desc);

    Files::PathContainer mDataDirs, mDataLocal;
    if (!variables["data"].empty()) {
        mDataDirs = Files::PathContainer(variables["data"].as<Files::PathContainer>());
    }

    std::string local = variables["data-local"].as<std::string>();
    if (!local.empty()) {
        mDataLocal.push_back(Files::PathContainer::value_type(local));
    }

    mCfgMgr.processPaths(mDataDirs);
    mCfgMgr.processPaths(mDataLocal);

    // Set the charset for reading the esm/esp files
    QString encoding = QString::fromUtf8 (variables["encoding"].as<std::string>().c_str());

    Files::PathContainer dataDirs;
    dataDirs.insert(dataDirs.end(), mDataDirs.begin(), mDataDirs.end());
    dataDirs.insert(dataDirs.end(), mDataLocal.begin(), mDataLocal.end());
    mFileSelector->setupDataFiles(dataDirs, encoding);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox);

    setLayout(layout);
    setWindowTitle(tr("Open"));
}

void OpenDialog::getFileList(std::vector<boost::filesystem::path>& paths)
{
    mFileSelector->selectedFiles(paths);
}
