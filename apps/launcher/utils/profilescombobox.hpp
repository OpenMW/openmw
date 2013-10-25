#ifndef PROFILESCOMBOBOX_HPP
#define PROFILESCOMBOBOX_HPP

#include "components/contentselector/view/combobox.hpp"
#include "lineedit.hpp"

class QString;

class ProfilesComboBox : public ContentSelectorView::ComboBox
{
    Q_OBJECT

public:
    class ComboBoxLineEdit : public LineEdit
    {
    public:
        explicit ComboBoxLineEdit (QWidget *parent = 0);
    };

public:

    explicit ProfilesComboBox(QWidget *parent = 0);
    void setEditEnabled(bool editable);

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
