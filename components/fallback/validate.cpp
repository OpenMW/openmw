#include "validate.hpp"

void Fallback::validate(boost::any& v, std::vector<std::string> const& tokens, FallbackMap*, int)
{
    if (v.empty())
    {
        v = boost::any(FallbackMap());
    }

    FallbackMap *map = boost::any_cast<FallbackMap>(&v);

    for (std::vector<std::string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it)
    {
        std::string temp = Files::EscapeHashString::processString(*it);
        int sep = temp.find(",");
        if (sep < 1 || sep == (int)temp.length() - 1)
#if (BOOST_VERSION < 104200)
            throw boost::program_options::validation_error("invalid value");
#else
            throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
#endif

        std::string key(temp.substr(0, sep));
        std::string value(temp.substr(sep + 1));

        map->mMap[key] = value;
    }
}
