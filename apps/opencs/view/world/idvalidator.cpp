
#include "idvalidator.hpp"

bool CSVWorld::IdValidator::isValid (const QChar& c, bool first) const
{
    if (c.isLetter() || c=='_')
        return true;

    if (!first && (c.isDigit()  || c.isSpace()))
        return true;

    return false;
}

CSVWorld::IdValidator::IdValidator (bool relaxed, QObject *parent)
: QValidator (parent), mRelaxed (relaxed)
{}

QValidator::State CSVWorld::IdValidator::validate (QString& input, int& pos) const
{
    if (mRelaxed)
    {
        if (input.indexOf ('"')!=-1 || input.indexOf ("::")!=-1 || input.indexOf ("#")!=-1)
            return QValidator::Invalid;
    }
    else
    {
        bool first = true;

        for (QString::const_iterator iter (input.begin()); iter!=input.end(); ++iter, first = false)
            if (!isValid (*iter, first))
                return QValidator::Invalid;
    }

    return QValidator::Acceptable;
}