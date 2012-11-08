#ifndef PROFILESCOMBOBOX_HPP
#define PROFILESCOMBOBOX_HPP

#include <QComboBox>

class QString;

class QRegExpValidator;

class ProfilesComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit ProfilesComboBox(QWidget *parent = 0);
    void setEditEnabled(bool editable);
    
signals:
    void profileChanged(const QString &previous, const QString &current);
    void profileRenamed(const QString &oldName, const QString &newName);
    
private slots:
    void slotReturnPressed();
    void slotIndexChanged(int index);

private:
    QString mOldProfile;
    QRegExpValidator *mValidator;
};

#endif // PROFILESCOMBOBOX_HPP
