
#include "vartypedelegate.hpp"

#include <QUndoStack>

#include "../../model/world/commands.hpp"

void CSVWorld::VarTypeDelegate::addCommands (QAbstractItemModel *model, const QModelIndex& index, int type)
    const
{
    QModelIndex next = model->index (index.row(), index.column()+1);

    QVariant old = model->data (next);

    QVariant value;

    switch (type)
    {
        case ESM::VT_Short:
        case ESM::VT_Int:
        case ESM::VT_Long:

            value = old.toInt();
            break;

        case ESM::VT_Float:

            value = old.toFloat();
            break;

        case ESM::VT_String:

            value = old.toString();
            break;

        default: break; // ignore the rest
    }

    getUndoStack().beginMacro (
        "Modify " + model->headerData (index.column(), Qt::Horizontal, Qt::DisplayRole).toString());

    getUndoStack().push (new CSMWorld::ModifyCommand (*model, index, type));
    getUndoStack().push (new CSMWorld::ModifyCommand (*model, next, value));

    getUndoStack().endMacro();
}

CSVWorld::VarTypeDelegate::VarTypeDelegate (const std::vector<std::pair<int, QString> >& values,
    QUndoStack& undoStack, QObject *parent)
: EnumDelegate (values, undoStack, parent)
{}


CSVWorld::VarTypeDelegateFactory::VarTypeDelegateFactory (ESM::VarType type0,
    ESM::VarType type1, ESM::VarType type2, ESM::VarType type3)
{
    if (type0!=ESM::VT_Unknown)
        add (type0);

    if (type1!=ESM::VT_Unknown)
        add (type1);

    if (type2!=ESM::VT_Unknown)
        add (type2);

    if (type3!=ESM::VT_Unknown)
        add (type3);
}

CSVWorld::CommandDelegate *CSVWorld::VarTypeDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{
    return new VarTypeDelegate (mValues, undoStack, parent);
}

void CSVWorld::VarTypeDelegateFactory::add (ESM::VarType type)
{
    struct Name
    {
        ESM::VarType mType;
        const char *mName;
    };

    static const Name sNames[] =
    {
        { ESM::VT_None, "empty" },
        { ESM::VT_Short, "short" },
        { ESM::VT_Int, "integer" },
        { ESM::VT_Long, "long" },
        { ESM::VT_Float, "float" },
        { ESM::VT_String, "string" },
        { ESM::VT_Unknown, 0 } // end marker
    };

    for (int i=0; sNames[i].mName; ++i)
        if (sNames[i].mType==type)
        {
            mValues.push_back (std::make_pair (type, sNames[i].mName));
            return;
        }

    throw std::logic_error ("Unsupported variable type");
}
