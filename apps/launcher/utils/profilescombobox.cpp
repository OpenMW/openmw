#include <QApplication>
#include <QString>

#include "profilescombobox.hpp"

ProfilesComboBox::ProfilesComboBox(QWidget* parent)
    : ContentSelectorView::ComboBox(parent)
{
    connect(this, qOverload<int>(&ProfilesComboBox::activated), this, &ProfilesComboBox::slotIndexChangedByUser);

    setInsertPolicy(QComboBox::NoInsert);
}

void ProfilesComboBox::setEditEnabled(bool editable)
{
    if (isEditable() == editable)
        return;

    if (!editable)
    {
        disconnect(lineEdit(), &QLineEdit::editingFinished, this, &ProfilesComboBox::slotEditingFinished);
        disconnect(lineEdit(), &QLineEdit::textChanged, this, &ProfilesComboBox::slotTextChanged);
        return setEditable(false);
    }

    // Reset the completer and validator
    setEditable(true);
    setValidator(mValidator);

    auto* edit = new ComboBoxLineEdit(this);

    setLineEdit(edit);
    setCompleter(nullptr);

    connect(lineEdit(), &QLineEdit::editingFinished, this, &ProfilesComboBox::slotEditingFinished);

    connect(lineEdit(), &QLineEdit::textChanged, this, &ProfilesComboBox::slotTextChanged);

    connect(lineEdit(), &QLineEdit::textChanged, this, &ProfilesComboBox::signalProfileTextChanged);
}

void ProfilesComboBox::slotTextChanged(const QString& text)
{
    QPalette palette;
    palette.setColor(QPalette::Text, Qt::red);

    int index = findText(text);

    if (text.isEmpty() || (index != -1 && index != currentIndex()))
    {
        lineEdit()->setPalette(palette);
    }
    else
    {
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
    emit profileRenamed(previous, current);
}

void ProfilesComboBox::slotIndexChangedByUser(int index)
{
    if (index == -1)
        return;

    emit signalProfileChanged(mOldProfile, currentText());
    mOldProfile = currentText();
}

ProfilesComboBox::ComboBoxLineEdit::ComboBoxLineEdit(QWidget* parent)
    : LineEdit(parent)
{
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

    setObjectName(QString("ComboBoxLineEdit"));
    setStyleSheet(QString("ComboBoxLineEdit { background-color: transparent; padding-right: %1px; } ")
                      .arg(mClearButton->sizeHint().width() + frameWidth + 1));
}
