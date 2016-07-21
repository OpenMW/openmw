#ifndef COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP
#define COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP

#include <map>
#include <queue>

#include <boost/program_options.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <components/files/fixedpath.hpp>
#include <components/files/collections.hpp>

/**
 * \namespace Files
 */
namespace Files
{

/**
 * \struct ConfigurationManager
 */
struct ConfigurationManager
{
    ConfigurationManager(bool silent=false); /// @param silent Emit log messages to cout?
    virtual ~ConfigurationManager();

    void readConfiguration(boost::program_options::variables_map& variables,
        boost::program_options::options_description& description, bool quiet=false);

    void processPaths(Files::PathContainer& dataDirs, bool create = false);
    ///< \param create Try creating the directory, if it does not exist.

    /**< Fixed paths */
    const boost::filesystem::path& getGlobalPath() const;
    const boost::filesystem::path& getUserConfigPath() const;
    const boost::filesystem::path& getLocalPath() const;

    const boost::filesystem::path& getGlobalDataPath() const;
    const boost::filesystem::path& getUserDataPath() const;
    const boost::filesystem::path& getLocalDataPath() const;
    const boost::filesystem::path& getInstallPath() const;

    const boost::filesystem::path& getCachePath() const;

    const boost::filesystem::path& getLogPath() const;

    private:
        typedef Files::FixedPath<> FixedPathType;

        typedef const boost::filesystem::path& (FixedPathType::*path_type_f)() const;
        typedef std::map<std::string, path_type_f> TokensMappingContainer;

        bool loadConfig(const boost::filesystem::path& path,
            boost::program_options::variables_map& variables,
            boost::program_options::options_description& description);

        void setupTokensMapping();

        FixedPathType mFixedPath;

        boost::filesystem::path mLogPath;

        TokensMappingContainer mTokensMapping;

        bool mSilent;
};


/**
 * \struct escape_hash_filter
 */
struct escape_hash_filter : public boost::iostreams::input_filter
{
    static const int sEscape = '@';
    static const int sHashIdentifier = 'h';
    static const int sEscapeIdentifier = 'a';

    escape_hash_filter();
    virtual ~escape_hash_filter();

    template <typename Source> int get(Source & src);

    private:
        std::queue<int> mNext;
        int mPrevious;

        bool mSeenNonWhitespace;
        bool mFinishLine;
};

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
        switch (nextChar)
        {
        case escape_hash_filter::sEscapeIdentifier:
            intended = escape_hash_filter::sEscape;
            break;
        case escape_hash_filter::sHashIdentifier:
            intended = '#';
            break;
        case boost::iostreams::WOULD_BLOCK:
            expectingIdentifier = true;
            intended = nextChar;
            break;
        default:
            intended = '?';
            break;
        }
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

} /* namespace Cfg */

#endif /* COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP */
