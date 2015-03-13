
#include "scripthighlighter.hpp"

#include <sstream>

#include <components/compiler/scanner.hpp>
#include <components/compiler/extensions0.hpp>

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
    /// \todo replace this with user settings
    {
        QTextCharFormat format;
        format.setForeground (Qt::darkMagenta);
        mScheme.insert (std::make_pair (Type_Int, format));
    }

    {
        QTextCharFormat format;
        format.setForeground (Qt::magenta);
        mScheme.insert (std::make_pair (Type_Float, format));
    }

    {
        QTextCharFormat format;
        format.setForeground (Qt::gray);
        mScheme.insert (std::make_pair (Type_Name, format));
    }

    {
        QTextCharFormat format;
        format.setForeground (Qt::red);
        mScheme.insert (std::make_pair (Type_Keyword, format));
    }

    {
        QTextCharFormat format;
        format.setForeground (Qt::darkYellow);
        mScheme.insert (std::make_pair (Type_Special, format));
    }

    {
        QTextCharFormat format;
        format.setForeground (Qt::green);
        mScheme.insert (std::make_pair (Type_Comment, format));
    }

    {
        QTextCharFormat format;
        format.setForeground (Qt::blue);
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
