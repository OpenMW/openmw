#include "vartypedelegate.hpp"

#include <QUndoStack>

#include "../../model/world/commands.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/commandmacro.hpp"

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

    CSMWorld::CommandMacro macro (getUndoStack(), "Modify " + model->headerData (index.column(), Qt::Horizontal, Qt::DisplayRole).toString());

    macro.push (new CSMWorld::ModifyCommand (*model, index, type));
    macro.push (new CSMWorld::ModifyCommand (*model, next, value));
}

CSVWorld::VarTypeDelegate::VarTypeDelegate (const std::vector<std::pair<int, QString> >& values,
    CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent)
: EnumDelegate (values, dispatcher, document, parent)
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

CSVWorld::CommandDelegate *CSVWorld::VarTypeDelegateFactory::makeDelegate (
    CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent) const
{
    return new VarTypeDelegate (mValues, dispatcher, document, parent);
}

void CSVWorld::VarTypeDelegateFactory::add (ESM::VarType type)
{
    std::vector<std::pair<int,std::string>> enums =
        CSMWorld::Columns::getEnums (CSMWorld::Columns::ColumnId_ValueType);

    if (static_cast<size_t>(type) >= enums.size())
        throw std::logic_error ("Unsupported variable type");

    mValues.emplace_back(type, QString::fromUtf8 (enums[type].second.c_str()));
}
