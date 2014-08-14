
#include "idvalidator.hpp"

#include <components/misc/stringops.hpp>

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
        bool scope = false;
        bool prevScope = false;

        QString::const_iterator iter = input.begin();

        if (!mNamespace.empty())
        {
            std::string namespace_ = input.left (mNamespace.size()).toUtf8().constData();

            if (Misc::StringUtils::lowerCase (namespace_)!=mNamespace)
                return QValidator::Invalid; // incorrect namespace

            iter += namespace_.size();
            first = false;
            prevScope = true;
        }
        else
        {
            int index = input.indexOf (":");

            if (index!=-1)
            {
                QString namespace_ = input.left (index);

                if (namespace_=="project" || namespace_=="session")
                    return QValidator::Invalid; // reserved namespace
            }
        }

        for (; iter!=input.end(); ++iter, first = false)
        {
            if (*iter==':')
            {
                if (first)
                    return QValidator::Invalid; // scope operator at the beginning

                if (scope)
                {
                    scope = false;
                    prevScope = true;
                }
                else
                {
                    if (prevScope)
                        return QValidator::Invalid; // sequence of two scope operators

                    scope = true;
                }
            }
            else if (scope)
                return QValidator::Invalid; // incomplete scope operator
            else
            {
                prevScope = false;

                if (!isValid (*iter, first))
                    return QValidator::Invalid;
            }
        }

        if (prevScope)
            return QValidator::Intermediate; // ending with scope operator
    }

    return QValidator::Acceptable;
}

void CSVWorld::IdValidator::setNamespace (const std::string& namespace_)
{
    mNamespace = Misc::StringUtils::lowerCase (namespace_);
}