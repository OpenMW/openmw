#include "optiongroup.hpp"

#include <QRadioButton>


// Code for option group borrowed and adapted from
// https://qt-project.org/forums/viewthread/25760

int OptionGroup::currentSelection() const
{    return mCurrentSelection; }

void OptionGroup::setCurrentSelection(int selection)
{
    // If the specified selection id is not in our button map,
    // then it is invalid, set selection to -1.  Otherwise,
    // update the selection to user specified value

    QMap<int, QRadioButton *>::Iterator iter;

    iter = mButtonMap.find(selection);

    if (iter == mButtonMap.end() || selection < 0)
    {
        mCurrentSelection = -1;

        for (iter = mButtonMap.begin(); iter != mButtonMap.end(); ++iter)
            iter.value()->setChecked(false);

    }
    else
    {
        iter.value()->setChecked(true);
        mCurrentSelection = selection;
    }
}

void OptionGroup::setSelectionId(QRadioButton* button, int id)
{
    // Make sure we got a valid Id (non-negative)
    // Also then listen for signals from this button
    if (id >= 0)
    {
        mButtonMap[id] = button;
        mRevButtonMap[button] = id;

        connect(button, SIGNAL(toggled(bool)), this, SLOT(buttonToggled(bool)));
    }
}

void OptionGroup::buttonToggled(bool checked)
{
    if (checked == true)
    {
        QRadioButton* btn = qobject_cast<QRadioButton*>(sender());
        Q_ASSERT(btn);
        mCurrentSelection = mRevButtonMap[btn];
        emit selectionChanged(mCurrentSelection);
    }
}
