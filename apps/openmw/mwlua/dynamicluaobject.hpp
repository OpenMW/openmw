#pragma once

/*
	Taken from the Sol2 documentation for dynamic objects:
	https://github.com/ThePhD/sol2/blob/develop/examples/dynamic_object.cpp

	Structures that inherit this also need to implement the following definitions in
	their new_usertype call:
	sol::meta_function::index, &DynamicLuaObject::dynamic_get,
	sol::meta_function::new_index, &DynamicLuaObject::dynamic_set,
	sol::meta_function::length, [](DynamicLuaObject& d) { return d.entries.size(); },
*/

#include <unordered_map>

struct DynamicLuaObject {
	std::unordered_map<std::string, sol::object> entries;

	void dynamic_set(std::string key, sol::stack_object value) {
		auto it = entries.find(key);
		if (it == entries.cend()) {
			entries.insert(it, { std::move(key), std::move(value) });
		}
		else {
			std::pair<const std::string, sol::object>& kvp = *it;
			sol::object& entry = kvp.second;
			entry = sol::object(std::move(value));
		}
	}

	sol::object dynamic_get(std::string key) {
		auto it = entries.find(key);
		if (it == entries.cend()) {
			return sol::lua_nil;
		}
		return it->second;
	}
};
