#include <QRegExpValidator>
#include <QString>
#include <QApplication>
#include <QKeyEvent>

#include "profilescombobox.hpp"

ProfilesComboBox::ProfilesComboBox(QWidget *parent) :
    ContentSelectorView::ComboBox(parent)
{
    connect(this, SIGNAL(activated(int)), this,
            SLOT(slotIndexChangedByUser(int)));

    setInsertPolicy(QComboBox::NoInsert);
}

void ProfilesComboBox::setEditEnabled(bool editable)
{
    if (isEditable() == editable)
        return;

    if (!editable) {
        disconnect(lineEdit(), SIGNAL(editingFinished()), this, SLOT(slotEditingFinished()));
        disconnect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(slotTextChanged(QString)));
        return setEditable(false);
    }

    // Reset the completer and validator
    setEditable(true);
    setValidator(mValidator);

    ComboBoxLineEdit *edit = new ComboBoxLineEdit(this);

    setLineEdit(edit);
    setCompleter(nullptr);

    connect(lineEdit(), SIGNAL(editingFinished()), this,
                SLOT(slotEditingFinished()));

    connect(lineEdit(), SIGNAL(textChanged(QString)), this,
            SLOT(slotTextChanged(QString)));

    connect (lineEdit(), SIGNAL(textChanged(QString)), this,
             SIGNAL (signalProfileTextChanged (QString)));
}

void ProfilesComboBox::slotTextChanged(const QString &text)
{
    QPalette palette;
    palette.setColor(QPalette::Text,Qt::red);

    int index = findText(text);

    if (text.isEmpty() || (index != -1 && index != currentIndex())) {
        lineEdit()->setPalette(palette);
    } else {
        lineEdit()->setPalette(QApplication::palette());
    }
}

void ProfilesComboBox::slotEditingFinished()
{
    QString current = currentText();
    QString previous = itemText(currentIndex());

    if (currentIndex() == -1)
        return;

    if (current.isEmpty())
        return;

    if (current == previous)
        return;

    if (findText(current) != -1)
        return;

    setItemText(currentIndex(), current);
    emit(profileRenamed(previous, current));
}

void ProfilesComboBox::slotIndexChangedByUser(int index)
{
    if (index == -1)
        return;

    emit (signalProfileChanged(mOldProfile, currentText()));
    mOldProfile = currentText();
}

ProfilesComboBox::ComboBoxLineEdit::ComboBoxLineEdit (QWidget *parent)
    : LineEdit (parent)
{
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

    setObjectName(QString("ComboBoxLineEdit"));
    setStyleSheet(QString("ComboBoxLineEdit { background-color: transparent; padding-right: %1px; } ").arg(mClearButton->sizeHint().width() + frameWidth + 1));
}
