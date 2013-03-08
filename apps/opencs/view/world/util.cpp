
#include "util.hpp"

#include <stdexcept>

#include <QUndoStack>

#include "../../model/world/commands.hpp"

CSVWorld::NastyTableModelHack::NastyTableModelHack (QAbstractItemModel& model)
: mModel (model)
{}

int CSVWorld::NastyTableModelHack::rowCount (const QModelIndex & parent) const
{
    return mModel.rowCount (parent);
}

int CSVWorld::NastyTableModelHack::columnCount (const QModelIndex & parent) const
{
    return mModel.columnCount (parent);
}

QVariant CSVWorld::NastyTableModelHack::data  (const QModelIndex & index, int role) const
{
    return mModel.data (index, role);
}

bool CSVWorld::NastyTableModelHack::setData ( const QModelIndex &index, const QVariant &value, int role)
{
    mData = value;
    return true;
}

QVariant CSVWorld::NastyTableModelHack::getData() const
{
    return mData;
}


CSVWorld::CommandDelegateFactory::~CommandDelegateFactory() {}


CSVWorld::CommandDelegateFactoryCollection *CSVWorld::CommandDelegateFactoryCollection::sThis = 0;

CSVWorld::CommandDelegateFactoryCollection::CommandDelegateFactoryCollection()
{
    if (sThis)
        throw std::logic_error ("multiple instances of CSVWorld::CommandDelegateFactoryCollection");

    sThis = this;
}

CSVWorld::CommandDelegateFactoryCollection::~CommandDelegateFactoryCollection()
{
    sThis = 0;

    for (std::map<CSMWorld::ColumnBase::Display, CommandDelegateFactory *>::iterator iter (
        mFactories.begin());
        iter!=mFactories.end(); ++iter)
         delete iter->second;
}

void CSVWorld::CommandDelegateFactoryCollection::add (CSMWorld::ColumnBase::Display display,
    CommandDelegateFactory *factory)
{
    mFactories.insert (std::make_pair (display, factory));
}

CSVWorld::CommandDelegate *CSVWorld::CommandDelegateFactoryCollection::makeDelegate (
    CSMWorld::ColumnBase::Display display, QUndoStack& undoStack, QObject *parent) const
{
    std::map<CSMWorld::ColumnBase::Display, CommandDelegateFactory *>::const_iterator iter =
        mFactories.find (display);

    if (iter!=mFactories.end())
        return iter->second->makeDelegate (undoStack, parent);

    return new CommandDelegate (undoStack, parent);
}

const CSVWorld::CommandDelegateFactoryCollection& CSVWorld::CommandDelegateFactoryCollection::get()
{
    if (!sThis)
        throw std::logic_error ("no instance of CSVWorld::CommandDelegateFactoryCollection");

    return *sThis;
}


QUndoStack& CSVWorld::CommandDelegate::getUndoStack() const
{
    return mUndoStack;
}

void CSVWorld::CommandDelegate::setModelDataImp (QWidget *editor, QAbstractItemModel *model,
    const QModelIndex& index) const
{
    NastyTableModelHack hack (*model);
    QStyledItemDelegate::setModelData (editor, &hack, index);
    mUndoStack.push (new CSMWorld::ModifyCommand (*model, index, hack.getData()));
}

CSVWorld::CommandDelegate::CommandDelegate (QUndoStack& undoStack, QObject *parent)
: QStyledItemDelegate (parent), mUndoStack (undoStack), mEditLock (false)
{}

void CSVWorld::CommandDelegate::setModelData (QWidget *editor, QAbstractItemModel *model,
        const QModelIndex& index) const
{
    if (!mEditLock)
    {
        setModelDataImp (editor, model, index);
    }

    ///< \todo provide some kind of feedback to the user, indicating that editing is currently not possible.
}

void CSVWorld::CommandDelegate::setEditLock (bool locked)
{
    mEditLock = locked;
}

bool CSVWorld::CommandDelegate::isEditLocked() const
{
    return mEditLock;
}