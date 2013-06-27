#include "AddPropertyDialog.hpp"
#include "ui_addpropertydialog.h"

AddPropertyDialog::AddPropertyDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::AddPropertyDialog)
	, mType(0)
{
    ui->setupUi(this);

	connect(ui->buttonBox, SIGNAL(accepted()),
			this, SLOT(accepted()));
	connect(ui->buttonBox, SIGNAL(rejected()),
			this, SLOT(rejected()));
}

void AddPropertyDialog::accepted()
{
	mName = ui->lineEdit->text();
	mType = ui->comboBox->currentIndex();
}

void AddPropertyDialog::rejected()
{
	mName = "";
}

AddPropertyDialog::~AddPropertyDialog()
{
    delete ui;
}
