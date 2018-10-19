#include "extendedcommandconfigurator.hpp"

#include <algorithm>

#include <QPushButton>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QLayout>

#include "../../model/doc/document.hpp"

#include "../../model/world/commanddispatcher.hpp"
#include "../../model/world/data.hpp"

CSVWorld::ExtendedCommandConfigurator::ExtendedCommandConfigurator(CSMDoc::Document &document,
                                                                   const CSMWorld::UniversalId &id,
                                                                   QWidget *parent)
    : QWidget(parent),
      mNumUsedCheckBoxes(0),
      mNumChecked(0),
      mMode(Mode_None),
      mData(document.getData()),
      mEditLock(false)
{
    mCommandDispatcher = new CSMWorld::CommandDispatcher(document, id, this);

    connect(&mData, SIGNAL(idListChanged()), this, SLOT(dataIdListChanged()));

    mPerformButton = new QPushButton(this);
    mPerformButton->setDefault(true);
    mPerformButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(mPerformButton, SIGNAL(clicked(bool)), this, SLOT(performExtendedCommand()));

    mCancelButton = new QPushButton("Cancel", this);
    mCancelButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(mCancelButton, SIGNAL(clicked(bool)), this, SIGNAL(done()));

    mTypeGroup = new QGroupBox(this);

    QGridLayout *groupLayout = new QGridLayout(mTypeGroup);
    groupLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mTypeGroup->setLayout(groupLayout);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(mTypeGroup);
    mainLayout->addWidget(mPerformButton);
    mainLayout->addWidget(mCancelButton);
}

void CSVWorld::ExtendedCommandConfigurator::configure(CSVWorld::ExtendedCommandConfigurator::Mode mode,
                                                      const std::vector<std::string> &selectedIds)
{
    mMode = mode;
    if (mMode != Mode_None)
    {
        mPerformButton->setText((mMode == Mode_Delete) ? "Extended Delete" : "Extended Revert");
        mSelectedIds = selectedIds;
        mCommandDispatcher->setSelection(mSelectedIds);

        setupCheckBoxes(mCommandDispatcher->getExtendedTypes());
        setupGroupLayout();
        lockWidgets(mEditLock);
    }
}

void CSVWorld::ExtendedCommandConfigurator::setEditLock(bool locked)
{
    if (mEditLock != locked)
    {
        mEditLock = locked;
        lockWidgets(mEditLock);
    }
}

void CSVWorld::ExtendedCommandConfigurator::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    setupGroupLayout();
}

void CSVWorld::ExtendedCommandConfigurator::setupGroupLayout()
{
    if (mMode == Mode_None)
    {
        return;
    }

    int groupWidth = mTypeGroup->geometry().width();
    QGridLayout *layout = qobject_cast<QGridLayout *>(mTypeGroup->layout());

    // Find the optimal number of rows to place the checkboxes within the available space
    int divider = 1;
    do
    {
        while (layout->itemAt(0) != nullptr)
        {
            layout->removeItem(layout->itemAt(0));
        }

        int counter = 0;
        int itemsPerRow = mNumUsedCheckBoxes / divider;
        CheckBoxMap::const_iterator current = mTypeCheckBoxes.begin();
        CheckBoxMap::const_iterator end = mTypeCheckBoxes.end();
        for (; current != end; ++current)
        {
            if (counter < mNumUsedCheckBoxes)
            {
                int row = counter / itemsPerRow;
                int column = counter - (counter / itemsPerRow) * itemsPerRow;
                layout->addWidget(current->first, row, column);
            }
            ++counter;
        }
        divider *= 2;
    }
    while (groupWidth < mTypeGroup->sizeHint().width() && divider <= mNumUsedCheckBoxes);
}

void CSVWorld::ExtendedCommandConfigurator::setupCheckBoxes(const std::vector<CSMWorld::UniversalId> &types)
{
    // Make sure that we have enough checkboxes
    int numTypes =  static_cast<int>(types.size());
    int numCheckBoxes = static_cast<int>(mTypeCheckBoxes.size());
    if (numTypes > numCheckBoxes)
    {
        for (int i = numTypes - numCheckBoxes; i > 0; --i)
        {
            QCheckBox *checkBox = new QCheckBox(mTypeGroup);
            connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(checkBoxStateChanged(int)));
            mTypeCheckBoxes.insert(std::make_pair(checkBox, CSMWorld::UniversalId::Type_None));
        }
    }

    // Set up the checkboxes
    int counter = 0;
    CheckBoxMap::iterator current = mTypeCheckBoxes.begin();
    CheckBoxMap::iterator end = mTypeCheckBoxes.end();
    for (; current != end; ++current)
    {
        if (counter < numTypes)
        {
            CSMWorld::UniversalId type = types[counter];
            current->first->setText(QString::fromUtf8(type.getTypeName().c_str()));
            current->first->setChecked(true);
            current->second = type;
            ++counter;
        }
        else
        {
            current->first->hide();
        }
    }
    mNumChecked = mNumUsedCheckBoxes = numTypes;
}

void CSVWorld::ExtendedCommandConfigurator::lockWidgets(bool locked)
{
    mPerformButton->setEnabled(!mEditLock && mNumChecked > 0);

    CheckBoxMap::const_iterator current = mTypeCheckBoxes.begin();
    CheckBoxMap::const_iterator end = mTypeCheckBoxes.end();
    for (int i = 0; current != end && i < mNumUsedCheckBoxes; ++current, ++i)
    {
        current->first->setEnabled(!mEditLock);
    }
}

void CSVWorld::ExtendedCommandConfigurator::performExtendedCommand()
{
    std::vector<CSMWorld::UniversalId> types;
    
    CheckBoxMap::const_iterator current = mTypeCheckBoxes.begin();
    CheckBoxMap::const_iterator end = mTypeCheckBoxes.end();
    for (; current != end; ++current)
    {
        if (current->first->isChecked())
        {
            types.push_back(current->second);
        }
    }

    mCommandDispatcher->setExtendedTypes(types);
    if (mMode == Mode_Delete)
    {
        mCommandDispatcher->executeExtendedDelete();
    }
    else
    {
        mCommandDispatcher->executeExtendedRevert();
    }
    emit done();
}

void CSVWorld::ExtendedCommandConfigurator::checkBoxStateChanged(int state)
{
    switch (state)
    {
        case Qt::Unchecked:
            --mNumChecked;
            break;
        case Qt::Checked:
            ++mNumChecked;
            break;
        case Qt::PartiallyChecked: // Not used
            break;
    }

    mPerformButton->setEnabled(mNumChecked > 0);
}

void CSVWorld::ExtendedCommandConfigurator::dataIdListChanged()
{
    bool idsRemoved = false;
    for (int i = 0; i < static_cast<int>(mSelectedIds.size()); ++i)
    {
        if (!mData.hasId(mSelectedIds[i]))
        {
            std::swap(mSelectedIds[i], mSelectedIds.back());
            mSelectedIds.pop_back();
            idsRemoved = true;
            --i;
        }
    }

    // If all selected IDs were removed, cancel the configurator
    if (mSelectedIds.empty())
    {
        emit done();
        return;
    }

    if (idsRemoved)
    {
        mCommandDispatcher->setSelection(mSelectedIds);
    }
}
