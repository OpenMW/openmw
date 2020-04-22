#include "scripthighlighter.hpp"

#include <sstream>

#include <components/compiler/scanner.hpp>
#include <components/compiler/extensions0.hpp>

#include "../../model/prefs/setting.hpp"
#include "../../model/prefs/category.hpp"

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
    // We should take in account multibyte characters
    int length = 0;
    const char* token = loc.mLiteral.c_str();
    while (*token) length += (*token++ & 0xc0) != 0x80;

    int index = loc.mColumn;

    // compensate for bug in Compiler::Scanner (position of token is the character after the token)
    index -= length;

    QTextCharFormat scheme = mScheme[type];
    if (mMarkOccurrences && type == Type_Name && loc.mLiteral == mMarkedWord)
        scheme.merge(mScheme[Type_Highlight]);

    setFormat (index, length, scheme);
}

CSVWorld::ScriptHighlighter::ScriptHighlighter (const CSMWorld::Data& data, Mode mode,
    QTextDocument *parent)
    : QSyntaxHighlighter (parent)
    , Compiler::Parser (mErrorHandler, mContext)
    , mContext (data)
    , mMode (mode)
    , mMarkOccurrences (false)
{
    QColor color ("black");
    QTextCharFormat format;
    format.setForeground (color);

    for (int i=0; i<=Type_Id; ++i)
        mScheme.insert (std::make_pair (static_cast<Type> (i), format));

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

void CSVWorld::ScriptHighlighter::setMarkOccurrences(bool flag)
{
    mMarkOccurrences = flag;
}

void CSVWorld::ScriptHighlighter::setMarkedWord(const std::string& name)
{
    mMarkedWord = name;
}

void CSVWorld::ScriptHighlighter::invalidateIds()
{
    mContext.invalidateIds();
}

bool CSVWorld::ScriptHighlighter::settingChanged (const CSMPrefs::Setting *setting)
{
    if (setting->getParent()->getKey()=="Scripts")
    {
        static const char *const colours[Type_Id+2] =
        {
            "colour-int", "colour-float", "colour-name", "colour-keyword",
            "colour-special", "colour-comment", "colour-highlight", "colour-id",
            0
        };

        for (int i=0; colours[i]; ++i)
            if (setting->getKey()==colours[i])
            {
                QTextCharFormat format;
                if (i == Type_Highlight)
                    format.setBackground (setting->toColor());
                else
                    format.setForeground (setting->toColor());
                mScheme[static_cast<Type> (i)] = format;
                return true;
            }
    }

    return false;
}
