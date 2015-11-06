#include "scripthighlighter.hpp"

#include <sstream>

#include <components/compiler/scanner.hpp>
#include <components/compiler/extensions0.hpp>

#include "../../model/settings/usersettings.hpp"

bool CSVWorld::ScriptHighlighter::parseInt (int value, const Compiler::TokenLoc& loc,
    Compiler::Scanner& scanner)
{
    highlight (loc, Type_Int);
    return true;
}

bool CSVWorld::ScriptHighlighter::parseFloat (float value, const Compiler::TokenLoc& loc,
    Compiler::Scanner& scanner)
{
    highlight (loc, Type_Float);
    return true;
}

bool CSVWorld::ScriptHighlighter::parseName (const std::string& name, const Compiler::TokenLoc& loc,
    Compiler::Scanner& scanner)
{
    highlight (loc, mContext.isId (name) ? Type_Id : Type_Name);
    return true;
}

bool CSVWorld::ScriptHighlighter::parseKeyword (int keyword, const Compiler::TokenLoc& loc,
    Compiler::Scanner& scanner)
{
    if (((mMode==Mode_Console || mMode==Mode_Dialogue) &&
        (keyword==Compiler::Scanner::K_begin || keyword==Compiler::Scanner::K_end ||
        keyword==Compiler::Scanner::K_short || keyword==Compiler::Scanner::K_long ||
        keyword==Compiler::Scanner::K_float))
        || (mMode==Mode_Console && (keyword==Compiler::Scanner::K_if ||
        keyword==Compiler::Scanner::K_endif || keyword==Compiler::Scanner::K_else ||
        keyword==Compiler::Scanner::K_elseif || keyword==Compiler::Scanner::K_while ||
        keyword==Compiler::Scanner::K_endwhile)))
        return parseName (loc.mLiteral, loc, scanner);

    highlight (loc, Type_Keyword);
    return true;
}

bool CSVWorld::ScriptHighlighter::parseSpecial (int code, const Compiler::TokenLoc& loc,
    Compiler::Scanner& scanner)
{
    highlight (loc, Type_Special);
    return true;
}

bool CSVWorld::ScriptHighlighter::parseComment (const std::string& comment,
    const Compiler::TokenLoc& loc, Compiler::Scanner& scanner)
{
    highlight (loc, Type_Comment);
    return true;
}

void CSVWorld::ScriptHighlighter::parseEOF (Compiler::Scanner& scanner)
{}

void CSVWorld::ScriptHighlighter::highlight (const Compiler::TokenLoc& loc, Type type)
{
    int length = static_cast<int> (loc.mLiteral.size());

    int index = loc.mColumn;

    // compensate for bug in Compiler::Scanner (position of token is the character after the token)
    index -= length;

    setFormat (index, length, mScheme[type]);
}

CSVWorld::ScriptHighlighter::ScriptHighlighter (const CSMWorld::Data& data, Mode mode,
    QTextDocument *parent)
: QSyntaxHighlighter (parent), Compiler::Parser (mErrorHandler, mContext), mContext (data),
  mMode (mode)
{
    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();

    QColor color = QColor();

    {
        color.setNamedColor(userSettings.setting("script-editor/colour-int", "Dark magenta"));
        if (!color.isValid())
            color = QColor(Qt::darkMagenta);

        QTextCharFormat format;
        format.setForeground (color);
        mScheme.insert (std::make_pair (Type_Int, format));
    }

    {
        color.setNamedColor(userSettings.setting ("script-editor/colour-float", "Magenta"));
        if (!color.isValid())
            color = QColor(Qt::magenta);

        QTextCharFormat format;
        format.setForeground (color);
        mScheme.insert (std::make_pair (Type_Float, format));
    }

    {
        color.setNamedColor(userSettings.setting ("script-editor/colour-name", "Gray"));
        if (!color.isValid())
            color = QColor(Qt::gray);

        QTextCharFormat format;
        format.setForeground (color);
        mScheme.insert (std::make_pair (Type_Name, format));
    }

    {
        color.setNamedColor(userSettings.setting ("script-editor/colour-keyword", "Red"));
        if (!color.isValid())
            color = QColor(Qt::red);

        QTextCharFormat format;
        format.setForeground (color);
        mScheme.insert (std::make_pair (Type_Keyword, format));
    }

    {
        color.setNamedColor(userSettings.setting ("script-editor/colour-special", "Dark yellow"));
        if (!color.isValid())
            color = QColor(Qt::darkYellow);

        QTextCharFormat format;
        format.setForeground (color);
        mScheme.insert (std::make_pair (Type_Special, format));
    }

    {
        color.setNamedColor(userSettings.setting ("script-editor/colour-comment", "Green"));
        if (!color.isValid())
            color = QColor(Qt::green);

        QTextCharFormat format;
        format.setForeground (color);
        mScheme.insert (std::make_pair (Type_Comment, format));
    }

    {
        color.setNamedColor(userSettings.setting ("script-editor/colour-id", "Blue"));
        if (!color.isValid())
            color = QColor(Qt::blue);

        QTextCharFormat format;
        format.setForeground (color);
        mScheme.insert (std::make_pair (Type_Id, format));
    }

    // configure compiler
    Compiler::registerExtensions (mExtensions);
    mContext.setExtensions (&mExtensions);
}

void CSVWorld::ScriptHighlighter::highlightBlock (const QString& text)
{
    std::istringstream stream (text.toUtf8().constData());

    Compiler::Scanner scanner (mErrorHandler, stream, mContext.getExtensions());

    try
    {
        scanner.scan (*this);
    }
    catch (...) {} // ignore syntax errors
}

void CSVWorld::ScriptHighlighter::invalidateIds()
{
    mContext.invalidateIds();
}

bool CSVWorld::ScriptHighlighter::updateUserSetting (const QString &name, const QStringList &list)
{
    if (list.empty())
        return false;

    QColor color = QColor();

    if (name == "script-editor/colour-int")
    {
        color.setNamedColor(list.at(0));
        if (!color.isValid())
            return false;

        QTextCharFormat format;
        format.setForeground (color);
        mScheme[Type_Int] = format;
    }
    else if (name == "script-editor/colour-float")
    {
        color.setNamedColor(list.at(0));
        if (!color.isValid())
            return false;

        QTextCharFormat format;
        format.setForeground (color);
        mScheme[Type_Float] = format;
    }
    else if (name == "script-editor/colour-name")
    {
        color.setNamedColor(list.at(0));
        if (!color.isValid())
            return false;

        QTextCharFormat format;
        format.setForeground (color);
        mScheme[Type_Name] = format;
    }
    else if (name == "script-editor/colour-keyword")
    {
        color.setNamedColor(list.at(0));
        if (!color.isValid())
            return false;

        QTextCharFormat format;
        format.setForeground (color);
        mScheme[Type_Keyword] = format;
    }
    else if (name == "script-editor/colour-special")
    {
        color.setNamedColor(list.at(0));
        if (!color.isValid())
            return false;

        QTextCharFormat format;
        format.setForeground (color);
        mScheme[Type_Special] = format;
    }
    else if (name == "script-editor/colour-comment")
    {
        color.setNamedColor(list.at(0));
        if (!color.isValid())
            return false;

        QTextCharFormat format;
        format.setForeground (color);
        mScheme[Type_Comment] = format;
    }
    else if (name == "script-editor/colour-id")
    {
        color.setNamedColor(list.at(0));
        if (!color.isValid())
            return false;

        QTextCharFormat format;
        format.setForeground (color);
        mScheme[Type_Id] = format;
    }
    else
        return false;

    return true;
}
