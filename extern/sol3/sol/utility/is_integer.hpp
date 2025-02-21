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

#ifndef SOL_IS_INTEGER_HPP
#define SOL_IS_INTEGER_HPP

#include <sol/object.hpp>

namespace sol::utility {

	// Returns true if the object is represented by an integer,
	// not a floating point number or any other type.
	inline bool is_integer(const sol::object& object) {
		auto pp = stack::push_pop(object);
		return lua_isinteger(object.lua_state(), -1);
	}
	inline bool is_integer(const sol::stack_object& object) {
		return lua_isinteger(object.lua_state(), object.stack_index());
	}

} // namespace sol::utility

#endif // SOL_IS_INTEGER_HPP
