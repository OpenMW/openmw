#include "escape.hpp"

#include <components/misc/stringops.hpp>

namespace Files
{
    const int escape_hash_filter::sEscape = '@';
    const int escape_hash_filter::sEscapeIdentifier = 'a';
    const int escape_hash_filter::sHashIdentifier = 'h';

    escape_hash_filter::escape_hash_filter() : mSeenNonWhitespace(false), mFinishLine(false)
    {
    }

    escape_hash_filter::~escape_hash_filter()
    {
    }

    unescape_hash_filter::unescape_hash_filter() : expectingIdentifier(false)
    {
    }

    unescape_hash_filter::~unescape_hash_filter()
    {
    }

    std::string EscapeHashString::processString(const std::string & str)
    {
        std::string temp = str;

        static const char hash[] = { escape_hash_filter::sEscape,  escape_hash_filter::sHashIdentifier };
        Misc::StringUtils::replaceAll(temp, hash, "#", 2, 1);

        static const char escape[] = { escape_hash_filter::sEscape,  escape_hash_filter::sEscapeIdentifier };
        Misc::StringUtils::replaceAll(temp, escape, "@", 2, 1);

        return temp;
    }

    EscapeHashString::EscapeHashString() : mData()
    {
    }

    EscapeHashString::EscapeHashString(const std::string & str) : mData(EscapeHashString::processString(str))
    {
    }

    EscapeHashString::EscapeHashString(const std::string & str, size_t pos, size_t len) : mData(EscapeHashString::processString(str), pos, len)
    {
    }

    EscapeHashString::EscapeHashString(const char * s) : mData(EscapeHashString::processString(std::string(s)))
    {
    }

    EscapeHashString::EscapeHashString(const char * s, size_t n) : mData(EscapeHashString::processString(std::string(s)), 0, n)
    {
    }

    EscapeHashString::EscapeHashString(size_t n, char c) : mData(n, c)
    {
    }

    template <class InputIterator>
    EscapeHashString::EscapeHashString(InputIterator first, InputIterator last) : mData(EscapeHashString::processString(std::string(first, last)))
    {
    }

    std::string EscapeHashString::toStdString() const
    {
        return std::string(mData);
    }

    std::istream & operator>> (std::istream & is, EscapeHashString & eHS)
    {
        std::string temp;
        is >> temp;
        eHS = EscapeHashString(temp);
        return is;
    }

    std::ostream & operator<< (std::ostream & os, const EscapeHashString & eHS)
    {
        os << eHS.mData;
        return os;
    }

    EscapeStringVector::EscapeStringVector() : mVector()
    {
    }

    EscapeStringVector::~EscapeStringVector()
    {
    }

    std::vector<std::string> EscapeStringVector::toStdStringVector() const
    {
        std::vector<std::string> temp = std::vector<std::string>();
        for (std::vector<EscapeHashString>::const_iterator it = mVector.begin(); it != mVector.end(); ++it)
        {
            temp.push_back(it->toStdString());
        }
        return temp;
    }

    // boost program options validation

    void validate(boost::any &v, const std::vector<std::string> &tokens, Files::EscapeHashString * eHS, int a)
    {
        boost::program_options::validators::check_first_occurrence(v);

        if (v.empty())
            v = boost::any(EscapeHashString(boost::program_options::validators::get_single_string(tokens)));
    }

    void validate(boost::any &v, const std::vector<std::string> &tokens, EscapeStringVector *, int)
    {
        if (v.empty())
            v = boost::any(EscapeStringVector());

        EscapeStringVector * eSV = boost::any_cast<EscapeStringVector>(&v);

        for (std::vector<std::string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it)
            eSV->mVector.emplace_back(*it);
    }

    PathContainer EscapePath::toPathContainer(const EscapePathContainer & escapePathContainer)
    {
        PathContainer temp;
        for (EscapePathContainer::const_iterator it = escapePathContainer.begin(); it != escapePathContainer.end(); ++it)
            temp.push_back(it->mPath);
        return temp;
    }

    std::istream & operator>> (std::istream & istream, EscapePath & escapePath)
    {
        boost::iostreams::filtering_istream filteredStream;
        filteredStream.push(unescape_hash_filter());
        filteredStream.push(istream);

        filteredStream >> escapePath.mPath;

        return istream;
    }
}
