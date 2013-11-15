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

    // Line edit
    QValidator *validator = new QRegExpValidator(QRegExp("^[a-zA-Z0-9_]*$"), this); // Alpha-numeric + underscore
    mLineEdit = new DialogLineEdit(this);
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

    setModal(true);

    connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(mLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotUpdateOkButton(QString)));

}

int Launcher::TextInputDialog::exec()
{
    mLineEdit->clear();
    mLineEdit->setFocus();
    return QDialog::exec();
}

QString Launcher::TextInputDialog::getText() const
{
    return mLineEdit->text();
}

void Launcher::TextInputDialog::slotUpdateOkButton(QString text)
{
    bool enabled = !(text.isEmpty());
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);

    if (enabled)
        mLineEdit->setPalette(QApplication::palette());
    else
    {
        // Existing profile name, make the text red
        QPalette *palette = new QPalette();
        palette->setColor(QPalette::Text,Qt::red);
        mLineEdit->setPalette(*palette);
    }
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

}
