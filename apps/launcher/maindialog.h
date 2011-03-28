#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include <QProcess>

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    MainDialog();

    QProcess *openmw;
    
public slots:

    void start();
    void showDataFiles();
    void showSettings();
    void finished(int, QProcess::ExitStatus exitStatus);

};

#endif
