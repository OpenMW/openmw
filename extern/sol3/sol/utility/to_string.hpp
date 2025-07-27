// sol2

// The MIT License (MIT)

// Copyright (c) 2013-2022 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this Spermission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SOL_TO_STRING_HPP
#define SOL_TO_STRING_HPP

#include <sol/forward.hpp>
#include <sol/object.hpp>

#include <cstddef>
#include <string>

namespace sol::utility {

	// Converts any object into a string using luaL_tolstring.
	//
	// Note: Uses the metamethod __tostring if available.
	inline std::string to_string(const sol::stack_object& object) {
		std::size_t len;
		const char* str = luaL_tolstring(object.lua_state(), object.stack_index(), &len);

		auto result = std::string(str, len);

		// luaL_tolstring pushes the string onto the stack, but since
		// we have copied it into our std::string by now we should
		// remove it from the stack.
		lua_pop(object.lua_state(), 1);

		return result;
	}

	inline std::string to_string(const sol::object& object) {
		auto pp = sol::stack::push_pop(object);
		return to_string(sol::stack_object(object.lua_state(), -1));
	}

} // namespace sol::utility

#endif // SOL_IS_INTEGER_HPP
