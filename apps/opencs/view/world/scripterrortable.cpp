#include "scripterrortable.hpp"

#include <QHeaderView>

#include <components/compiler/tokenloc.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/fileparser.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/extensions0.hpp>

#include "../../model/doc/document.hpp"
#include "../../model/settings/usersettings.hpp"

void CSVWorld::ScriptErrorTable::report (const std::string& message, const Compiler::TokenLoc& loc, Type type)
{
    std::ostringstream stream;
    stream << message << " (" << loc.mLiteral << ")";

    addMessage (stream.str(), type==Compiler::ErrorHandler::WarningMessage ?
        CSMDoc::Message::Severity_Warning : CSMDoc::Message::Severity_Error,
        loc.mLine, loc.mColumn-loc.mLiteral.length());
}

void CSVWorld::ScriptErrorTable::report (const std::string& message, Type type)
{
    addMessage (message, type==Compiler::ErrorHandler::WarningMessage ?
        CSMDoc::Message::Severity_Warning : CSMDoc::Message::Severity_Error);
}

void CSVWorld::ScriptErrorTable::addMessage (const std::string& message,
    CSMDoc::Message::Severity severity, int line, int column)
{
    int row = rowCount();

    setRowCount (row+1);

    QTableWidgetItem *severityItem = new QTableWidgetItem (
        QString::fromUtf8 (CSMDoc::Message::toString (severity).c_str()));
    severityItem->setFlags (severityItem->flags() ^ Qt::ItemIsEditable);
    setItem (row, 0, severityItem);

    if (line!=-1)
    {
        QTableWidgetItem *lineItem = new QTableWidgetItem;
        lineItem->setData (Qt::DisplayRole, line+1);
        lineItem->setFlags (lineItem->flags() ^ Qt::ItemIsEditable);
        setItem (row, 1, lineItem);

        QTableWidgetItem *columnItem = new QTableWidgetItem;
        columnItem->setData (Qt::DisplayRole, column);
        columnItem->setFlags (columnItem->flags() ^ Qt::ItemIsEditable);
        setItem (row, 3, columnItem);
    }

    QTableWidgetItem *messageItem = new QTableWidgetItem (QString::fromUtf8 (message.c_str()));
    messageItem->setFlags (messageItem->flags() ^ Qt::ItemIsEditable);
    setItem (row, 2, messageItem);
}

void CSVWorld::ScriptErrorTable::setWarningsMode (const QString& value)
{
    if (value=="Ignore")
        Compiler::ErrorHandler::setWarningsMode (0);
    else if (value=="Normal")
        Compiler::ErrorHandler::setWarningsMode (1);
    else if (value=="Strict")
        Compiler::ErrorHandler::setWarningsMode (2);
}

CSVWorld::ScriptErrorTable::ScriptErrorTable (const CSMDoc::Document& document, QWidget *parent)
: QTableWidget (parent), mContext (document.getData())
{
    setColumnCount (4);

    QStringList headers;
    headers << "Severity" << "Line" << "Description";
    setHorizontalHeaderLabels (headers);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    horizontalHeader()->setSectionResizeMode (0, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode (1, QHeaderView::ResizeToContents);
#else
    horizontalHeader()->setResizeMode (0, QHeaderView::ResizeToContents);
    horizontalHeader()->setResizeMode (1, QHeaderView::ResizeToContents);
#endif
    horizontalHeader()->setStretchLastSection (true);
    verticalHeader()->hide();
    setColumnHidden (3, true);

    setSelectionMode (QAbstractItemView::NoSelection);

    Compiler::registerExtensions (mExtensions);
    mContext.setExtensions (&mExtensions);

    setWarningsMode (CSMSettings::UserSettings::instance().settingValue ("script-editor/warnings"));

    connect (this, SIGNAL (cellClicked (int, int)), this, SLOT (cellClicked (int, int)));
}

void CSVWorld::ScriptErrorTable::updateUserSetting (const QString& name, const QStringList& value)
{
    if (name=="script-editor/warnings" && !value.isEmpty())
        setWarningsMode (value.at (0));
}

void CSVWorld::ScriptErrorTable::update (const std::string& source)
{
    clear();

    try
    {
        std::istringstream input (source);

        Compiler::Scanner scanner (*this, input, mContext.getExtensions());

        Compiler::FileParser parser (*this, mContext);

        scanner.scan (parser);
    }
    catch (const Compiler::SourceException&)
    {
        // error has already been reported via error handler
    }
    catch (const std::exception& error)
    {
        addMessage (error.what(), CSMDoc::Message::Severity_SeriousError);
    }
}

void CSVWorld::ScriptErrorTable::clear()
{
    setRowCount (0);
}

void CSVWorld::ScriptErrorTable::cellClicked (int row, int column)
{
    if (item (row, 1))
    {
        int scriptLine = item (row, 1)->data (Qt::DisplayRole).toInt();
        int scriptColumn = item (row, 3)->data (Qt::DisplayRole).toInt();
        emit highlightError (scriptLine-1, scriptColumn);
    }
}
