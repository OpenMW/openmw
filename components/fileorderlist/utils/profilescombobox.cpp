#include <QRegExpValidator>
#include <QLineEdit>
#include <QString>
#include <QApplication>
#include <QKeyEvent>

#include "profilescombobox.hpp"
#include "comboboxlineedit.hpp"

ProfilesComboBox::ProfilesComboBox(QWidget *parent) :
    QComboBox(parent)
{
    mValidator = new QRegExpValidator(QRegExp("^[a-zA-Z0-9_]*$"), this); // Alpha-numeric + underscore
    setEditEnabled(true);
    setValidator(mValidator);
    setCompleter(0);

    connect(this, SIGNAL(currentIndexChanged(int)), this,
            SLOT(slotIndexChanged(int)));

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
    setCompleter(0);

    connect(lineEdit(), SIGNAL(editingFinished()), this,
                SLOT(slotEditingFinished()));

    connect(lineEdit(), SIGNAL(textChanged(QString)), this,
            SLOT(slotTextChanged(QString)));
}

void ProfilesComboBox::slotTextChanged(const QString &text)
{
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::Text,Qt::red);

    int index = findText(text);

    if (text.isEmpty() || (index != -1 && index != currentIndex())) {
        lineEdit()->setPalette(*palette);
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

void ProfilesComboBox::slotIndexChanged(int index)
{
    if (index == -1)
        return;

    emit(profileChanged(mOldProfile, currentText()));
    mOldProfile = itemText(index);
}
