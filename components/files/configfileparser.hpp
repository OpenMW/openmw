#ifndef COMPONENTS_FILES_CONFIGFILEPARSER_HPP
#define COMPONENTS_FILES_CONFIGFILEPARSER_HPP

#include <boost/program_options/parsers.hpp>

// NOLINTBEGIN(readability-identifier-naming)

namespace Files
{

    namespace bpo = boost::program_options;

    template <class charT>
    bpo::basic_parsed_options<charT> parse_config_file(
        std::basic_istream<charT>&, const bpo::options_description&, bool allow_unregistered = false);

}

// NOLINTEND(readability-identifier-naming)

#endif // COMPONENTS_FILES_CONFIGFILEPARSER_HPP
