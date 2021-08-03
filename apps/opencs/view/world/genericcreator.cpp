#include "genericcreator.hpp"

#include <memory>

#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QUndoStack>
#include <QLabel>
#include <QComboBox>

#include <components/misc/stringops.hpp>

#include "../../model/world/commands.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"

#include "idvalidator.hpp"

void CSVWorld::GenericCreator::update()
{
    mErrors = getErrors();

    mCreate->setToolTip (QString::fromUtf8 (mErrors.c_str()));
    mId->setToolTip (QString::fromUtf8 (mErrors.c_str()));

    mCreate->setEnabled (mErrors.empty() && !mLocked);
}

void CSVWorld::GenericCreator::setManualEditing (bool enabled)
{
    mId->setVisible (enabled);
}

void CSVWorld::GenericCreator::insertAtBeginning (QWidget *widget, bool stretched)
{
    mLayout->insertWidget (0, widget, stretched ? 1 : 0);
}

void CSVWorld::GenericCreator::insertBeforeButtons (QWidget *widget, bool stretched)
{
    mLayout->insertWidget (mLayout->count()-2, widget, stretched ? 1 : 0);

    // Reset tab order relative to buttons.
    setTabOrder(widget, mCreate);
    setTabOrder(mCreate, mCancel);
}

std::string CSVWorld::GenericCreator::getId() const
{
    return mId->text().toUtf8().constData();
}

std::string CSVWorld::GenericCreator::getClonedId() const
{
    return mClonedId;
}

std::string CSVWorld::GenericCreator::getIdValidatorResult() const
{
    std::string errors;

    if (!mId->hasAcceptableInput())
        errors = mValidator->getError();

    return errors;
}

void CSVWorld::GenericCreator::configureCreateCommand (CSMWorld::CreateCommand& command) const {}

void CSVWorld::GenericCreator::pushCommand (std::unique_ptr<CSMWorld::CreateCommand> command,
    const std::string& id)
{
    mUndoStack.push (command.release());
}

CSMWorld::Data& CSVWorld::GenericCreator::getData() const
{
    return mData;
}

QUndoStack& CSVWorld::GenericCreator::getUndoStack()
{
    return mUndoStack;
}

const CSMWorld::UniversalId& CSVWorld::GenericCreator::getCollectionId() const
{
    return mListId;
}

std::string CSVWorld::GenericCreator::getNamespace() const
{
    CSMWorld::Scope scope = CSMWorld::Scope_Content;

    if (mScope)
    {
        scope = static_cast<CSMWorld::Scope> (mScope->itemData (mScope->currentIndex()).toInt());
    }
    else
    {
        if (mScopes & CSMWorld::Scope_Project)
            scope = CSMWorld::Scope_Project;
        else if (mScopes & CSMWorld::Scope_Session)
            scope = CSMWorld::Scope_Session;
    }

    switch (scope)
    {
        case CSMWorld::Scope_Content: return "";
        case CSMWorld::Scope_Project: return "project::";
        case CSMWorld::Scope_Session: return "session::";
    }

    return "";
}

void CSVWorld::GenericCreator::updateNamespace()
{
    std::string namespace_ = getNamespace();

    mValidator->setNamespace (namespace_);

    int index = mId->text().indexOf ("::");

    if (index==-1)
    {
        // no namespace in old text
        mId->setText (QString::fromUtf8 (namespace_.c_str()) + mId->text());
    }
    else
    {
        std::string oldNamespace =
            Misc::StringUtils::lowerCase (mId->text().left (index).toUtf8().constData());

        if (oldNamespace=="project" || oldNamespace=="session")
            mId->setText (QString::fromUtf8 (namespace_.c_str()) + mId->text().mid (index+2));
    }
}

void CSVWorld::GenericCreator::addScope (const QString& name, CSMWorld::Scope scope,
    const QString& tooltip)
{
    mScope->addItem (name, static_cast<int> (scope));
    mScope->setItemData (mScope->count()-1, tooltip, Qt::ToolTipRole);
}

CSVWorld::GenericCreator::GenericCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id, bool relaxedIdRules)
: mData (data), mUndoStack (undoStack), mListId (id), mLocked (false),
  mClonedType (CSMWorld::UniversalId::Type_None), mScopes (CSMWorld::Scope_Content), mScope (nullptr),
  mScopeLabel (nullptr), mCloneMode (false)
{
    // If the collection ID has a parent type, use it instead.
    // It will change IDs with Record/SubRecord class (used for creators in Dialogue subviews)
    // to IDs with general RecordList class (used for creators in Table subviews).
    CSMWorld::UniversalId::Type listParentType = CSMWorld::UniversalId::getParentType(mListId.getType());
    if (listParentType != CSMWorld::UniversalId::Type_None)
    {
        mListId = listParentType;
    }

    mLayout = new QHBoxLayout;
    mLayout->setContentsMargins (0, 0, 0, 0);

    mId = new QLineEdit;
    mId->setValidator (mValidator = new IdValidator (relaxedIdRules, this));
    mLayout->addWidget (mId, 1);

    mCreate = new QPushButton ("Create");
    mLayout->addWidget (mCreate);

    mCancel = new QPushButton("Cancel");
    mLayout->addWidget(mCancel);

    setLayout (mLayout);

    connect (mCancel, SIGNAL (clicked (bool)), this, SIGNAL (done()));
    connect (mCreate, SIGNAL (clicked (bool)), this, SLOT (create()));

    connect (mId, SIGNAL (textChanged (const QString&)), this, SLOT (textChanged (const QString&)));
    connect (mId, SIGNAL (returnPressed()), this, SLOT (inputReturnPressed()));

    connect (&mData, SIGNAL (idListChanged()), this, SLOT (dataIdListChanged()));
}

