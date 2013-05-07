#ifndef NEWMATERIALDIALOG_HPP
#define NEWMATERIALDIALOG_HPP

#include <QDialog>

namespace Ui {
class NewMaterialDialog;
}

class NewMaterialDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit NewMaterialDialog(QWidget *parent = 0);
    ~NewMaterialDialog();
    
private:
    Ui::NewMaterialDialog *ui;
};

#endif // NEWMATERIALDIALOG_HPP
