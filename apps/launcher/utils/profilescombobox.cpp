#include <QRegExpValidator>
#include <QLineEdit>
#include <QString>

#include "profilescombobox.hpp"

ProfilesComboBox::ProfilesComboBox(QWidget *parent) :
    QComboBox(parent)
{
    mValidator = new QRegExpValidator(QRegExp("^[a-zA-Z0-9_]*$"), this); // Alpha-numeric + underscore

    setEditable(true);
    setValidator(mValidator);
    setCompleter(0);

    connect(this, SIGNAL(currentIndexChanged(int)), this,
            SLOT(slotIndexChanged(int)));
    connect(lineEdit(), SIGNAL(returnPressed()), this,
                SLOT(slotReturnPressed()));
}

void ProfilesComboBox::setEditEnabled(bool editable)
{
    if (!editable)
        return setEditable(false);

    // Reset the completer and validator
    setEditable(true);
    setValidator(mValidator);
    setCompleter(0);
}

void ProfilesComboBox::slotReturnPressed()
{
    QString current = currentText();
    QString previous = itemText(currentIndex());

    if (findText(current) != -1)
        return;

    setItemText(currentIndex(), current);
    emit(profileRenamed(previous, current));
}

void ProfilesComboBox::slotIndexChanged(int index)
{
    if (index == -1)
        return;

    emit(profileChanged(mOldProfile, currentText()));
    mOldProfile = itemText(index);
}
