
#include "idvalidator.hpp"

bool CSVWorld::IdValidator::isValid (const QChar& c, bool first) const
{
    if (c.isLetter() || c=='_')
        return true;

    if (!first && (c.isDigit()  || c.isSpace()))
        return true;

    return false;
}

CSVWorld::IdValidator::IdValidator (QObject *parent) : QValidator (parent) {}

QValidator::State CSVWorld::IdValidator::validate (QString& input, int& pos) const
{
    bool first = true;

    for (QString::const_iterator iter (input.begin()); iter!=input.end(); ++iter, first = false)
        if (!isValid (*iter, first))
            return QValidator::Invalid;

    return QValidator::Acceptable;
}