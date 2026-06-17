// This file's contents is largely lifted from boost::program_options with only minor modification.
// Its original preamble (without updated dates) from those source files is below:

// Copyright Vladimir Prus 2002-2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "configfileparser.hpp"

#include <boost/program_options/detail/config_file.hpp>
#include <boost/program_options/detail/convert.hpp>

// NOLINTBEGIN(readability-identifier-naming)

namespace Files
{
    namespace
    {
        /** Standalone parser for config files in ini-line format.
            The parser is a model of single-pass lvalue iterator, and
            default constructor creates past-the-end-iterator. The typical usage is:
            config_file_iterator i(is, ... set of options ...), e;
            for(; i !=e; ++i) {
                *i;
            }

            Syntax conventions:

            - config file can not contain positional options
            - '#' is comment character: it is ignored together with
            the rest of the line.
            - variable assignments are in the form
            name '=' value.
            spaces around '=' are trimmed.
            - Section names are given in brackets.

            The actual option name is constructed by combining current section
            name and specified option name, with dot between. If section_name
            already contains dot at the end, new dot is not inserted. For example:
            @verbatim
            [gui.accessibility]
            visual_bell=yes
            @endverbatim
            will result in option "gui.accessibility.visual_bell" with value
            "yes" been returned.

            TODO: maybe, we should just accept a pointer to options_description
            class.
        */
        class common_config_file_iterator : public boost::eof_iterator<common_config_file_iterator, bpo::option>
        {
        public:
            common_config_file_iterator() { found_eof(); }
            common_config_file_iterator(const std::set<std::string>& allowed_options, bool allow_unregistered = false);

            virtual ~common_config_file_iterator() {}

        public: // Method required by eof_iterator
            void get();

#if BOOST_WORKAROUND(_MSC_VER, <= 1900)
            void decrement() {}
            void advance(difference_type) {}
#endif

        protected: // Stubs for derived classes
            // Obtains next line from the config file
            // Note: really, this design is a bit ugly
            // The most clean thing would be to pass 'line_iterator' to
            // constructor of this class, but to avoid templating this class
            // we'd need polymorphic iterator, which does not exist yet.
            virtual bool getline(std::string&)
            {
                return false;
            }

        protected:
            /** Adds another allowed option. If the 'name' ends with
                '*', then all options with the same prefix are
                allowed. For example, if 'name' is 'foo*', then 'foo1' and
                'foo_bar' are allowed. */
            void add_option(const char* name);

            // Returns true if 's' is a registered option name.
            bool allowed_option(const std::string& s) const;

            // That's probably too much data for iterator, since
            // it will be copied, but let's not bother for now.
            std::set<std::string> allowed_options;
            // Invariant: no element is prefix of other element.
            std::set<std::string> allowed_prefixes;
            std::string m_prefix;
            bool m_allow_unregistered = false;
        };

        common_config_file_iterator::common_config_file_iterator(
            const std::set<std::string>& allowed_options, bool allow_unregistered)
            : allowed_options(allowed_options)
            , m_allow_unregistered(allow_unregistered)
        {
            for (std::set<std::string>::const_iterator i = allowed_options.begin(); i != allowed_options.end(); ++i)
            {
                add_option(i->c_str());
            }
        }

        void common_config_file_iterator::add_option(const char* name)
        {
            std::string s(name);
            assert(!s.empty());
            if (*s.rbegin() == '*')
            {
                s.resize(s.size() - 1);
                bool bad_prefixes(false);
                // If 's' is a prefix of one of allowed suffix, then
                // lower_bound will return that element.
                // If some element is prefix of 's', then lower_bound will
                // return the next element.
                std::set<std::string>::iterator i = allowed_prefixes.lower_bound(s);
                if (i != allowed_prefixes.end())
                {
                    if (i->find(s) == 0)
                        bad_prefixes = true;
                }
                if (i != allowed_prefixes.begin())
                {
                    --i;
                    if (s.find(*i) == 0)
                        bad_prefixes = true;
                }
                if (bad_prefixes)
                    boost::throw_exception(
                        bpo::error("options '" + std::string(name) + "' and '" + *i
                            + "*' will both match the same "
                              "arguments from the configuration file"));
                allowed_prefixes.insert(s);
            }
        }

