#include "textinputdialog.hpp"

#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QValidator>

Launcher::TextInputDialog::TextInputDialog(const QString& title, const QString& text, QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    mButtonBox = new QDialogButtonBox(this);
    mButtonBox->addButton(QDialogButtonBox::Ok);
    mButtonBox->addButton(QDialogButtonBox::Cancel);
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    auto* label = new QLabel(this);
    label->setText(text);

    // Line edit
    QValidator* validator = new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9_]*$"), this);
    mLineEdit = new LineEdit(this);
    mLineEdit->setValidator(validator);
    mLineEdit->setCompleter(nullptr);

    auto* dialogLayout = new QVBoxLayout(this);
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

    connect(mButtonBox, &QDialogButtonBox::accepted, this, &TextInputDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &TextInputDialog::reject);
}

int Launcher::TextInputDialog::exec()
{
    mLineEdit->clear();
    mLineEdit->setFocus();
    return QDialog::exec();
}

void Launcher::TextInputDialog::setOkButtonEnabled(bool enabled)
{
    QPushButton* okButton = mButtonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(enabled);

    QPalette palette;
    palette.setColor(QPalette::Text, Qt::red);

    if (enabled)
    {
        mLineEdit->setPalette(QApplication::palette());
    }
    else
    {
        // Existing profile name, make the text red
        mLineEdit->setPalette(palette);
    }
}
