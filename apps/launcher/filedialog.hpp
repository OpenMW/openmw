#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QFileDialog>

class QPushButton;

struct FileDialogArgs
{
    FileDialogArgs() : parent(0), mode(QFileDialog::AnyFile) {}
    QWidget *parent;
    QString caption;
    QString directory;
    QString selection;
    QString filter;
    QFileDialog::FileMode mode;
    QFileDialog::Options options;

};

class FileDialog : public QFileDialog
{
    Q_OBJECT

public:
    FileDialog(QWidget *parent = 0);
//    FileDialog(QWidget *parent, Qt::WindowFlags f);

    //QString getExistingDirectory();
    static QString getExistingDirectory(QWidget *parent = 0,
                                            const QString &caption = QString(),
                                            const QString &dir = QString(),
                                            Options options = ShowDirsOnly);
    //FileDialog mDirectoryDialog;

    bool initialized;
protected:

private slots:
//    void updateOkButton(const QString &text);
    void updateChooseButton(const QString &directory);
    //void

private:
    QPushButton *mChooseButton;
    //QFileDialog *mDirectoryDialog;
};


#endif // FILEDIALOG_HPP
