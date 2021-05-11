#include "validate.hpp"

void Fallback::validate(boost::any& v, std::vector<std::string> const& tokens, FallbackMap*, int)
{
    if (v.empty())
    {
        v = boost::any(FallbackMap());
    }

    FallbackMap *map = boost::any_cast<FallbackMap>(&v);

    for (const auto& token : tokens)
    {
        std::string temp = Files::EscapeHashString::processString(token);
        size_t sep = temp.find(',');
        if (sep < 1 || sep == temp.length() - 1 || sep == std::string::npos)
            throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);

        std::string key(temp.substr(0, sep));
        std::string value(temp.substr(sep + 1));

        map->mMap[key] = value;
    }
}
