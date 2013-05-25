#include "NewMaterialDialog.hpp"
#include "ui_newmaterialdialog.h"

NewMaterialDialog::NewMaterialDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewMaterialDialog)
{
    ui->setupUi(this);
}

NewMaterialDialog::~NewMaterialDialog()
{
    delete ui;
}
