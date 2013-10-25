#include "textinputdialog.hpp"

#include <QDialogButtonBox>
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QValidator>
#include <QLabel>

<<<<<<< Updated upstream
#include <components/fileorderlist/utils/lineedit.hpp>

TextInputDialog::TextInputDialog(const QString& title, const QString &text, QWidget *parent) :
=======
Launcher::TextInputDialog::TextInputDialog(const QString& title, const QString &text, QWidget *parent) :
>>>>>>> Stashed changes
    QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    mButtonBox = new QDialogButtonBox(this);
    mButtonBox->addButton(QDialogButtonBox::Ok);
    mButtonBox->addButton(QDialogButtonBox::Cancel);

    // Line edit
    QValidator *validator = new QRegExpValidator(QRegExp("^[a-zA-Z0-9_]*$"), this); // Alpha-numeric + underscore
    mLineEdit = new LineEdit(this);
    mLineEdit->setValidator(validator);
    mLineEdit->setCompleter(0);

    QLabel *label = new QLabel(this);
    label->setText(text); 

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

    setOkButtonEnabled(false);
    setModal(true);

    connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

}

int Launcher::TextInputDialog::exec()
{
    mLineEdit->clear();
    mLineEdit->setFocus();
    return QDialog::exec();
}

<<<<<<< Updated upstream
void TextInputDialog::setOkButtonEnabled(bool enabled)
=======
QString Launcher::TextInputDialog::getText() const
>>>>>>> Stashed changes
{
    QPushButton *okButton = mButtonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(enabled);

<<<<<<< Updated upstream
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::Text,Qt::red);
=======
void Launcher::TextInputDialog::slotUpdateOkButton(QString text)
{
    bool enabled = !(text.isEmpty());
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
>>>>>>> Stashed changes

    if (enabled) {
        mLineEdit->setPalette(QApplication::palette());
    } else {
        // Existing profile name, make the text red
        mLineEdit->setPalette(*palette);
    }
<<<<<<< Updated upstream
=======
}

Launcher::TextInputDialog::DialogLineEdit::DialogLineEdit (QWidget *parent) :
    LineEdit (parent)
{
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

    setObjectName(QString("LineEdit"));
    setStyleSheet(QString("LineEdit { padding-right: %1px; } ").arg(mClearButton->sizeHint().width() + frameWidth + 1));
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), mClearButton->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(msz.height(), mClearButton->sizeHint().height() + frameWidth * 2 + 2));
>>>>>>> Stashed changes

}
