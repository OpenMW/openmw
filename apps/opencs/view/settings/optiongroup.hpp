#ifndef OPTIONGROUP_HPP
#define OPTIONGROUP_HPP

#include <QWidget>
#include <QMap>

class QRadioButton;

class OptionGroup : public QWidget
{
    int mCcurrentSelection;
    QMap<int, QRadioButton*> mButtonMap;
    QMap<QRadioButton*, int> mRevButtonMap;

    Q_OBJECT
    Q_PROPERTY(int currentSelection READ currentSelection WRITE setCurrentSelection USER true)

public:
    explicit OptionGroup(QWidget *parent = 0);

    int currentSelection() const;
    void setCurrentSelection(int selection);

    void setSelectionId(QRadioButton *button, int id);

signals:
    void selectionChanged(int selection);

public slots:
    void buttonToggled(bool checked);
};

#endif // OPTIONGROUP_HPP
