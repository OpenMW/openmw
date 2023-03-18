/*
  Copyright (C) 2016, 2020-2021 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#include "formid.hpp"

#include <charconv>
#include <stdexcept>
#include <string>
#include <system_error>

namespace ESM4
{
    void formIdToString(FormId formId, std::string& str)
    {
        char buf[8 + 1];
        int res = snprintf(buf, 8 + 1, "%08X", formId);
        if (res > 0 && res < 8 + 1)
            str.assign(buf);
        else
            throw std::runtime_error("Possible buffer overflow while converting formId");
    }

    std::string formIdToString(FormId formId)
    {
        std::string str;
        formIdToString(formId, str);
        return str;
    }

    bool isFormId(const std::string& str, FormId* id)
    {
        if (str.size() != 8)
            return false;

        unsigned long value = 0;

        if (auto [_ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value, 16); ec != std::errc())
            return false;

        if (id != nullptr)
            *id = static_cast<FormId>(value);

        return true;
    }

    FormId stringToFormId(const std::string& str)
    {
        if (str.size() != 8)
            throw std::out_of_range("StringToFormId: incorrect string size");

        unsigned long value = 0;

        if (auto [_ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value, 16); ec != std::errc())
            throw std::invalid_argument("StringToFormId: string not a valid hexadecimal number");

        return static_cast<FormId>(value);
    }
}
