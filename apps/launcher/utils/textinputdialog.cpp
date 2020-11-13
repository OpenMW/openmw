#include "textinputdialog.hpp"

#include <QDialogButtonBox>
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QValidator>
#include <QLabel>

Launcher::TextInputDialog::TextInputDialog(const QString& title, const QString &text, QWidget *parent) :
    QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    mButtonBox = new QDialogButtonBox(this);
    mButtonBox->addButton(QDialogButtonBox::Ok);
    mButtonBox->addButton(QDialogButtonBox::Cancel);
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled (false);

    QLabel *label = new QLabel(this);
    label->setText(text);

    // Line edit
    QValidator *validator = new QRegExpValidator(QRegExp("^[a-zA-Z0-9_]*$"), this); // Alpha-numeric + underscore
    mLineEdit = new LineEdit(this);
    mLineEdit->setValidator(validator);
    mLineEdit->setCompleter(nullptr);

    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(label);
    dialogLayout->addWidget(mLineEdit);
    dialogLayout->addWidget(mButtonBox);

    // Messageboxes on mac have no title
#ifndef Q_OS_MAC
    setWindowTitle(title);
#else
    Q_UNUSED(title);
#endif

    setModal(true);

    connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

Launcher::TextInputDialog::~TextInputDialog()
{
}

int Launcher::TextInputDialog::exec()
{
    mLineEdit->clear();
    mLineEdit->setFocus();
    return QDialog::exec();
}

void Launcher::TextInputDialog::setOkButtonEnabled(bool enabled)
{
    QPushButton *okButton = mButtonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(enabled);

    QPalette palette;
    palette.setColor(QPalette::Text, Qt::red);

    if (enabled) {
        mLineEdit->setPalette(QApplication::palette());
    } else {
        // Existing profile name, make the text red
        mLineEdit->setPalette(palette);
    }
}
