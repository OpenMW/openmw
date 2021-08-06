#include "cellcreator.hpp"

#include <limits>
#include <sstream>

#include <QComboBox>
#include <QSpinBox>
#include <QLabel>

#include "../../model/world/commands.hpp"
#include "../../model/world/idtree.hpp"

std::string CSVWorld::CellCreator::getId() const
{
    if (mType->currentIndex()==0)
        return GenericCreator::getId();

    std::ostringstream stream;

    stream << "#" << mX->value() << " " << mY->value();

    return stream.str();
}

void CSVWorld::CellCreator::configureCreateCommand(CSMWorld::CreateCommand& command) const
{
    CSMWorld::IdTree* model = &dynamic_cast<CSMWorld::IdTree&>(*getData().getTableModel(getCollectionId()));

    int parentIndex = model->findColumnIndex(CSMWorld::Columns::ColumnId_Cell);
    int index = model->findNestedColumnIndex(parentIndex, CSMWorld::Columns::ColumnId_Interior);
    command.addNestedValue(parentIndex, index, mType->currentIndex() == 0);
}

CSVWorld::CellCreator::CellCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id)
: GenericCreator (data, undoStack, id)
{
    mY = new QSpinBox (this);
    mY->setVisible (false);
    mY->setMinimum (std::numeric_limits<int>::min());
    mY->setMaximum (std::numeric_limits<int>::max());
    connect (mY, SIGNAL (valueChanged (int)), this, SLOT (valueChanged (int)));
    insertAtBeginning (mY, true);

    mYLabel = new QLabel ("Y", this);
    mYLabel->setVisible (false);
    insertAtBeginning (mYLabel, false);

    mX = new QSpinBox (this);
    mX->setVisible (false);
    mX->setMinimum (std::numeric_limits<int>::min());
    mX->setMaximum (std::numeric_limits<int>::max());
    connect (mX, SIGNAL (valueChanged (int)), this, SLOT (valueChanged (int)));
    insertAtBeginning (mX, true);

    mXLabel = new QLabel ("X", this);
    mXLabel->setVisible (false);
    insertAtBeginning (mXLabel, false);

    mType = new QComboBox (this);

    mType->addItem ("Interior Cell");
    mType->addItem ("Exterior Cell");

    connect (mType, SIGNAL (currentIndexChanged (int)), this, SLOT (setType (int)));

    insertAtBeginning (mType, false);
}

void CSVWorld::CellCreator::reset()
{
    mX->setValue (0);
    mY->setValue (0);
    mType->setCurrentIndex (0);
    setType(0);
    GenericCreator::reset();
}

void CSVWorld::CellCreator::setType (int index)
{
    setManualEditing (index==0);
    mXLabel->setVisible (index==1);
    mX->setVisible (index==1);
    mYLabel->setVisible (index==1);
    mY->setVisible (index==1);

    // The cell name is limited to 64 characters. (ESM::Header::GMDT::mCurrentCell)
    std::string text = mType->currentText().toStdString();
    if (text == "Interior Cell")
        GenericCreator::setEditorMaxLength (64);
    else
        GenericCreator::setEditorMaxLength (32767);

    update();
}

void CSVWorld::CellCreator::valueChanged (int index)
{
    update();
}

void CSVWorld::CellCreator::cloneMode(const std::string& originId, 
                                      const CSMWorld::UniversalId::Type type)
{
    CSVWorld::GenericCreator::cloneMode(originId, type);
    if (*(originId.begin()) == '#') //if originid points to the exterior cell
    {
        setType(1); //enable x and y controls
        mType->setCurrentIndex(1);
    } else {
        setType(0);
        mType->setCurrentIndex(0);
    }
}

std::string CSVWorld::CellCreator::getErrors() const
{
    std::string errors;
    if (mType->currentIndex() == 0)
    {
        errors = GenericCreator::getErrors();
    }
    else if (getData().hasId(getId()))
    {
        errors = "The Exterior Cell is already exist";
    }
    return errors;
}
