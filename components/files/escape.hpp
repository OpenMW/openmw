#ifndef COMPONENTS_FILES_ESCAPE_HPP
#define COMPONENTS_FILES_ESCAPE_HPP

#include <queue>

#include <components/files/multidircollection.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

/**
 * \namespace Files
 */
namespace Files
{
    /**
    * \struct escape_hash_filter
    */
    struct escape_hash_filter : public boost::iostreams::input_filter
    {
        static const int sEscape;
        static const int sHashIdentifier;
        static const int sEscapeIdentifier;

        escape_hash_filter();
        virtual ~escape_hash_filter();

        template <typename Source> int get(Source & src);

    private:
        std::queue<int> mNext;

        bool mSeenNonWhitespace;
        bool mFinishLine;
    };

    template <typename Source>
    int escape_hash_filter::get(Source & src)
    {
        if (mNext.empty())
        {
            int character = boost::iostreams::get(src);
            if (character == boost::iostreams::WOULD_BLOCK)
            {
                mNext.push(character);
            }
            else if (character == EOF)
            {
                mSeenNonWhitespace = false;
                mFinishLine = false;
                mNext.push(character);
            }
            else if (character == '\n')
            {
                mSeenNonWhitespace = false;
                mFinishLine = false;
                mNext.push(character);
            }
            else if (mFinishLine)
            {
                mNext.push(character);
            }
            else if (character == '#')
            {
                if (mSeenNonWhitespace)
                {
                    mNext.push(sEscape);
                    mNext.push(sHashIdentifier);
                }
                else
                {
                    //it's fine being interpreted by Boost as a comment, and so is anything afterwards
                    mNext.push(character);
                    mFinishLine = true;
                }
            }
            else if (character == sEscape)
            {
                mNext.push(sEscape);
                mNext.push(sEscapeIdentifier);
            }
            else
            {
                mNext.push(character);
            }
            if (!mSeenNonWhitespace && !isspace(character))
                mSeenNonWhitespace = true;
        }
        int retval = mNext.front();
        mNext.pop();
        return retval;
    }

    struct unescape_hash_filter : public boost::iostreams::input_filter
    {
        unescape_hash_filter();
        virtual ~unescape_hash_filter();

        template <typename Source> int get(Source & src);

    private:
        bool expectingIdentifier;
    };

    template <typename Source>
    int unescape_hash_filter::get(Source & src)
    {
        int character;
        if (!expectingIdentifier)
            character = boost::iostreams::get(src);
        else
        {
            character = escape_hash_filter::sEscape;
            expectingIdentifier = false;
        }
        if (character == escape_hash_filter::sEscape)
        {
            int nextChar = boost::iostreams::get(src);
            int intended;
            if (nextChar == escape_hash_filter::sEscapeIdentifier)
                intended = escape_hash_filter::sEscape;
            else if (nextChar == escape_hash_filter::sHashIdentifier)
                intended = '#';
            else if (nextChar == boost::iostreams::WOULD_BLOCK)
            {
                expectingIdentifier = true;
                intended = nextChar;
            }
            else
                intended = '?';
            return intended;
        }
        else
            return character;
    }

    /**
    * \class EscapeHashString
    */
    class EscapeHashString
    {
    private:
        std::string mData;
    public:
        static std::string processString(const std::string & str);

        EscapeHashString();
        EscapeHashString(const std::string & str);
        EscapeHashString(const std::string & str, size_t pos, size_t len = std::string::npos);
        EscapeHashString(const char * s);
        EscapeHashString(const char * s, size_t n);
        EscapeHashString(size_t n, char c);
        template <class InputIterator>
        EscapeHashString(InputIterator first, InputIterator last);

        std::string toStdString() const;

        friend std::ostream & operator<< (std::ostream & os, const EscapeHashString & eHS);
    };

    std::istream & operator>> (std::istream & is, EscapeHashString & eHS);

    struct EscapeStringVector
    {
        std::vector<Files::EscapeHashString> mVector;

        EscapeStringVector();
        virtual ~EscapeStringVector();

        std::vector<std::string> toStdStringVector() const;
    };

    //boost program options validation

    void validate(boost::any &v, const std::vector<std::string> &tokens, Files::EscapeHashString * eHS, int a);

    void validate(boost::any &v, const std::vector<std::string> &tokens, EscapeStringVector *, int);

    struct EscapePath
    {
        boost::filesystem::path mPath;

        static PathContainer toPathContainer(const std::vector<EscapePath> & escapePathContainer);
    };

    typedef std::vector<EscapePath> EscapePathContainer;

    std::istream & operator>> (std::istream & istream, EscapePath & escapePath);
} /* namespace Files */
#endif /* COMPONENTS_FILES_ESCAPE_HPP */
