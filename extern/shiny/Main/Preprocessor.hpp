#ifndef SH_PREPROCESSOR_H
#define SH_PREPROCESSOR_H

#include <string>
#include <vector>

#include <cstdio>
#include <ostream>
#include <string>
#include <algorithm>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/wave/cpp_throw.hpp>
#include <boost/wave/cpp_exceptions.hpp>
#include <boost/wave/token_ids.hpp>
#include <boost/wave/util/macro_helpers.hpp>
#include <boost/wave/preprocessing_hooks.hpp>

namespace sh
{
	/**
	 * @brief A simple interface for the boost::wave preprocessor
	 */
	class Preprocessor
	{
	public:
		/**
		 * @brief Run a shader source string through the preprocessor
		 * @param source source string
		 * @param includePath path to search for includes (that are included with #include)
		 * @param definitions macros to predefine (vector of strings of the format MACRO=value, or just MACRO to define it as 1)
		 * @param name name to use for error messages
		 * @return processed string
		 */
		static std::string preprocess (std::string source, const std::string& includePath, std::vector<std::string> definitions, const std::string& name);
	};



	class emit_custom_line_directives_hooks
	:   public boost::wave::context_policies::default_preprocessing_hooks
	{
	public:

		template <typename ContextT, typename ContainerT>
		bool
		emit_line_directive(ContextT const& ctx, ContainerT &pending,
			typename ContextT::token_type const& act_token)
		{
		// emit a #line directive showing the relative filename instead
		typename ContextT::position_type pos = act_token.get_position();
		unsigned int column = 1;

			typedef typename ContextT::token_type result_type;

			// no line directives for now
			pos.set_column(column);
			pending.push_back(result_type(boost::wave::T_GENERATEDNEWLINE, "\n", pos));

			return true;
		}
	};


}

#endif