void CSVWorld::GenericCreator::setEditorMaxLength (int length)
{
    mId->setMaxLength (length);
}

void CSVWorld::GenericCreator::setEditLock (bool locked)
{
    mLocked = locked;
    update();
}

void CSVWorld::GenericCreator::reset()
{
    mCloneMode = false;
    mId->setText ("");
    update();
    updateNamespace();
}

std::string CSVWorld::GenericCreator::getErrors() const
{
    std::string errors;

    if (!mId->hasAcceptableInput())
        errors = mValidator->getError();
    else if (mData.hasId (getId()))
        errors = "ID is already in use";

    return errors;
}

void CSVWorld::GenericCreator::textChanged (const QString& text)
{
    update();
}

void CSVWorld::GenericCreator::inputReturnPressed()
{
    if (mCreate->isEnabled())
    {
        create();
    }
}

void CSVWorld::GenericCreator::create()
{
    if (!mLocked)
    {
        std::string id = getId();

        std::unique_ptr<CSMWorld::CreateCommand> command;

        if (mCloneMode)
        {
            command.reset (new CSMWorld::CloneCommand (
                dynamic_cast<CSMWorld::IdTable&> (*mData.getTableModel(mListId)), mClonedId, id, mClonedType));
        }
        else
        {
            command.reset (new CSMWorld::CreateCommand (
                dynamic_cast<CSMWorld::IdTable&> (*mData.getTableModel (mListId)), id));

        }

        configureCreateCommand (*command);
        pushCommand (std::move(command), id);

        emit done();
        emit requestFocus(id);
    }
}

void CSVWorld::GenericCreator::cloneMode(const std::string& originId,
                                         const CSMWorld::UniversalId::Type type)
{
    mCloneMode = true;
    mClonedId = originId;
    mClonedType = type;
}

void CSVWorld::GenericCreator::touch(const std::vector<CSMWorld::UniversalId>& ids)
{
    // Combine multiple touch commands into one "macro" command
    mUndoStack.beginMacro("Touch Records");

    CSMWorld::IdTable& table = dynamic_cast<CSMWorld::IdTable&>(*mData.getTableModel(mListId));
    for (const CSMWorld::UniversalId& uid : ids)
    {
        CSMWorld::TouchCommand* touchCmd = new CSMWorld::TouchCommand(table, uid.getId());
        mUndoStack.push(touchCmd);
    }

    // Execute
    mUndoStack.endMacro();
}

void CSVWorld::GenericCreator::toggleWidgets(bool active)
{
}

void CSVWorld::GenericCreator::focus()
{
    mId->setFocus();
}

void CSVWorld::GenericCreator::setScope (unsigned int scope)
{
    mScopes = scope;
    int count = (mScopes & CSMWorld::Scope_Content) + (mScopes & CSMWorld::Scope_Project) +
        (mScopes & CSMWorld::Scope_Session);

    // scope selector widget
    if (count>1)
    {
        mScope = new QComboBox (this);
        insertAtBeginning (mScope, false);

        if (mScopes & CSMWorld::Scope_Content)
            addScope ("Content", CSMWorld::Scope_Content,
                "Record will be stored in the currently edited content file.");

        if (mScopes & CSMWorld::Scope_Project)
            addScope ("Project", CSMWorld::Scope_Project,
                "Record will be stored in a local project file.<p>"
                "Record will be created in the reserved namespace \"project\".<p>"
                "Record is available when running OpenMW via OpenCS.");

        if (mScopes & CSMWorld::Scope_Session)
            addScope ("Session", CSMWorld::Scope_Session,
                "Record exists only for the duration of the current editing session.<p>"
                "Record will be created in the reserved namespace \"session\".<p>"
                "Record is not available when running OpenMW via OpenCS.");

        connect (mScope, SIGNAL (currentIndexChanged (int)), this, SLOT (scopeChanged (int)));

        mScopeLabel = new QLabel ("Scope", this);
        insertAtBeginning (mScopeLabel, false);

        mScope->setCurrentIndex (0);
    }
    else
    {
        delete mScope;
        mScope = nullptr;

        delete mScopeLabel;
        mScopeLabel = nullptr;
    }

    updateNamespace();
}

void CSVWorld::GenericCreator::scopeChanged (int index)
{
    update();
    updateNamespace();
}

void CSVWorld::GenericCreator::dataIdListChanged()
{
    // If the original ID of cloned record was removed, cancel the creator
    if (mCloneMode && !mData.hasId(mClonedId))
    {
        emit done();
    }
}
