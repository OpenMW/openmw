#ifndef PROFILESCOMBOBOX_HPP
#define PROFILESCOMBOBOX_HPP

#include "components/contentselector/view/combobox.hpp"
#include "lineedit.hpp"

#include <QDebug>

class QString;

class ProfilesComboBox : public ContentSelectorView::ComboBox
{
    Q_OBJECT

public:
    class ComboBoxLineEdit : public LineEdit
    {
    public:
        explicit ComboBoxLineEdit (QWidget *parent = nullptr);
    };

public:

    explicit ProfilesComboBox(QWidget *parent = nullptr);
    void setEditEnabled(bool editable);
    void setCurrentProfile(int index)
    {
        ComboBox::setCurrentIndex(index);
        mOldProfile = currentText();
    }

signals:
    void signalProfileTextChanged(const QString &item);
    void signalProfileChanged(const QString &previous, const QString &current);
    void signalProfileChanged(int index);
    void profileRenamed(const QString &oldName, const QString &newName);

private slots:

    void slotEditingFinished();
    void slotIndexChangedByUser(int index);
    void slotTextChanged(const QString &text);

private:
    QString mOldProfile;
};
#endif // PROFILESCOMBOBOX_HPP