        std::string trim_ws(const std::string& s)
        {
            std::string::size_type n, n2;
            n = s.find_first_not_of(" \t\r\n");
            if (n == std::string::npos)
                return std::string();
            else
            {
                n2 = s.find_last_not_of(" \t\r\n");
                return s.substr(n, n2 - n + 1);
            }
        }

        void common_config_file_iterator::get()
        {
            std::string s;
            std::string::size_type n;
            bool found = false;

            while (this->getline(s))
            {

                // strip '#' comments and whitespace
                if (s.find('#') == s.find_first_not_of(" \t\r\n"))
                    continue;
                s = trim_ws(s);

                if (!s.empty())
                {
                    // Handle section name
                    if (*s.begin() == '[' && *s.rbegin() == ']')
                    {
                        m_prefix = s.substr(1, s.size() - 2);
                        if (*m_prefix.rbegin() != '.')
                            m_prefix += '.';
                    }
                    else if ((n = s.find('=')) != std::string::npos)
                    {

                        std::string name = m_prefix + trim_ws(s.substr(0, n));
                        std::string value = trim_ws(s.substr(n + 1));

                        bool registered = allowed_option(name);
                        if (!registered && !m_allow_unregistered)
                            boost::throw_exception(bpo::unknown_option(name));

                        found = true;
                        this->value().string_key = name;
                        this->value().value.clear();
                        this->value().value.push_back(value);
                        this->value().unregistered = !registered;
                        this->value().original_tokens.clear();
                        this->value().original_tokens.push_back(name);
                        this->value().original_tokens.push_back(value);
                        break;
                    }
                    else
                    {
                        boost::throw_exception(
                            bpo::invalid_config_file_syntax(s, bpo::invalid_syntax::unrecognized_line));
                    }
                }
            }
            if (!found)
                found_eof();
        }

        bool common_config_file_iterator::allowed_option(const std::string& s) const
        {
            std::set<std::string>::const_iterator i = allowed_options.find(s);
            if (i != allowed_options.end())
                return true;
            // If s is "pa" where "p" is allowed prefix then
            // lower_bound should find the element after "p".
            // This depends on 'allowed_prefixes' invariant.
            i = allowed_prefixes.lower_bound(s);
            if (i != allowed_prefixes.begin() && s.find(*--i) == 0)
                return true;
            return false;
        }

        template <class charT>
        class basic_config_file_iterator : public Files::common_config_file_iterator
        {
        public:
            basic_config_file_iterator() { found_eof(); }

            /** Creates a config file parser for the specified stream.
             */
            basic_config_file_iterator(std::basic_istream<charT>& is, const std::set<std::string>& allowed_options,
                bool allow_unregistered = false);

        private: // base overrides
            bool getline(std::string&);

        private: // internal data
            std::shared_ptr<std::basic_istream<charT>> is;
        };

        template <class charT>
        basic_config_file_iterator<charT>::basic_config_file_iterator(
            std::basic_istream<charT>& is, const std::set<std::string>& allowed_options, bool allow_unregistered)
            : common_config_file_iterator(allowed_options, allow_unregistered)
        {
            this->is.reset(&is, bpo::detail::null_deleter());
            get();
        }

        template <class charT>
        bool basic_config_file_iterator<charT>::getline(std::string& s)
        {
            std::basic_string<charT> in;
            if (std::getline(*is, in))
            {
                s = bpo::to_internal(in);
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    template <class charT>
    bpo::basic_parsed_options<charT> parse_config_file(
        std::basic_istream<charT>& is, const bpo::options_description& desc, bool allow_unregistered)
    {
        std::set<std::string> allowed_options;

        const std::vector<boost::shared_ptr<bpo::option_description>>& options = desc.options();
        for (unsigned i = 0; i < options.size(); ++i)
        {
            const bpo::option_description& d = *options[i];

            if (d.long_name().empty())
                boost::throw_exception(
                    bpo::error("abbreviated option names are not permitted in options configuration files"));

            allowed_options.insert(d.long_name());
        }

        // Parser return char strings
        bpo::parsed_options result(&desc);
        copy(basic_config_file_iterator<charT>(is, allowed_options, allow_unregistered),
            basic_config_file_iterator<charT>(), back_inserter(result.options));
        // Convert char strings into desired type.
        return bpo::basic_parsed_options<charT>(result);
    }

    template bpo::basic_parsed_options<char> parse_config_file(
        std::basic_istream<char>& is, const bpo::options_description& desc, bool allow_unregistered);
}

// NOLINTEND(readability-identifier-naming)
