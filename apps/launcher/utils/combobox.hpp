#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QComboBox>

class ComboBox : public QComboBox
{
    Q_OBJECT
private:
    QString oldText;
public:
    ComboBox(QWidget *parent=0) : QComboBox(parent), oldText()
    {
        connect(this,SIGNAL(editTextChanged(const QString&)), this,
                SLOT(textChangedSlot(const QString&)));
        connect(this,SIGNAL(currentIndexChanged(const QString&)), this,
                SLOT(textChangedSlot(const QString&)));
    }
private slots:
    void textChangedSlot(const QString &newText)
    {
        emit textChanged(oldText, newText);
        oldText = newText;
    }
signals:
    void textChanged(const QString &oldText, const QString &newText);
};
#endif