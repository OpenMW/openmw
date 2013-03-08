#include <qdialog.h>
#include <components/files/configurationmanager.hpp>

class DataFilesList;
class QDialogButtonBox;

class OpenDialog : public QDialog {
    Q_OBJECT
public:
    OpenDialog(QWidget * parent = 0);
    
    void getFileList(std::vector<boost::filesystem::path>& paths);
private:
    DataFilesList * mFileSelector;
    QDialogButtonBox * buttonBox;
    Files::ConfigurationManager mCfgMgr;
};