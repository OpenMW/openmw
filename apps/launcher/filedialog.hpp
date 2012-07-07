#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QFileDialog>

class QPushButton;

class FileDialog : public QFileDialog
{
    Q_OBJECT

public:
    FileDialog(QWidget *parent = 0);

    static QString getExistingDirectory(QWidget *parent = 0,
                                            const QString &caption = QString(),
                                            const QString &dir = QString(),
                                            Options options = ShowDirsOnly);

private slots:
    void updateChooseButton(const QString &directory);

private:
    QPushButton *mChooseButton;
};


#endif // FILEDIALOG_HPP
